$ErrorActionPreference = "Stop"

$version = "1.22.0"
$zipName = "w64devkit-$version.zip"
$url = "https://github.com/skeeto/w64devkit/releases/download/v$version/$zipName"
$outputDir = "compiler"
$tempZip = "$outputDir\$zipName"

# Create compiler directory
if (!(Test-Path -Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# Download w64devkit if not present
if (!(Test-Path -Path "$outputDir\w64devkit")) {
    Write-Host "Downloading w64devkit (C++ Compiler) $version..."
    Invoke-WebRequest -Uri $url -OutFile $tempZip
    
    Write-Host "Extracting compiler..."
    Expand-Archive -Path $tempZip -DestinationPath $outputDir -Force
    
    # Cleanup
    Remove-Item -Path $tempZip
    Write-Host "Compiler setup complete."
} else {
    Write-Host "Compiler already exists in $outputDir\w64devkit"
}
