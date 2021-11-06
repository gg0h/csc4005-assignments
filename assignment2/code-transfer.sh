#!/bin/bash

# cleanup

if [[ -f assignment2.tar.gz ]] 
then
    echo "removing old tar file"
    rm assignment2.tar.gz;
else
    echo "no existing tar files cleanup not performed";
fi

tar -czvf assignment2.tar.gz ./source 

scp -i ~/csc4005/kelvin_id_rsa -P 55890 assignment2.tar.gz 40173582@login.kelvin.alces.network:~/csc4005/assignments/assignment2
