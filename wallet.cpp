#include "wallet.hpp"
#include "get_history.ipp"
#include <bitcoin/bitcoin.hpp>

namespace bcs = bc;

auto is_testnet = true;

void set_testnet(bool testnet)
{
    is_testnet = testnet;
}

// Not testable due to lack of random engine injection.
bcs::data_chunk new_seed(size_t bit_length=192)
{
    size_t fill_seed_size = bit_length / bcs::byte_bits;
    bcs::data_chunk seed(fill_seed_size);
    bcs::pseudo_random_fill(seed);
    return seed;
}

// The key may be invalid, caller may test for null secret.
bcs::ec_secret new_key_from_seed(const bcs::data_chunk& seed)
{
    const bcs::wallet::hd_private key(seed);
    return key.secret();
}

py::str new_key()
{
    const auto seed = new_seed();
    const auto secret_key = new_key_from_seed(seed);

    auto version = bcs::wallet::ec_private::mainnet;
    if (is_testnet)
        version = bcs::wallet::ec_private::testnet;

    bcs::wallet::ec_private private_key(secret_key, version);
    return private_key.encoded();
}

auto convert_key_to_private(const std::string& wif_key)
{
    auto version = bcs::wallet::ec_private::mainnet;
    if (is_testnet)
        version = bcs::wallet::ec_private::testnet;

    bcs::wallet::ec_private private_key(wif_key, version);
    return private_key;
}
auto convert_key_to_secret(const std::string& wif_key)
{
    return convert_key_to_private(wif_key).secret();
}
auto convert_key_to_payment_address(const std::string& wif_key)
{
    return convert_key_to_private(wif_key).to_payment_address();
}

py::str key_to_address(const std::string& wif_key)
{
    return convert_key_to_payment_address(wif_key).encoded();
}

py::object decode_base10(const std::string& amount_string)
{
    uint64_t value;
    const auto rc = bcs::decode_base10(value, amount_string, 8);
    if (!rc)
        return py::cast<py::none>(Py_None);
    return py::cast(value);
}
py::str encode_base10(const uint64_t value)
{
    return bcs::encode_base10(value, 8);
}

std::optional<history_map> fetch_history(const address_list& addresses)
{
    const auto blockchain_server_address = is_testnet ?
        "tcp://testnet.libbitcoin.net:19091" :
        "tcp://mainnet.libbitcoin.net:9091";

    std::vector<bcs::wallet::payment_address> payaddrs;
    for (const auto& address: addresses)
        payaddrs.emplace_back(address);

    const auto result = biji::get_history(payaddrs, blockchain_server_address);
    if (!result)
        return std::nullopt;

    history_map histories;
    for (const auto& [address, history]: *result)
    {
        history_list table;
        for (const auto& row: history)
        {
            history_row new_row;
            new_row.output = transaction_point{
                py::bytes(
                    reinterpret_cast<const char*>(row.output.hash().data()),
                    row.output.hash().size()
                ),
                row.output.index(),
                row.output_height
            };

            new_row.value = row.value;

            if (row.spend.index() == bcs::max_uint32)
            {
                new_row.spend = py::cast<py::none>(Py_None);
            }
            else
            {
                new_row.spend = py::cast(new transaction_point{
                    py::bytes(
                        reinterpret_cast<const char*>(row.spend.hash().data()),
                        row.spend.hash().size()
                    ),
                    row.spend.index(),
                    row.spend_height
                });
            }

            table.push_back(new_row);
        }
        histories[address.encoded()] = table;
    }

    return histories;
}

auto convert_keys_to_payment_addresses(const auto& keys)
{
    address_list addresses;
    for (const auto& secret: keys)
        addresses.push_back(convert_key_to_payment_address(secret).encoded());
    return addresses;
}

std::optional<point_key_list> select_outputs(
    const std::vector<std::string>& keys, const uint64_t value,
    const history_map& histories)
{
    const auto addresses = convert_keys_to_payment_addresses(keys);

    std::map<std::string, std::string> keys_map;
    for (auto i = 0; i < keys.size(); ++i)
        keys_map[addresses[i]] = keys[i];

    uint64_t total = 0;
    point_key_list unspent;
    for (const auto& [address, history]: histories)
    {
        for (const auto& row: history)
        {
            // Skip spent outputs
            if (!row.spend.is_none())
                continue;

            const auto& key = keys_map[address];

            // Unspent output
            total += row.value;
            unspent.push_back({ row.output, key });

            if (total >= value)
                goto break_loop;
        }
    }
    if (total < value)
        return std::nullopt;
break_loop:
    BITCOIN_ASSERT(total >= value);
    return unspent;
}

auto bytes_to_hash(const py::bytes& bytes)
{
    std::string bytes_data = bytes;
    bcs::hash_digest result;
    BITCOIN_ASSERT(bytes_data.size() == bcs::hash_size);
    std::copy(bytes_data.begin(), bytes_data.end(), result.data());
    return result;
}

py::bytes build_transaction(
    const destination_list& destinations, const key_list& keys,
    const point_key_list& unspent)
{
    // sum values in dest
    auto fold = [](uint64_t total,
        std::tuple<std::string, uint64_t> destination)
    {
        return total + std::get<1>(destination);
    };
    auto sum = std::accumulate(
        destinations.begin(), destinations.end(), 0, fold);

    // select unspent outputs where sum(outputs) >= sum
    /*const auto unspent = select_outputs(keys, sum);
    if (!unspent)
    {
        std::cerr << "Not enough funds for send." << std::endl;
        return std::nullopt;
    }*/

    // build tx
    bcs::chain::transaction tx;
    tx.set_version(1);
    tx.set_locktime(0);

    bcs::chain::input::list inputs;
    for (const auto& [previous_output, key]: unspent)
    {
        bcs::chain::input input;
        input.set_previous_output({
            bytes_to_hash(previous_output.hash),
            previous_output.index
        });
        input.set_sequence(bcs::max_uint32);
        inputs.push_back(std::move(input));
    }
    tx.set_inputs(inputs);

    bcs::chain::output::list outputs;
    for (const auto& [address, value]: destinations)
    {
        bcs::wallet::payment_address payaddr(address);

        outputs.push_back({
            value,
            bcs::chain::script::to_pay_key_hash_pattern(payaddr.hash())
        });
    }
    tx.set_outputs(std::move(outputs));

    // sign tx
    for (auto i = 0; i < unspent.size(); ++i)
    {
        const auto& [previous_output, key] = unspent[i];
        const auto private_key = convert_key_to_private(key);

        const auto address = convert_key_to_payment_address(key);

        bcs::chain::script prevout_script =
            bcs::chain::script::to_pay_key_hash_pattern(address.hash());

        bcs::endorsement endorsement;
        auto rc = bcs::chain::script::create_endorsement(endorsement,
            private_key, prevout_script, tx, i,
            bcs::machine::sighash_algorithm::all);

        bcs::wallet::ec_public public_key(private_key);

        bcs::chain::script input_script({
            { endorsement },
            bcs::machine::operation({
                public_key.point().begin(), public_key.point().end() })
        });

        inputs[i].set_script(input_script);
    }
    tx.set_inputs(inputs);

    const auto data = tx.to_data();
    return py::bytes(
        reinterpret_cast<const char*>(data.data()), data.size());
}

