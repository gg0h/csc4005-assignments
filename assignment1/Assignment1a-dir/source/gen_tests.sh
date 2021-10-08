#!/bin/bash

# usage ./gen_tests.sh <path/to/output/directory>
# REMEBER TO MOVE TO NODE BEFORE RUNNING

gcc testcase_generation.c --std=c99 -o tc
./tc $(pwd)/file.txt $1
