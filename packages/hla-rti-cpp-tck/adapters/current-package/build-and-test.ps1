param(
  [Parameter(Mandatory = $true)]
  [string] $PackagePrefix,
  [string] $FomPath,
  [string] $ModelFomPath,
  [string] $DdmFomPath,
  [string] $MultiAttributeFomPath,
  [string] $ThreeDimensionalFomPath,
  [string] $MimFomPath,
  [string] $SwitchesFomPath,
  [string[]] $DdmDimension,
  [string[]] $ThreeDimensionalDimension,
  [string] $ThreeDimensionalObjectClass,
  [string] $ThreeDimensionalAttribute,
  [string[]] $AdditionalFomPath,
  [string[]] $InvalidFomPath,
  [ValidateSet("verified", "all")]
  [string] $ScenarioSet = "verified",
  [string] $TimeImplementation = "HLAinteger64Time",
  [int] $TimeoutMilliseconds = 10000,
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
if (-not $ModelFomPath) {
  $ModelFomPath = Join-Path $PSScriptRoot "..\..\fom\fom-model-tck.xml"
}
if (-not $DdmFomPath) {
  $DdmFomPath = Join-Path $PSScriptRoot "..\..\fom\ddm-tck.xml"
}
if (-not $MultiAttributeFomPath) {
  $MultiAttributeFomPath = Join-Path $PSScriptRoot "..\..\fom\ddm-multi-attribute-tck.xml"
}
if (-not $ThreeDimensionalFomPath) {
  $ThreeDimensionalFomPath = Join-Path $PSScriptRoot "..\..\fom\ddm-three-dimensional-tck.xml"
}
if (-not $MimFomPath) {
  $MimFomPath = Join-Path $PSScriptRoot "..\..\..\..\third_party\ieee1516.2-2025\resources\mim\HLAstandardMIM-2025.xml"
}
if (-not $SwitchesFomPath) {
  $SwitchesFomPath = Join-Path $PSScriptRoot "..\..\fom\switches-tck.xml"
}
if (-not $DdmDimension) {
  $DdmDimension = @("TckDimensionX", "TckDimensionY")
}
if (-not $ThreeDimensionalDimension) {
  $ThreeDimensionalDimension = @("TckDimensionX", "TckDimensionY", "TckDimensionZ")
}
if (-not $ThreeDimensionalObjectClass) {
  $ThreeDimensionalObjectClass = "HLAobjectRoot.TckThreeDimensionalObject"
}
if (-not $ThreeDimensionalAttribute) {
  $ThreeDimensionalAttribute = "Value"
}
if (-not $AdditionalFomPath) {
  $AdditionalFomPath = @(Join-Path $PSScriptRoot "..\..\fom\fom-extension-tck.xml")
}
if (-not $InvalidFomPath) {
  $InvalidFomPath = @(
    (Join-Path $PSScriptRoot "..\..\fom\invalid-malformed-tck.xml"),
    (Join-Path $PSScriptRoot "..\..\fom\invalid-namespace-tck.xml"),
    (Join-Path $PSScriptRoot "..\..\fom\invalid-duplicate-tck.xml")
  )
}
$PackagePrefix = (Resolve-Path -LiteralPath $PackagePrefix).Path
$FomPath = (Resolve-Path -LiteralPath $FomPath).Path
$ModelFomPath = (Resolve-Path -LiteralPath $ModelFomPath).Path
$DdmFomPath = (Resolve-Path -LiteralPath $DdmFomPath).Path
$MultiAttributeFomPath = (Resolve-Path -LiteralPath $MultiAttributeFomPath).Path
$ThreeDimensionalFomPath = (Resolve-Path -LiteralPath $ThreeDimensionalFomPath).Path
$MimFomPath = (Resolve-Path -LiteralPath $MimFomPath).Path
$SwitchesFomPath = (Resolve-Path -LiteralPath $SwitchesFomPath).Path
$AdditionalFomPath = @($AdditionalFomPath | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
$InvalidFomPath = @($InvalidFomPath | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })

$configureArguments = @(
  "-S", $PSScriptRoot,
  "-B", $BuildDirectory,
  "-DCMAKE_PREFIX_PATH=$PackagePrefix",
  "-DHLA_RTI_TCK_ADAPTER_FOM=$FomPath",
  "-DHLA_RTI_TCK_ADAPTER_MODEL_FOM=$ModelFomPath",
  "-DHLA_RTI_TCK_ADAPTER_DDM_FOM=$DdmFomPath",
  "-DHLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM=$MultiAttributeFomPath",
  "-DHLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM=$ThreeDimensionalFomPath",
  "-DHLA_RTI_TCK_ADAPTER_MIM_FOM=$MimFomPath",
  "-DHLA_RTI_TCK_ADAPTER_SWITCHES_FOM=$SwitchesFomPath",
  "-DHLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS=$($DdmDimension -join ';')",
  "-DHLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS=$($ThreeDimensionalDimension -join ';')",
  "-DHLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS=$ThreeDimensionalObjectClass",
  "-DHLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE=$ThreeDimensionalAttribute",
  "-DHLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES=$($AdditionalFomPath -join ';')",
  "-DHLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES=$($InvalidFomPath -join ';')",
  "-DHLA_RTI_TCK_ADAPTER_SCENARIO_SET=$ScenarioSet",
  "-DHLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION=$TimeImplementation",
  "-DHLA_RTI_TCK_ADAPTER_TIMEOUT_MS=$TimeoutMilliseconds",
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

& ctest --test-dir $BuildDirectory -C $Configuration -L "^portable-cpp-tck$" --output-on-failure
exit $LASTEXITCODE
