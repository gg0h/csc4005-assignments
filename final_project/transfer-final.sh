#!/bin/bash

# cleanup

if [[ -f final.tar.gz ]] 
then
    echo "removing old tar file"
    rm final.tar.gz;
else
    echo "no existing tar files cleanup not performed";
fi

tar -czvf final.tar.gz execute_OMP execute_MPI small_inputs pattern_set jobscript_MPI.sh jobscript_OMP.sh project_MPI.c small_inputs_sorted.txt project_OMP.c

scp -i ~/csc4005/kelvin_id_rsa -P 55890 final.tar.gz 40173582@login.kelvin.alces.network:~/csc4005/assignments/final
