#!/bin/bash

cd build || exit 1
cmake .. || exit 1
make || exit 1

