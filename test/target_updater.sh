#!/bin/bash
# run simulations from model, put them in test_results, compare to target_results

# recreate targets with tests
rm -rf target_results.zip
# execute the for loop

# add to path
PATH=../Cell2Fire:$PATH

# run
set -x # enable debug tracing
for format in asc tif; do
    for model in fbp kitral sb; do
        echo running $model-$format
        output_folder=test_results/$model-$format
        mkdir -p $output_folder
        rm -rf $output_folder/*
        if [ "$model" == "fbp" ]; then
            additional_args="--cros"
            sim_code="C"
        elif [ "$model" == "sb" ]; then
            additional_args="--scenario 1"
            sim_code="S"
        elif [ "$model" == "kitral" ]; then
            additional_args=""
            sim_code="K"
        fi
        Cell2Fire$1 --input-instance-folder model/$model-$format --output-folder $output_folder --nsims 113 --output-messages --grids --out-ros --out-intensity --sim ${sim_code} --seed 123 --ignitionsLog $additional_args > test_results/$model-$format/log.txt
    done
done
set +x # disable debug tracing

mv test_results target_results
zip -r target_results.zip target_results
rm -rf target_results

