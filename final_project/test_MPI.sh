./execute_MPI pattern_set
sort -k 1,1n -k 2,2n -k 3,3n result_MPI.txt > sorted_MPI.txt
diff sorted_MPI.txt pattern_set_sorted.txt
