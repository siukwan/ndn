#!/bin/bash
git pull
./waf --run=nrndn_test > ndn_result.txt
