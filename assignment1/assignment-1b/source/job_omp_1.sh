#!/bin/bash

#SBATCH --job-name=40173582-sequential-pattern-search
#SBATCH -o logs/sequential-pattern-search.%N.%j.out
#SBATCH -e logs/sequential-pattern-search.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4

gcc -fopenmp -O2 --std=gnu99 searching_OMP_1.c -o s_omp_1

date

export OMP_NUM_THREADS=2

{ time ./s_omp_1; } 2>&1  # program is dependant on the existence of test0 to run test1, shuffle the folder to rename test1 -> test0 so real test0 does not interfere with timings.

date