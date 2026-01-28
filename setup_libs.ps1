$ErrorActionPreference = "Stop"

$raylibVersion = "5.0"
$raylibZip = "raylib-5.0_win64_mingw-w64.zip"
$url = "https://github.com/raysan5/raylib/releases/download/$raylibVersion/$raylibZip"
$outputDir = "lib"
$tempZip = "$outputDir\$raylibZip"

# Create lib directory
if (!(Test-Path -Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# Download Raylib
Write-Host "Downloading Raylib $raylibVersion..."
Invoke-WebRequest -Uri $url -OutFile $tempZip

# Extract
Write-Host "Extracting..."
Expand-Archive -Path $tempZip -DestinationPath $outputDir -Force

# Rename/Cleanup
$extractedFolder = "$outputDir\raylib-5.0_win64_mingw-w64"
$targetFolder = "$outputDir\raylib"

if (Test-Path -Path $targetFolder) {
    Remove-Item -Path $targetFolder -Recurse -Force
}
Rename-Item -Path $extractedFolder -NewName "raylib"

# Cleanup Zip
Remove-Item -Path $tempZip

Write-Host "Raylib setup complete at $targetFolder"
