param(
  [string]$PortName = "COM11"
)

$port = New-Object System.IO.Ports.SerialPort($PortName, 2400, "None", 7, "Two")
$port.Open()

$pkts = @(
  "0000 000000",
  "0001 057900",
  "0002 057400",
  "0003 057100",
  "0004 056900",
  "FF01 057900",
  "FF02 056900",
  "FF03 057325",
  "FF04 000435",
  "FF05 007500"
)

Write-Host "Dang gui chuoi gia lap DD60A qua $PortName (2400, 7-N-2)..." -ForegroundColor Cyan
foreach ($p in $pkts) {
  $frame = [byte[]]@(0x02) + [System.Text.Encoding]::ASCII.GetBytes($p) + [byte[]]@(0x03)
  $port.Write($frame, 0, $frame.Length)
  Write-Host "  -> [02] $p [03]" -ForegroundColor Green
  Start-Sleep -Milliseconds 80
}
$port.Close()
Write-Host "Da gui xong!" -ForegroundColor Yellow
