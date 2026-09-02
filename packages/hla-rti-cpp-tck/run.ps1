param(
  [Parameter(Mandatory = $true)]
  [string] $Executable,
  [string] $FomPath,
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
  [string] $InteractionClass,
  [string] $Parameter,
  [string] $TimeImplementation,
  [string] $ConnectionLossCommand,
  [string] $ConnectionLossMarker,
  [switch] $ConnectionLossServerManaged,
  [int] $TimeoutMilliseconds = 5000,
  [string] $Results,
  [string] $JUnit
)

if (-not $FomPath) {
  $FomPath = Join-Path $PSScriptRoot "fom/p0-tck.xml"
}

$arguments = @("--fom", $FomPath, "--callback-model", $CallbackModel,
  "--provider-id", $ProviderId, "--federation-prefix", $FederationPrefix,
  "--timeout-ms", $TimeoutMilliseconds)

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
if ($InteractionClass) { $arguments += @("--interaction-class", $InteractionClass) }
if ($Parameter) { $arguments += @("--parameter", $Parameter) }
if ($TimeImplementation) { $arguments += @("--time-implementation", $TimeImplementation) }
if ($ConnectionLossCommand) { $arguments += @("--connection-loss-command", $ConnectionLossCommand) }
if ($ConnectionLossMarker) { $arguments += @("--connection-loss-marker", $ConnectionLossMarker) }
if ($ConnectionLossServerManaged) { $arguments += "--connection-loss-server-managed" }
if ($Results) { $arguments += @("--results", $Results) }
if ($JUnit) { $arguments += @("--junit", $JUnit) }

& $Executable @arguments
exit $LASTEXITCODE
