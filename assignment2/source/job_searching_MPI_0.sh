#!/bin/bash

#SBATCH --job-name=40173582-MPI_0
#SBATCH -o mpi_0.%N.%j.out
#SBATCH -e mpi_0.%N.%j.err

# Ask the scheduler to run N MPI processes on N compute nodes
#SBATCH --nodes=8
#SBATCH --ntasks=8

# Load mpi module
module add mpi/openmpi

date

# Compiling the Program
mpicc program5.c -O2 --std=gnu99 -o program5

# Executes the compiled program on 4 MPI processes
mpirun -np 8 program5

# Prints date
date