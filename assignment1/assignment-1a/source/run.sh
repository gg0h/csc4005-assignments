#!/bin/bash

#SBATCH --job-name=40173582-sequential-pattern-search
#SBATCH -o logs/sequential-pattern-search.%N.%j.out
#SBATCH -e logs/sequential-pattern-search.%N.%j.err

#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1

./compile searching_sequential
date
./searching_sequential
date