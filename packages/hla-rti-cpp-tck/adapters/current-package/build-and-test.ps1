param(
  [Parameter(Mandatory = $true)]
  [string] $PackagePrefix,
  [string] $FomPath,
  [string] $BuildDirectory,
  [ValidateSet("both", "evoked", "immediate")]
  [string] $CallbackModel = "both",
  [string] $ProviderId = "installed-package",
  [string] $RtiAddress,
  [string] $RtiConfigurationName,
  [string] $AdditionalSettings,
  [ValidateSet("Debug", "Release")]
  [string] $Configuration = "Debug"
)

if (-not $BuildDirectory) {
  $BuildDirectory = Join-Path $PSScriptRoot "build"
}
if (-not $FomPath) {
  $FomPath = Join-Path $PSScriptRoot "..\..\fom\p0-tck.xml"
}
$PackagePrefix = (Resolve-Path -LiteralPath $PackagePrefix).Path
$FomPath = (Resolve-Path -LiteralPath $FomPath).Path

$configureArguments = @(
  "-S", $PSScriptRoot,
  "-B", $BuildDirectory,
  "-DCMAKE_PREFIX_PATH=$PackagePrefix",
  "-DHLA_RTI_TCK_ADAPTER_FOM=$FomPath",
  "-DHLA_RTI_TCK_ADAPTER_CALLBACK_MODEL=$CallbackModel",
  "-DHLA_RTI_TCK_ADAPTER_PROVIDER_ID=$ProviderId"
)
if ($RtiAddress) {
  $configureArguments += "-DHLA_RTI_TCK_ADAPTER_RTI_ADDRESS=$RtiAddress"
}
if ($RtiConfigurationName) {
  $configureArguments += "-DHLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME=$RtiConfigurationName"
}
if ($AdditionalSettings) {
  $configureArguments += "-DHLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS=$AdditionalSettings"
}

& cmake @configureArguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build $BuildDirectory --config $Configuration --parallel 2
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& ctest --test-dir $BuildDirectory -C $Configuration -R "^hla_rti_cpp_tck_installed_p0$" --output-on-failure
exit $LASTEXITCODE
