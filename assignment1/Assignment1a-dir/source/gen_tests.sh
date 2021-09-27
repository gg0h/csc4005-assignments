#!/bin/bash

gcc testcase_generation.c -o tc
./tc $(pwd)/file.txt /home/gg0h/csc4005/assignments/assignment1/Assignment1a-dir/inputs 