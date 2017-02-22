#!/bin/bash

CXXFLAGS="-Wall -g -std=c++11" ./waf -d debug --enable-examples --boost-includes=/usr/local/include --boost-libs=/usr/local/lib configure

./waf --run "nrndn_test_20160716_backup --method=3"