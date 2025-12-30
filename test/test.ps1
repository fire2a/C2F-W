# Accept an optional first (positional) parameter for the executable to run.
# If none is supplied, use the repository default used by CI:
param(
    [string]$Exe = ".\..\Cell2Fire\x64\Release\Cell2Fire.exe"
)
Write-Output "Using executable: $Exe"

# Unzip target results
Expand-Archive -Path "target_results.zip" -DestinationPath "."

$DebugPreference = "SilentlyContinue"
$WarningPreference = "SilentlyContinue"

# Run simulations
foreach ($format in "asc", "tif") {
    foreach ($model in "fbp", "kitral", "sb", "portugal") {
        Write-Output "Running $model-$format"
        $outputDir = "test_results/$model-$format"

        # Create (or clean) output folder
        if (Test-Path $outputDir) {
            Get-ChildItem -Path $outputDir -Recurse -Force | Remove-Item -Force
        } else {
            New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
        }

        # Determine arguments based on model
        switch ($model) {
            "fbp" {
                $additionalArgs = "--cros"
                $simCode = "C"
            }
            "sb" {
                $additionalArgs = "--scenario 1"
                $simCode = "S"
            }
            "portugal" {
                $additionalArgs = "--scenario 1"
                $simCode = "P"
            }
            "kitral" {
                $additionalArgs = ""
                $simCode = "K"
            }
        }

        # Build argument list and run simulation
        $cmdArgs = @(
            "--input-instance-folder", "model/$model-$format",
            "--output-folder", $outputDir,
            "--nsims", "113",
            "--output-messages", "--grids", "--out-intensity", "--ignitionsLog",
            "--sim", $simCode,
            "--seed", "123"
        )
        if ($additionalArgs) {
            $cmdArgs += $additionalArgs.Split(" ")
        }
        $logFile = "$outputDir/log.txt"

        # Run the simulation and tee the output to a log file
        & $Exe @cmdArgs *> $logFile 2>$null
            (Get-Content $logFile) -replace '\\', '/' | Set-Content $logFile
            (Get-Content $logFile | Select-String -pattern 'version:' -notmatch) | Set-Content $logFile
            (Get-Content "$outputDir/ignition_and_weather_log.csv") -replace '\\', '/' | Set-Content "$outputDir/ignition_and_weather_log.csv"
        }
}

# Define directories to compare
$dir1 = "test_results"
$dir2 = "target_results"

# Recursively get all files in each directory
$dir1_files = Get-ChildItem -Path $dir1 -Recurse -File
$dir2_files = Get-ChildItem -Path $dir2 -Recurse -File

# Count and compare number of files
$dir1_num_files = $dir1_files.Count
$dir2_num_files = $dir2_files.Count

if ($dir1_num_files -ne $dir2_num_files) {
    Write-Output " Directories have different file counts:"
    Write-Output "  ${dir1}: ${dir1_num_files} files"
    Write-Output "  ${dir2}: ${dir2_num_files} files"
    exit 1
}

# Compare file contents
foreach ($file1 in $dir1_files) {
    $file2_path = $file1.FullName.Replace($dir1, $dir2)

    if (-not (Test-Path $file2_path)) {
        Write-Output "Missing file in target: $file2_path"
        exit 1
    }

    $diff = Compare-Object -ReferenceObject @((Get-Content $file1.FullName)|Select-Object) -DifferenceObject @((Get-Content $file2_path)|Select-Object)
    if ($diff) {
        Write-Output " Files differ: $($file1.FullName)"
        Write-Output $diff
        Write-Output "`nIf this is expected, you can update targets with:"
        Write-Output "Remove-Item -Recurse -Force target_results"
        Write-Output "Rename-Item test_results target_results"
        Write-Output "Compress-Archive target_results target_results.zip -Force"
        exit 1
    }
}
Write-Output "All files match!"

# Cleanup
Remove-Item -Recurse -Force "target_results"
Remove-Item -Recurse -Force "test_results"

exit 0
