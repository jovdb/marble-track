#!/usr/bin/env pwsh
# Deploy website script for marble-track project

# Store the original directory
$originalDir = Get-Location

try {
    Write-Host "Building website files..." -ForegroundColor Green
    
    # Change to website directory and build
    Set-Location ".\website"
    npm run build
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: Website build failed" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Website build successful!" -ForegroundColor Green
    
    # Return to project root for PlatformIO operations
    Set-Location $originalDir
    
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
    exit 1
}
finally {
    # Always return to original directory
    Set-Location $originalDir
}
