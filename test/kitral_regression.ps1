# unzip target results
Expand-Archive -Path kitral_target_results.zip -DestinationPath "."

# add Cell2Fire to PATH
$env:PATH = "..\Cell2Fire;$env:PATH"

# define runs: name|input_folder|extra_args
$runs = @(
    @{ name = "portillo"; input = "..\data\Kitral\Portillo-asc\"; extra = @("--nsims", "1", "--seed", "24", "--fmc", "50", "--ignitions", "--cros", "--out-crown", "--out-cfb") }
    @{ name = "villarrica"; input = "..\data\Kitral\Villarrica-tif\"; extra = @("--nsims", "10", "--seed", "42", "--fmc", "66", "--FirebreakCells", "..\data\Kitral\Villarrica-tif\harvested_Random.csv") }
    #@{ name = "biobio"; input = "..\data\Kitral\biobio\"; extra = @("--nsims", "3", "--seed", "5", "--fmc", "50", "--cros", "--out-crown", "--out-cfb") }
    @{ name = "portezuelo"; input = "..\data\Kitral\Portillo-asc\"; extra = @("--nsims", "1", "--seed", "5", "--fmc", "50", "--ignitions") }
)

foreach ($run in $runs) {
    $name = $run.name
    $inputFolder = $run.input
    $extraArgs = $run.extra
    Write-Host "Running simulation: $name"

    $outputFolder = "kitral_test_results\$name"
    New-Item -ItemType Directory -Force -Path $outputFolder | Out-Null

    $logFile = "$outputFolder/log.txt"
    # enable debug tracing
    Write-Host "Executing Cell2Fire..."
    & .\..\Cell2Fire\x64\Release\Cell2Fire.exe --input-instance-folder $inputFolder --output-folder $outputFolder --output-messages --out-fl --out-intensity --ignitionsLog --sim K --weather rows --grids $extraArgs *> $logFile 2>$null
    (Get-Content $logFile) -replace '\\', '/' | Set-Content $logFile
    (Get-Content $logFile | Select-String -pattern 'version:' -notmatch) | Set-Content $logFile

    # compare number of files
    $target = "kitral_target_results\$name"
    # Recursively get all files in each directory
$dir1_files = Get-ChildItem -Path $target -Recurse -File
$dir2_files = Get-ChildItem -Path $outputFolder -Recurse -File

# Count and compare number of files
$dir1_num_files = $dir1_files.Count
$dir2_num_files = $dir2_files.Count

if ($dir1_num_files -ne $dir2_num_files) {
    Write-Output " Directories have different file counts:"
    Write-Output "  ${target}: ${dir1_num_files} files"
    Write-Output "  ${outputFolder}: ${dir2_num_files} files"
    exit 1
}

# Compare file contents
foreach ($file1 in $dir1_files) {
    $file2_path = $file1.FullName.Replace($target, $outputFolder)

    if (-not (Test-Path $file2_path)) {
        Write-Output "Missing file in output: $file2_path"
        exit 1
    }

    $diff = Compare-Object -ReferenceObject @((Get-Content $file1.FullName)|Select-Object) -DifferenceObject @((Get-Content $file2_path)|Select-Object)
    if ($diff) {
        Write-Output " Files differ: $($file1.FullName)"
        Write-Output $diff
        Write-Output "`nIf this is expected, you can update targets with:"
        Write-Output "Remove-Item -Recurse -Force kitral_target_results"
        Write-Output "Rename-Item test_results kitral_target_results"
        Write-Output "Compress-Archive kitral_target_results kitral_target_results.zip -Force"
        exit 1
    }
}


    # optionally clean up after each run
    Remove-Item -Recurse -Force $outputFolder
}

# clean up target results
Remove-Item -Recurse -Force kitral_target_results

exit 0
