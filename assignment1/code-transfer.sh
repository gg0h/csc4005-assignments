#!/bin/bash

# cleanup

if [[ -f assignment1.tar.gz ]] 
then
    rm assignment1.tar.gz;
else
    echo "no existing tar files cleanup not performed";
fi

tar -czvf assignment1.tar.gz Assignment1a-dir/ 

scp -i ~/csc4005/kelvin_id_rsa -P 55890 assignment1.tar.gz 40173582@login.kelvin.alces.network:~/csc4005/assignments/
