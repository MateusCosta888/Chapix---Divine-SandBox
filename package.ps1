Write-Host "Creating Divine Sandbox release package..." -ForegroundColor Cyan

# 1. Compile the game
Write-Host "Compiling project using build.bat..."
cmd.exe /c "build.bat"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed! Fix compilation errors before packaging." -ForegroundColor Red
    exit 1
}

# 2. Setup release directory
$releaseDir = "Divine_Sandbox_Release"
if (Test-Path $releaseDir) {
    Remove-Item -Recurse -Force $releaseDir
}

New-Item -ItemType Directory -Path $releaseDir | Out-Null

# 3. Copy executable
Write-Host "Copying executable..."
if (Test-Path "bin\game.exe") {
    Copy-Item "bin\game.exe" -Destination "$releaseDir\game.exe"
} else {
    Write-Host "Game executable not found in bin\game.exe!" -ForegroundColor Red
    exit 1
}

# 4. Copy assets directory
Write-Host "Copying assets folder..."
if (Test-Path "assets") {
    Copy-Item -Path "assets" -Destination "$releaseDir\assets" -Recurse
} else {
    Write-Host "Assets folder not found!" -ForegroundColor Red
    exit 1
}

# 5. Create ZIP Archive
Write-Host "Creating ZIP archive..."
$zipPath = "Divine_Sandbox_Release.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath
}
Compress-Archive -Path "$releaseDir\*" -DestinationPath $zipPath

Write-Host "Done! The game is successfully packaged." -ForegroundColor Green
Write-Host "Folder: $PWD\$releaseDir" -ForegroundColor Yellow
Write-Host "Zip Archive: $PWD\$zipPath" -ForegroundColor Yellow
Write-Host "You can now distribute the ZIP file or run game.exe directly from the release folder!" -ForegroundColor Cyan
