# Quick filesystem upload for ESP32-S3
$platformioPath = "C:\Users\vandenberghej\.platformio\penv\Scripts\platformio.exe"

Write-Host "Uploading data folder..." -ForegroundColor Green
& $platformioPath run --target uploadfs

if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: Upload complete!" -ForegroundColor Green
} else {
    Write-Host "FAILED: Upload failed" -ForegroundColor Red
}
