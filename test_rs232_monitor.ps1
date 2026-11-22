param(
  [string]$PortName = "COM11",
  [int]$BaudRate = 9600,
  [int]$DataBits = 7
)

$port = New-Object System.IO.Ports.SerialPort($PortName, $BaudRate, "None", $DataBits, "One")
$port.ReadTimeout = 200
$port.Open()

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " DANG LANG NGHE RS-232 TREN $PortName ($BaudRate, $DataBits-N-1)" -ForegroundColor Cyan
Write-Host " Hay bam 'LUU & IN THU' tren Web (Ctrl+C de thoat)" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor Cyan

try {
  while ($true) {
    if ($port.BytesToRead -gt 0) {
      $buf = New-Object byte[] $port.BytesToRead
      $count = $port.Read($buf, 0, $buf.Length)
      $txt = [System.Text.Encoding]::ASCII.GetString($buf, 0, $count)
      Write-Host -NoNewline $txt -ForegroundColor Green
    }
    Start-Sleep -Milliseconds 30
  }
} finally {
  if ($port.IsOpen) { $port.Close() }
  Write-Host "`nDa dong cong $PortName." -ForegroundColor Gray
}
