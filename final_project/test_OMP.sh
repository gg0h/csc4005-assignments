./execute_OMP test_set
sort -k 1,1n -k 2,2n -k 3,3n result_OMP.txt > sorted_OMP.txt
diff sorted_OMP.txt test_set_sorted.txt
