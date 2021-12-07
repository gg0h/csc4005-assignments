./execute_OMP pattern_set
sort -k 1,1n -k 2,2n -k 3,3n result_OMP.txt > sorted_OMP.txt
diff sorted_OMP.txt pattern_set_sorted.txt
