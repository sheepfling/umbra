param(
  [Parameter(Mandatory = $true)]
  [string] $Executable,
  [string] $FomPath,
  [string] $ModelFomPath,
  [string] $DdmFomPath,
  [string] $MultiAttributeFomPath,
  [string] $ThreeDimensionalFomPath,
  [string] $MimFomPath,
  [string] $SwitchesFomPath,
  [string[]] $DdmDimension = @(),
  [string[]] $ThreeDimensionalDimension = @(),
  [string[]] $AdditionalFomPath = @(),
  [string[]] $InvalidFomPath = @(),
  [string[]] $Scenario = @(),
  [ValidateSet("both", "evoked", "immediate")]
  [string] $CallbackModel = "both",
  [string] $ProviderId = "unspecified",
  [string] $RtiAddress,
  [string] $ConfigurationName,
  [string] $AdditionalSettings,
  [string] $FederationPrefix = "cpp-tck",
  [string] $FederationName,
  [string] $OwnerName,
  [string] $MemberName,
  [string] $FederateType,
  [string] $OwnerConfigurationName,
  [string] $MemberConfigurationName,
  [string] $ObjectClass,
  [string] $Attribute,
  [string] $ThreeDimensionalObjectClass,
  [string] $ThreeDimensionalAttribute,
  [string] $InteractionClass,
  [string] $Parameter,
  [string] $TimeImplementation,
  [string] $ConnectionLossMarker,
  [switch] $ConnectionLossServerManaged,
  [int] $TimeoutMilliseconds = 5000,
  [string] $Results,
  [string] $JUnit
)

if (-not $FomPath) {
  $FomPath = Join-Path $PSScriptRoot "fom/p0-tck.xml"
}
if (-not $DdmFomPath) {
  $DdmFomPath = Join-Path $PSScriptRoot "fom/ddm-tck.xml"
}
if (-not $MultiAttributeFomPath) {
  $MultiAttributeFomPath = Join-Path $PSScriptRoot "fom/ddm-multi-attribute-tck.xml"
}
if (-not $ThreeDimensionalFomPath) {
  $ThreeDimensionalFomPath = Join-Path $PSScriptRoot "fom/ddm-three-dimensional-tck.xml"
}
if (-not $SwitchesFomPath) {
  $SwitchesFomPath = Join-Path $PSScriptRoot "fom/switches-tck.xml"
}
if (-not $DdmDimension) {
  $DdmDimension = @("TckDimensionX", "TckDimensionY")
}
if (-not $ThreeDimensionalDimension) {
  $ThreeDimensionalDimension = @("TckDimensionX", "TckDimensionY", "TckDimensionZ")
}

$arguments = @("--fom", $FomPath, "--callback-model", $CallbackModel,
  "--provider-id", $ProviderId, "--federation-prefix", $FederationPrefix,
  "--timeout-ms", $TimeoutMilliseconds)

if ($ModelFomPath) { $arguments += @("--model-fom", $ModelFomPath) }
if ($DdmFomPath) { $arguments += @("--ddm-fom", $DdmFomPath) }
if ($MultiAttributeFomPath) { $arguments += @("--multi-attribute-fom", $MultiAttributeFomPath) }
if ($ThreeDimensionalFomPath) { $arguments += @("--three-dimensional-fom", $ThreeDimensionalFomPath) }
if ($MimFomPath) { $arguments += @("--mim-fom", $MimFomPath) }
if ($SwitchesFomPath) { $arguments += @("--switches-fom", $SwitchesFomPath) }
foreach ($dimension in $DdmDimension) { $arguments += @("--ddm-dimension", $dimension) }
foreach ($dimension in $ThreeDimensionalDimension) {
  $arguments += @("--three-dimensional-dimension", $dimension)
}
foreach ($path in $AdditionalFomPath) { $arguments += @("--additional-fom", $path) }
foreach ($path in $InvalidFomPath) { $arguments += @("--invalid-fom", $path) }

foreach ($value in $Scenario) { $arguments += @("--scenario", $value) }
if ($RtiAddress) { $arguments += @("--rti-address", $RtiAddress) }
if ($ConfigurationName) { $arguments += @("--configuration-name", $ConfigurationName) }
if ($AdditionalSettings) { $arguments += @("--additional-settings", $AdditionalSettings) }
if ($FederationName) { $arguments += @("--federation-name", $FederationName) }
if ($OwnerName) { $arguments += @("--owner-name", $OwnerName) }
if ($MemberName) { $arguments += @("--member-name", $MemberName) }
if ($FederateType) { $arguments += @("--federate-type", $FederateType) }
if ($OwnerConfigurationName) { $arguments += @("--owner-configuration-name", $OwnerConfigurationName) }
if ($MemberConfigurationName) { $arguments += @("--member-configuration-name", $MemberConfigurationName) }
if ($ObjectClass) { $arguments += @("--object-class", $ObjectClass) }
if ($Attribute) { $arguments += @("--attribute", $Attribute) }
if ($ThreeDimensionalObjectClass) {
  $arguments += @("--three-dimensional-object-class", $ThreeDimensionalObjectClass)
}
if ($ThreeDimensionalAttribute) {
  $arguments += @("--three-dimensional-attribute", $ThreeDimensionalAttribute)
}
if ($InteractionClass) { $arguments += @("--interaction-class", $InteractionClass) }
if ($Parameter) { $arguments += @("--parameter", $Parameter) }
if ($TimeImplementation) { $arguments += @("--time-implementation", $TimeImplementation) }
if ($ConnectionLossMarker) { $arguments += @("--connection-loss-marker", $ConnectionLossMarker) }
if ($ConnectionLossServerManaged) { $arguments += "--connection-loss-server-managed" }
if ($Results) { $arguments += @("--results", $Results) }
if ($JUnit) { $arguments += @("--junit", $JUnit) }

& $Executable @arguments
exit $LASTEXITCODE
