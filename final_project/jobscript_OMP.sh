#!/bin/bash

#SBATCH --job-name=40173582-omp
#SBATCH -o omp.%N.%j.out
#SBATCH -e omp.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4

date

./execute_OMP large_inputs
sort -k 1,1n -k 2,2n -k 3,3n result_OMP.txt > sorted_OMP.txt

date