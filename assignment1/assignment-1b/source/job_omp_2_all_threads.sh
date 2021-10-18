#!/bin/bash

#SBATCH --job-name=40173582-s_omp_2_all_threads
#SBATCH -o logs/s_omp_2.%N.%j.out
#SBATCH -e logs/s_omp_2.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4

gcc -fopenmp -O2 --std=gnu99 searching_OMP_2.c -o s_omp_2

date

for i in {0..6}
do
    export OMP_NUM_THREADS=$(echo "2^$i" | bc)
    { time ./s_omp_2; } 2>&1
done

date