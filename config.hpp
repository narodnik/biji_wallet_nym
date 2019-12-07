#ifndef BIJIWALLET_CONFIG_HPP
#define BIJIWALLET_CONFIG_HPP

namespace biji {

constexpr auto is_testnet = true;

constexpr auto blockchain_server_address = is_testnet ?
    "tcp://testnet.libbitcoin.net:19091" :
    "tcp://mainnet.libbitcoin.net:9091";

} // namespace biji

#endif // BIJIWALLET_CONFIG_HPP

