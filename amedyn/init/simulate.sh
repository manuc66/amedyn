#!/bin/bash
make clean
make -e SIMULATE=1
make -e SIMULATE=1 debug
make -e SIMULATE=1 debugt
make install
