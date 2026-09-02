param(
  [Parameter(Mandatory = $true)]
  [string] $ApiIncludeDirectory,
  [string[]] $ProviderLibrary = @(),
  [string[]] $ProviderCompileDefinition = @(),
  [string[]] $ProviderLinkOption = @(),
  [string] $BuildDirectory,
  [ValidateSet("Debug", "Release")]
  [string] $Configuration = "Release",
  [switch] $Install
)

if (-not $BuildDirectory) {
  $BuildDirectory = Join-Path $PSScriptRoot "build"
}

function ConvertTo-CMakeList([string[]] $Values) {
  $items = @($Values | ForEach-Object { $_ -split "," })
  $normalized = foreach ($item in $items) {
    if (Test-Path -LiteralPath $item -PathType Leaf) {
      (Resolve-Path -LiteralPath $item).Path
    } else {
      $item
    }
  }
  return ($normalized -join ";")
}

$configureArguments = @(
  "-S", $PSScriptRoot,
  "-B", $BuildDirectory,
  "-DHLA_RTI_TCK_API_INCLUDE_DIR=$ApiIncludeDirectory"
)
if ($ProviderLibrary.Count -gt 0) {
  $configureArguments += "-DHLA_RTI_TCK_PROVIDER_LIBRARIES=$(ConvertTo-CMakeList $ProviderLibrary)"
}
if ($ProviderCompileDefinition.Count -gt 0) {
  $configureArguments += "-DHLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS=$(ConvertTo-CMakeList $ProviderCompileDefinition)"
}
if ($ProviderLinkOption.Count -gt 0) {
  $configureArguments += "-DHLA_RTI_TCK_PROVIDER_LINK_OPTIONS=$(ConvertTo-CMakeList $ProviderLinkOption)"
}

& cmake @configureArguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build $BuildDirectory --config $Configuration
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Install) {
  & cmake --install $BuildDirectory --config $Configuration
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
