#ifndef BIJIWALLET_WALLET_HPP
#define BIJIWALLET_WALLET_HPP

#include <map>
#include <vector>
#include <pybind11/pybind11.h>
namespace py = pybind11;

struct transaction_point
{
    py::bytes hash;
    uint32_t index;
    uint64_t height;
};

struct history_row
{
    transaction_point output;
    uint64_t value;
    py::object spend;
};

using history_list = std::vector<history_row>;
using history_map = std::map<std::string, history_list>;

using address_list = std::vector<std::string>;

using point_key_tuple =
    std::tuple<transaction_point, std::string>;
using point_key_list = std::vector<point_key_tuple>;

using key_list = std::vector<std::string>;
using destination_tuple = std::tuple<std::string, uint64_t>;
using destination_list = std::vector<destination_tuple>;

using data_type = std::vector<uint8_t>;

void set_testnet(bool testnet);
py::str new_key();
py::str key_to_address(const std::string& wif_key);
py::object decode_base10(const std::string& amount_string);
py::str encode_base10(const uint64_t value);
std::optional<history_map> fetch_history(const address_list& addresses);
std::optional<point_key_list> select_outputs(
    const std::vector<std::string>& keys, const uint64_t value,
    const history_map& histories);
py::bytes build_transaction(
    const destination_list& destinations, const key_list& keys,
    const point_key_list& unspent);

#endif

