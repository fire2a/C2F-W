#!/bin/bash
# generalized regression test script for multiple Cell2Fire runs

# unzip target results
unzip -q kitral_target_results.zip

# add Cell2Fire to PATH
PATH=../Cell2Fire:$PATH

# define runs: name|input_folder|extra_args
runs=(
    "portillo|../data/Kitral/Portillo-asc/|--nsims 1 --seed 24 --fmc 50 --ignitions"
    "villarrica|../data/Kitral/Villarrica-tif/|--nsims 10 --seed 42 --fmc 66 --FirebreakCells ../data/Kitral/Villarrica-tif/harvested_Random.csv"
    "biobio|../data/Kitral/biobio/|--nsims 3 --seed 5 --fmc 50 --cros --out-crown --out-cfb"
    # add more runs here following the same pattern
)

# loop over each run
for run in "${runs[@]}"; do
    IFS='|' read -r name input_folder extra_args <<< "$run"
    echo "Running simulation: $name"

    output_folder=kitral_test_results/$name
    mkdir -p "$output_folder"

    set -x # enable debug tracing
    Cell2Fire$1 --input-instance-folder "$input_folder" --output-folder "$output_folder" --grids --output-messages --out-fl --out-intensity --ignitionsLog --sim K --weather rows $extra_args > "$output_folder/log.txt"
    set +x # disable debug tracing

    # compare number of files
    target="kitral_target_results/$name"
    target_files=$(find "$target" -type f | sort)
    target_num_files=$(echo "$target_files" | wc -l)
    output_files=$(find "$output_folder" -type f | sort)
    output_num_files=$(echo "$output_files" | wc -l)

    if [ "$target_num_files" -ne "$output_num_files" ]; then
        echo "Mismatch in file count for $name"
        echo "Target: $target_num_files files"
        echo "Output: $output_num_files files"
        exit 1
    fi

    # clean version lines from logs
    find "$output_folder" -name log.txt -type f | xargs sed -i -e '/version:/d'

    # diff directories
    diff_output=$(diff -rq "$target" "$output_folder")
    if [ -z "$diff_output" ]; then
        echo "$name: Directories are equal"
    else
        echo "$name: Differences found"
        # detailed per-file check
        for file1 in $(find "$target" -type f); do
            file2=$(echo "$file1" | sed "s|$target|$output_folder|")
            diff_result=$(diff "$file1" "$file2")
            if [ -n "$diff_result" ]; then
                echo "Difference in file: $file1"
                echo "$diff_result"
                exit 1
            fi
        done
    fi

    # optionally clean up after each run
    rm -r "$output_folder"
done

# clean up target results
rm -r kitral_target_results

exit 0
