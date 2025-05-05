# unzip target results
Expand-Archive -Path kitral_target_results.zip -DestinationPath kitral_target_results -Force

# add Cell2Fire to PATH
$env:PATH = "..\Cell2Fire;$env:PATH"

# define runs: name|input_folder|extra_args
$runs = @(
    @{ name = "portillo"; input = "..\data\Kitral\Portillo-asc\"; extra = "--nsims 1 --seed 24 --fmc 50 --ignitions" }
    @{ name = "villarrica"; input = "..\data\Kitral\Villarrica-tif\"; extra = "--nsims 10 --seed 42 --fmc 66 --FirebreakCells ..\data\Kitral\Villarrica-tif\harvested_Random.csv" }
    @{ name = "biobio"; input = "..\data\Kitral\biobio\"; extra = "--nsims 3 --seed 5 --fmc 50 --cros --out-crown --out-cfb" }
    # add more runs here
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
    & .\..\Cell2Fire\x64\Release\Cell2Fire.exe --input-instance-folder $inputFolder --output-folder $outputFolder --grids --output-messages --out-fl --out-intensity --ignitionsLog --sim K --weather rows $extraArgs *> $logFile
    (Get-Content $logFile) -replace '\\', '/' | Set-Content $logFile
    (Get-Content $logFile | Select-String -pattern 'version:' -notmatch) | Set-Content $logFile

    # compare number of files
    $target = "kitral_target_results\$name"
    $targetFiles = Get-ChildItem -Path $target -Recurse -File
    $outputFiles = Get-ChildItem -Path $outputFolder -Recurse -File

    if ($targetFiles.Count -ne $outputFiles.Count) {
        Write-Host "Mismatch in file count for $name"
        Write-Host "Target: $($targetFiles.Count) files"
        Write-Host "Output: $($outputFiles.Count) files"
        exit 1
    }


    # diff directories
    $diffOutput = Compare-Object `
        -ReferenceObject (Get-ChildItem $target -Recurse -File | ForEach-Object { Get-Content $_.FullName }) `
        -DifferenceObject (Get-ChildItem $outputFolder -Recurse -File | ForEach-Object { Get-Content $_.FullName })

    if ($diffOutput.Count -eq 0) {
        Write-Host "${name}: Directories are equal"
    }
    else {
        Write-Host "${name}: Differences found"
        foreach ($file1 in $targetFiles) {
            $relativePath = $file1.FullName.Substring($target.Length)
            $file2 = Join-Path $outputFolder $relativePath
            if (Test-Path $file2) {
                $diff = Compare-Object (Get-Content $file1.FullName) (Get-Content $file2)
                if ($diff) {
                    Write-Host "Difference in file: ${file1}"
                    $diff | Format-Table
                    exit 1
                }
            }
            else {
                Write-Host "File missing in output: $relativePath"
                exit 1
            }
        }
    }

    # optionally clean up after each run
    Remove-Item -Recurse -Force $outputFolder
}

# clean up target results
Remove-Item -Recurse -Force kitral_target_results

exit 0
