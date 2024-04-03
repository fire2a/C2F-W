#!/bin/bash
cd ../Cell2FireC
pwd

#RUN CANADIAN FBP INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/FBP_Instance --output-folder ../test/test_results/FBP --sim-years 1 --nsims 100 --output-messages --grids --out-ros --out-intensity --sim C --seed 123 # 

#RUN KITRAL INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/Kitral_Instance --output-folder ../test/test_results/Kitral --sim-years 1 --nsims 100 --output-messages --out-ros --out-intensity --grids --sim K -seed 123

#RUN SCOTT AND BURGAN INSTANCE
./Cell2Fire.Linux.x86_64 --input-instance-folder ../test/SB_Instance --output-folder ../test/test_results/SB --sim-years 1 --nsims 100 --output-messages --out-ros --out-intensity --grids --sim S --seed 123


#FIND DIFFERENCE BETWEEN FILES
#for filename in ../test/test_results/*;
#do
#echo $filename
#done

# Define the directories to compare
dir1="../test/test_results"
dir2="../test/results"

# Get the list of files in each directory
dir1_files=$(find "$dir1" -type f | sort)
dir2_files=$(find "$dir2" -type f | sort)

# Check if the number of files in each directory is equal
if [ "$(echo "$dir1_files" | wc -l)" -ne "$(echo "$dir2_files" | wc -l)" ]; then
    echo "Directories are not equal due to number"
    exit 1
fi

# Use diff to compare the files in each directory
diff_output=$(diff -rq "$dir1" "$dir2")

# Check if there is any difference
if [ -z "$diff_output" ]; then
    echo "Directories are equal"
else
    echo "Directories are not equal due to differences"
fi