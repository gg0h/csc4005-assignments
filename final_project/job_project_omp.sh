#!/bin/bash

#SBATCH --job-name=40173582-omp
#SBATCH -o omp.%N.%j.out
#SBATCH -e omp.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4

date

./execute_OMP small-inputs

date