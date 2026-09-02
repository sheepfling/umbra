param(
  [Parameter(Mandatory = $true)]
  [string] $TckExecutable,
  [Parameter(Mandatory = $true)]
  [string] $ProcessFixture,
  [string] $FomPath,
  [ValidateSet("evoked", "immediate")]
  [string] $CallbackModel = "evoked",
  [string] $ProviderId = "current-process",
  [int] $TimeoutMilliseconds = 5000
)

if (-not $FomPath) {
  $FomPath = Join-Path $PSScriptRoot "..\..\..\..\third_party\ieee1516.2-2025\resources\examples\RestaurantFOMmodule-2025.xml"
}

$markerDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("hla-rti-cpp-tck-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $markerDirectory | Out-Null
$server = $null
try {
  $server = Start-Process -FilePath $ProcessFixture `
    -ArgumentList @("public-server-loss", $markerDirectory) `
    -PassThru -WindowStyle Hidden

  $port = $null
  for ($attempt = 0; $attempt -lt 300 -and $null -eq $port; $attempt++) {
    $portPath = Join-Path $markerDirectory "port.txt"
    if (Test-Path -LiteralPath $portPath) {
      $parsed = 0
      $contents = (Get-Content -LiteralPath $portPath -Raw).Trim()
      if ([int]::TryParse($contents, [ref]$parsed) -and $parsed -gt 0 -and $parsed -le 65535) {
        $port = $parsed
      }
    }
    if ($null -eq $port) { Start-Sleep -Milliseconds 100 }
  }
  if ($null -eq $port) {
    throw "The provider loss fixture did not publish a listening port."
  }

  $arguments = @(
    "--fom", $FomPath,
    "--scenario", "cpp-tck.connection-loss-cleanup",
    "--callback-model", $CallbackModel,
    "--provider-id", $ProviderId,
    "--rti-address", "tcp://127.0.0.1:$port",
    "--federation-name", "process-execution",
    "--owner-name", "package-process-sender",
    "--member-name", "package-process-receiver",
    "--federate-type", "package-process-type",
    "--owner-configuration-name", "package-process-sender",
    "--member-configuration-name", "package-process-receiver",
    "--interaction-class", "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
    "--connection-loss-server-managed",
    "--connection-loss-marker", (Join-Path $markerDirectory "receiver-loss.ok"),
    "--timeout-ms", $TimeoutMilliseconds
  )
  & $TckExecutable @arguments
  $tckExitCode = $LASTEXITCODE
  if ($tckExitCode -ne 0) { exit $tckExitCode }

  $serverDeadline = (Get-Date).AddSeconds(30)
  while (-not $server.HasExited -and (Get-Date) -lt $serverDeadline) {
    Start-Sleep -Milliseconds 100
  }
  if (-not $server.HasExited) {
    throw "The provider loss fixture did not terminate."
  }
  if ($server.ExitCode -ne 0) {
    throw "The provider loss fixture returned exit code $($server.ExitCode)."
  }
} finally {
  if ($null -ne $server -and -not $server.HasExited) {
    Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
  }
  Remove-Item -LiteralPath $markerDirectory -Recurse -Force -ErrorAction SilentlyContinue
}
