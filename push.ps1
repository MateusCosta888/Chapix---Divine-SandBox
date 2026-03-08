$ErrorActionPreference = "Stop"
$source = "c:\Users\caioc\Downloads\Chapix---Divine-SandBox-main\Chapix---Divine-SandBox-main"
$dest = "$source\TempGitClone"

Write-Host "Cleaning up old files in repository..."
Get-ChildItem -Path $dest -Exclude ".git" | Remove-Item -Recurse -Force

Write-Host "Copying fresh files into repository..."
Copy-Item "$source\*" -Destination $dest -Recurse -Force -Exclude "TempGitClone", ".git", "Divine_Sandbox_Release", "Divine_Sandbox_Release.zip"

Set-Location $dest

Write-Host "Adding and committing to Git..."
git add .
git commit -m "feat(sandbox): major update to survival mechanics, ui, and performance

- Implemented Kingdom War system with soldier raids and sieges
- Added Cold & Snow Survival System (clothes requirement, damage over time)
- Added Hunting for Food behavior and Animal Reproduction (Food Chain) 
- Added Toolbar buttons to spawn Women and corrected overlapping spawn UI inputs
- Fixed Eraser tool to remove terrain AND buildings/entities correctly
- Implemented Incremental Loading Screen preventing UI freezes during heavy initial generation
- Added distribution script for pre-compiled releases"

Write-Host "Pushing to GitHub..."
git push origin main
Write-Host "Finished GitHub Push Script."
