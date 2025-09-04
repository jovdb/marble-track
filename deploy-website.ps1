#!/usr/bin/env pwsh
# Deploy website script for marble-track project

# Store the original directory
$originalDir = Get-Location

try {

     # Remove previous ESP32 filesystem files
    Write-Host "Clearing previous files..." -ForegroundColor Yellow
    Get-ChildItem ".\esp32_ws\data" -Recurse -File | Remove-Item -Force

    Write-Host "Building website files..." -ForegroundColor Green
    
    # Change to website directory
    Set-Location ".\website"
    
    # Temporarily rename .env file to prevent it from being included in build
    $envFile = ".\.env"
    $envBackup = ".\.env.backup"
    $envRenamed = $false
    
    if (Test-Path $envFile) {
        Write-Host "Temporarily renaming .env file..." -ForegroundColor Yellow
        Rename-Item $envFile $envBackup
        $envRenamed = $true
    }
    
    # Build the website
    npm run build
    
    # Restore .env file immediately after build
    if ($envRenamed) {
        Write-Host "Restoring .env file..." -ForegroundColor Yellow
        Rename-Item $envBackup $envFile
        $envRenamed = $false
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: Website build failed" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Website build successful!" -ForegroundColor Green
    
    # Return to project root for PlatformIO operations
    Set-Location $originalDir
    
    # Clean up any .env files that might have been copied to data directory
    Write-Host "Cleaning up .env files from data directory..." -ForegroundColor Yellow
    Get-ChildItem ".\esp32_ws\data" -Recurse -Force | Where-Object { $_.Name -like "*.env*" } | Remove-Item -Force

    # Quick filesystem upload for ESP32-S3
    $platformioPath = "C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe"

    Write-Host "Uploading data folder to ESP32..." -ForegroundColor Green
    & $platformioPath run --target uploadfs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "SUCCESS: Upload complete!" -ForegroundColor Green
    }
    else {
        Write-Host "FAILED: Upload failed" -ForegroundColor Red
        exit 1
    }
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    
    # Restore .env file if it was renamed
    if ($envRenamed -and (Test-Path ".\website\.env.backup")) {
        Write-Host "Restoring .env file after error..." -ForegroundColor Yellow
        Set-Location ".\website"
        Rename-Item ".\.env.backup" ".\.env"
        Set-Location $originalDir
    }
    
    exit 1
}
finally {
    # Always return to original directory and ensure .env is restored
    if ($envRenamed -and (Test-Path ".\website\.env.backup")) {
        Write-Host "Final .env file restoration..." -ForegroundColor Yellow
        Set-Location ".\website"
        Rename-Item ".\.env.backup" ".\.env"
    }
    Set-Location $originalDir
}
