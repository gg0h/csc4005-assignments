#!/bin/bash

#SBATCH --job-name=40173582-seq
#SBATCH -o seq.%N.%j.out
#SBATCH -e seq.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

gcc -O2 --std=gnu99 -o searching_sequential searching_sequential.c

date
{ time ./searching_sequential; } 2>&1 # a trick to get time to stdout in a script
date
