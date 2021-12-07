#!/bin/bash

#SBATCH --job-name=40173582-MPI
#SBATCH -o mpi.%N.%j.out
#SBATCH -e mpi.%N.%j.err

# Ask the scheduler to run N MPI processes on N compute nodes
#SBATCH --nodes=4
#SBATCH --ntasks=4

# Load mpi module
module add mpi/openmpi

date

./execute_MPI small_inputs
sort -k 1,1n -k 2,2n -k 3,3n result_MPI.txt > sorted_MPI.txt
diff sorted_MPI.txt small_inputs_sorted.txt

# Prints date
date