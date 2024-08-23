# Unzip the file
Expand-Archive -Path "target_results.zip" -DestinationPath "."

# Set PATH environment variable
$env:PATH = "../Cell2Fire;$env:PATH"

# Enable debug tracing
$DebugPreference = "Continue"

# Run simulations
foreach ($model in "fbp", "kitral", "sb") {
    Write-Output "running $model"
    $outputDir = "test_results/$model"
    New-Item -ItemType Directory -Force -Path $outputDir
    Remove-Item -Recurse -Force -Path "$outputDir/*"

    if ($model -eq "fbp") {
        $additionalArgs = "--cros"
        $simCode = "C"
    } elseif ($model -eq "sb") {
        $additionalArgs = "--scenario 1"
        $simCode = "S"
    } elseif ($model -eq "kitral") {
        $additionalArgs = ""
        $simCode = "K"
    }

    $command = "Cell2Fire.exe --input-instance-folder model/$model --output-folder $outputDir --nsims 113 --output-messages --grids --out-ros --out-intensity --sim $simCode --seed 123 $additionalArgs"
    Start-Process -FilePath "pwsh" -ArgumentList "-Command $command" -RedirectStandardOutput "$outputDir/log.txt" -NoNewWindow -Wait
}

# Disable debug tracing
$DebugPreference = "SilentlyContinue"

# find difference between directories and files

# define the directories to compare
$dir1 = "test_results"
$dir2 = "target_results"

# get the list of files in each directory
$dir1_files = Get-ChildItem -Path $dir1 -File | Sort-Object
$dir1_num_files = $dir1_files.Count
# echo $dir1_files $dir1_num_files
$dir2_files = Get-ChildItem -Path $dir2 -File | Sort-Object
$dir2_num_files = $dir2_files.Count
# echo $dir2_files $dir2_num_files

# check if the number of files in each directory is equal
if ($dir1_num_files -ne $dir2_num_files) {
    Write-Output "Directories are not equal due to number"
    Write-Output "Directory ${dir1} has ${dir1_num_files} files"
    Write-Output "Directory ${dir2} has ${dir2_num_files} files"
    exit 1
}

# use Compare-Object to compare the files in each directory
$diff_output = Compare-Object -ReferenceObject $dir1_files -DifferenceObject $dir2_files -Property Name, Length -PassThru
# echo $diff_output

# check if there is any difference
if (-not $diff_output) {
    Write-Output "Directories are equal"
} else {
    Write-Output "Directories are not equal due to differences"
    # compare file by file if fails show the file name
    foreach ($file1 in (Get-ChildItem -Path $dir1 -File -Recurse)) {
        # find file1 in dir2
        $file2 = $file1.FullName.Replace($dir1, $dir2)
        # echo "Comparing $file1 and $file2"
        $diff_output = Compare-Object -ReferenceObject (Get-Content $file1.FullName) -DifferenceObject (Get-Content $file2.FullName)
        if ($diff_output) {
            Write-Output "Files are not equal, $file1"
            Write-Output $diff_output
            # exit at first different
            Write-Output "run this command:"
            Write-Output "`trm -rf target_results`"
            Write-Output "run this command: before running the test again"
            exit 1
        }
    }
    #uncomment exit below to check all differences
    #exit 1
}

Remove-Item -Recurse -Force -Path "target_results"
exit 0
