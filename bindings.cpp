#include "wallet.hpp"
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

PYBIND11_MAKE_OPAQUE(history_list);
PYBIND11_MAKE_OPAQUE(history_map);

//PYBIND11_MAKE_OPAQUE(address_list);

PYBIND11_MODULE(wallet, m)
{
    py::class_<transaction_point>(m, "transaction_point")
        .def(py::init<>())
        .def_readwrite("hash", &transaction_point::hash)
        .def_readwrite("index", &transaction_point::index)
        .def_readwrite("height", &transaction_point::height);

    py::class_<history_row>(m, "history_row")
        .def(py::init<>())
        .def_readwrite("output", &history_row::output)
        .def_readwrite("value", &history_row::value)
        .def_readwrite("spend", &history_row::spend);

    py::bind_vector<history_list>(m, "history_list");
    py::bind_map<history_map>(m, "history_map");

    //py::bind_vector<address_list>(m, "address_list");

    m.def("set_testnet", &set_testnet);
    m.def("new_key", &new_key);
    m.def("key_to_address", &key_to_address);
    m.def("decode_base10", &decode_base10);
    m.def("encode_base10", &encode_base10);
    m.def("fetch_history", &fetch_history);
    m.def("select_outputs", &select_outputs);
    m.def("build_transaction", &build_transaction);
}

