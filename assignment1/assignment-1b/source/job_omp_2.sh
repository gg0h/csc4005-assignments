#!/bin/bash

#SBATCH --job-name=40173582-s_omp_2
#SBATCH -o logs/s_omp_2.%N.%j.out
#SBATCH -e logs/s_omp_2.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4

gcc -fopenmp -O2 --std=gnu99 searching_OMP_2.c -o s_omp_2

date

export OMP_NUM_THREADS=32

# program is dependant on the existence of test0 to run test1 & 2, shuffle the folder to rename test2 -> test0 so real test0 does not interfere with timings.
# REMEBER TO RENAME TESTS ACCORDINGLY
{ time ./s_omp_2; } 2>&1 

date