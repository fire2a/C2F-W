#!/bin/bash
# run simulations from model, put them in test_results, compare to target_results


#rm -rf kitral_target_results
unzip -q kitral_target_results.zip

# recreate targets with tests
# rm -rf target_results.zip
# execute the for loop
# mv test_results target_results
# zip -r target_results.zip target_results
# git add target_results.zip
# git commit -m "Update target results"
# git push

# add to path
PATH=../Cell2Fire:$PATH

# run
set -x # enable debug tracing
echo running Portillo
output_folder=kitral_test_results/portillo
mkdir -p $output_folder

Cell2Fire$1 --input-instance-folder ../data/Kitral/Portillo-asc/ --output-folder $output_folder --grids  --output-messages --ignitions --out-fl --out-intensity  --ignitionsLog  --out-  --sim K --nsims 50 --seed 24 --fmc 50 --weather rows > $output_folder/log.txt

set +x # disable debug tracing

# find difference between directories and files

# define the directories to compare
target="kitral_target_results"

# get the list of files in each directory
target_files=$(find ${target} -type f | sort)
target_num_files=$(ls -1 $target_files | wc -l)
# echo $target_files $target_num_files
output_folder_files=$(find ${output_folder} -type f | sort)
output_folder_num_files=$(ls -1 $output_folder_files | wc -l)
# echo $output_folder_files $output_folder_num_files

# check if the number of files in each directory is equal
if [ $target_num_files -ne $output_folder_num_files ]; then
    echo "Directories are not equal due to number"
    echo "Directory ${target} has ${target_num_files} files"
    echo "Directory ${output_folder} has ${output_folder_num_files} files"
    exit 1
fi
# delete version line
find $output_folder/ -name log.txt -type f | xargs sed -i -e '/version:/d'

# use diff to compare the files in each directory
diff_output=$(diff -rq "$target" "$output_folder")
# echo $diff_output

# check if there is any difference
if [ -z "$diff_output" ]; then
    echo "Directories are equal"
else
    echo "Directories are not equal due to differences"
    # compare file by file if fails show the file name
    for file1 in $(find "$target" -type f); do
        # find file1 in output_folder
        file2=$(echo $file1 | sed "s/${target}/${output_folder}/")
        # echo "Comparing $file1 and $file2"
        diff_output=$(diff "$file1" "$file2")
        if [ -n "$diff_output" ]; then
            echo "Files are not equal, aborting... $file1"
            echo $diff_output
            exit 1
        fi
    done
    #uncomment exit below to check all differences
    #exit 1
fi

rm -r kitral_target_results
rm -r $output_folder
exit 0
