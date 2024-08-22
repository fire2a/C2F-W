# Unzip the file
Expand-Archive -Path "target_results.zip" -DestinationPath "."

# Set PATH environment variable
$env:PATH = "../Cell2FireC;$env:PATH"

# Enable debug tracing
Set-PSDebug -Trace 1

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

    $command = "Cell2Fire.Linux.x86_64 --input-instance-folder model/$model --output-folder $outputDir --nsims 113 --output-messages --grids --out-ros --out-intensity --sim $simCode --seed 123 $additionalArgs"
    Start-Process -FilePath "powershell" -ArgumentList "-Command $command" -RedirectStandardOutput "$outputDir/log.txt" -NoNewWindow -Wait
}

# Disable debug tracing
Set-PSDebug -Trace 0