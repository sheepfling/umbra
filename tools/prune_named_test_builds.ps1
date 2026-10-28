param(
  [int]$MinimumAgeHours = 24,
  [switch]$Apply
)

$ErrorActionPreference = 'Stop'
if ($MinimumAgeHours -lt 1) {
  throw 'MinimumAgeHours must be at least 1.'
}

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'out/mgw'))
if (-not (Test-Path -LiteralPath $buildRoot -PathType Container)) {
  Write-Output "No MinGW build directory: $buildRoot"
  return
}

$cutoff = (Get-Date).AddHours(-$MinimumAgeHours)
$activeBuilds = Get-Process -Name cmake,ctest,ninja,g++,cc1plus,ld,collect2 -ErrorAction SilentlyContinue
if ($Apply -and $activeBuilds) {
  throw 'A build or test process is active; retry cleanup after it finishes.'
}
$candidates = Get-ChildItem -LiteralPath $buildRoot -Directory |
  Where-Object { $_.Name -match '^cpp-tck-named-[a-z0-9-]+$' -and $_.LastWriteTime -lt $cutoff }
foreach ($candidate in $candidates) {
  $resolved = [System.IO.Path]::GetFullPath($candidate.FullName)
  if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
      [System.IO.Path]::GetDirectoryName($resolved), $buildRoot)) {
    throw "Refusing path outside the intended MinGW build root: $resolved"
  }
  if ($candidate.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
    throw "Refusing reparse-point build directory: $resolved"
  }
  $links = Get-ChildItem -LiteralPath $resolved -Force -Recurse -Attributes ReparsePoint
  if ($links) {
    throw "Refusing build directory containing reparse points: $resolved"
  }
  $bytes = (Get-ChildItem -LiteralPath $resolved -Force -File -Recurse |
    Measure-Object -Property Length -Sum).Sum
  if ($null -eq $bytes) { $bytes = 0 }
  $gib = [math]::Round($bytes / 1GB, 2)
  if ($Apply) {
    Remove-Item -LiteralPath $resolved -Recurse -Force
    Write-Output "Removed $resolved ($gib GiB; generated build output, rebuildable)"
  } else {
    Write-Output "Would remove $resolved ($gib GiB; last write $($candidate.LastWriteTime))"
  }
}
if (-not $candidates) {
  Write-Output "No named-test build directories older than $MinimumAgeHours hours."
}
