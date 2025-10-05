$port = New-Object System.IO.Ports.SerialPort('COM13', 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.Open()
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
while ($stopwatch.ElapsedMilliseconds -lt 5000) {
    if ($port.BytesToRead -gt 0) {
        $data = $port.ReadExisting()
        if ($data) {
            Write-Output $data
        }
    }
    Start-Sleep -Milliseconds 50
}
$port.Close()
