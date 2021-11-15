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
mpicc searching_MPI_0.c -O2 --std=gnu99 -o searching_MPI_0


for i in {1..3}
do
    NUM_PROCCESES=$(echo "2^$i" | bc)
    { time mpirun -np $NUM_PROCCESES searching_MPI_0 8; } 2>&1
done


# Prints date
date