#!/bin/bash
g++ -fvisibility=hidden -ggdb -shared -fPIC $(python3 -m pybind11 --includes) bindings.cpp wallet.cpp -o wallet$(python3-config --extension-suffix) -std=c++2a -fconcepts $(pkg-config --cflags --libs libbitcoin-client)
