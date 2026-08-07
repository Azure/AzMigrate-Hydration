<#
    asr-rp-ticker.ps1  -  ASR recovery-point cut-off validator (Windows guest)

    Writes one line every N seconds to a file on the REPLICATED DATA DISK and
    forces each line to physical disk (WriteThrough + FlushFileBuffers) so the
    LAST line in the file is guaranteed-durable at the crash-consistent cut-off.

    After you fail over (or test-fail over) to a chosen recovery point, open this
    file on the recovered VM and look at the LAST line: that timestamp is the
    exact point the data was cut off. With the fix in place it must be <= the
    recovery-point time. If it is meaningfully NEWER than the RP time, the RP
    over-shot (recovered more data than the point represents).

    Line format (CSV, no header):  <seq>,<utc-iso8601-ms>
      seq  = monotonic counter (detect gaps / the exact cut-off index)
      utc  = e.g. 2026-08-07T05:44:10.123Z  (UTC, matches portal RP times)

    USAGE (run on the SOURCE VM; elevation not required):
      # Pick a folder on the replicated DATA disk; file defaults to ticker.log:
      powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Folder F:\asr-validate
      # Or choose folder + file name explicitly:
      powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Folder F:\asr-validate -FileName ticker.log
      # Or give one full path (overrides -Folder/-FileName):
      powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Path F:\asr-validate\ticker.log
      # The folder MUST be on the replicated DATA disk (e.g. F:), NOT C: or the temp disk.
      # Leave it running. Stop with Ctrl+C (it flushes and closes cleanly).

    This script is provided as-is with no warranty (see repo LICENSE).
#>
param(
    [string]$Folder = 'F:\asr-validate',
    [string]$FileName = 'ticker.log',
    [string]$Path,
    [int]$IntervalSec = 10
)

$ErrorActionPreference = 'Stop'

if ($IntervalSec -lt 1) { throw "IntervalSec must be >= 1." }

# -Path (full path) wins; otherwise build it from -Folder + -FileName.
if (-not $Path) { $Path = Join-Path $Folder $FileName }

$dir = Split-Path -Parent $Path
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }

# Append + WriteThrough so every Flush($true) issues FlushFileBuffers (durable to disk).
$fs = [System.IO.FileStream]::new(
    $Path,
    [System.IO.FileMode]::Append,
    [System.IO.FileAccess]::Write,
    [System.IO.FileShare]::ReadWrite,
    4096,
    [System.IO.FileOptions]::WriteThrough)
$sw = [System.IO.StreamWriter]::new($fs)
$sw.AutoFlush = $false

$seq = 0
Write-Host "Ticker writing to $Path every $IntervalSec s. Press Ctrl+C to stop."
try {
    while ($true) {
        $seq++
        $utc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
        $sw.WriteLine(('{0},{1}' -f $seq, $utc))
        $sw.Flush()          # flush managed buffer to OS
        $fs.Flush($true)     # FlushFileBuffers -> durable to physical disk
        Start-Sleep -Seconds $IntervalSec
    }
}
finally {
    try { $sw.Flush(); $fs.Flush($true) } catch {}
    $sw.Dispose(); $fs.Dispose()
    Write-Host "Ticker stopped. Last seq = $seq. File: $Path"
}
