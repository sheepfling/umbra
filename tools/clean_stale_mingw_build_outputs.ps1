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
$codexWorktreesRoot = if ($env:USERPROFILE) {
  [System.IO.Path]::GetFullPath((Join-Path $env:USERPROFILE '.codex/worktrees'))
} else { $null }
$localWorktreesRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'out/worktrees'))
if (-not (Test-Path -LiteralPath $buildRoot -PathType Container)) {
  Write-Output "No MinGW build directory: $buildRoot"
  return
}

# Keep the reusable routine profile. Scenario-specific and focused-debug
# outputs expire after the retention window; their source and build presets
# remain available for a fresh configure/build.
$preservedBuilds = @('shared')
$cutoff = (Get-Date).AddHours(-$MinimumAgeHours)
$activeBuilds = Get-Process -ErrorAction SilentlyContinue | Where-Object {
  $_.ProcessName -match '^(cmake|ctest|ninja|g\+\+|cc1plus|ld|collect2|umbra_|hla_rti_)'
}
if ($Apply -and $activeBuilds) {
  $names = ($activeBuilds | Select-Object -ExpandProperty ProcessName -Unique) -join ', '
  throw "A build or test process is active ($names); retry cleanup after it finishes."
}

$candidates = [System.Collections.Generic.List[object]]::new()
$externalSourceTrees = 0
$generatedExtensions = @('.a', '.dll', '.exe', '.gcda', '.gcno', '.gcov', '.lib', '.o', '.obj', '.pdb', '.profraw', '.so', '.dylib')
foreach ($directory in Get-ChildItem -LiteralPath $buildRoot -Directory -Force) {
  if ($directory.Name -in $preservedBuilds) {
    continue
  }

  $resolved = [System.IO.Path]::GetFullPath($directory.FullName)
  if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
      [System.IO.Path]::GetDirectoryName($resolved), $buildRoot)) {
    throw "Refusing path outside the direct MinGW build root: $resolved"
  }
  if ($directory.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
    throw "Refusing reparse-point build directory: $resolved"
  }

  $cachePath = Join-Path $resolved 'CMakeCache.txt'
  $ninjaManifest = Join-Path $resolved 'build.ninja'
  if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf) -or
      -not (Test-Path -LiteralPath $ninjaManifest -PathType Leaf)) {
    continue
  }

  # Nested build outputs do not reliably update the direct child's timestamp.
  # Use the newest file in the tree so recent work is never aged out just
  # because its build directory was configured earlier.
  $treeFiles = @(Get-ChildItem -LiteralPath $resolved -File -Recurse -Force -ErrorAction Stop)
  if (-not $treeFiles.Count) { continue }
  $latestFileWrite = ($treeFiles | Measure-Object -Property LastWriteTime -Maximum).Maximum
  if ($latestFileWrite -ge $cutoff) { continue }

  $sourceLine = Get-Content -LiteralPath $cachePath | Where-Object {
    $_ -match '^CMAKE_HOME_DIRECTORY:INTERNAL='
  } | Select-Object -First 1
  if (-not $sourceLine) {
    throw "CMake source root is missing from $cachePath"
  }
  $sourceRoot = [System.IO.Path]::GetFullPath($sourceLine.Substring(
    'CMAKE_HOME_DIRECTORY:INTERNAL='.Length))
  $sourceIsLocal = [System.StringComparer]::OrdinalIgnoreCase.Equals(
    $sourceRoot, $repositoryRoot)
  foreach ($allowedRoot in @($repositoryRoot, $localWorktreesRoot, $codexWorktreesRoot)) {
    if ($allowedRoot -and $sourceRoot.StartsWith(
        $allowedRoot + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
      $sourceIsLocal = $true
    }
  }
  if (-not $sourceIsLocal) {
    $externalSourceTrees++
    continue
  }

  $generatedFiles = @($treeFiles |
    Where-Object { $_.Extension.ToLowerInvariant() -in $generatedExtensions })
  if (-not $generatedFiles.Count) {
    continue
  }
  $bytes = ($generatedFiles | Measure-Object -Property Length -Sum).Sum
  if ($null -eq $bytes) { $bytes = 0 }
  $candidates.Add([pscustomobject]@{
    Name = $directory.Name
    Path = $resolved
    Bytes = [long]$bytes
    LastWrite = $latestFileWrite
  })
}

if (-not $candidates.Count) {
  Write-Output "No stale in-repository MinGW CMake build outputs older than $MinimumAgeHours hours."
} else {
  $generatedBytes = ($candidates | Measure-Object -Property Bytes -Sum).Sum
  Write-Output ("Found {0} stale CMake build trees ({1:N2} GiB of generated artifacts); retention is {2} hours." -f
    $candidates.Count, ($generatedBytes / 1GB), $MinimumAgeHours)
}
if ($externalSourceTrees) {
  Write-Output "Skipped $externalSourceTrees trees configured from outside this checkout and its worktrees."
}
foreach ($candidate in $candidates | Sort-Object Bytes -Descending) {
  $size = [math]::Round($candidate.Bytes / 1GB, 2)
  if (-not $Apply) {
    Write-Output "Would clean $($candidate.Path) ($size GiB; last write $($candidate.LastWrite))"
    continue
  }

  & ninja -C $candidate.Path -t clean
  if ($LASTEXITCODE -ne 0) {
    throw "Ninja clean failed for $($candidate.Path); later candidates were not changed."
  }
  # Ninja cleans outputs still present in the current graph. Old CMake
  # reconfigures can leave object files from removed sources outside that
  # graph, so remove only known compiled-artifact extensions left in this
  # disposable build tree.
  $remainingArtifacts = @(Get-ChildItem -LiteralPath $candidate.Path -File -Recurse -Force -ErrorAction Stop |
    Where-Object { $_.Extension.ToLowerInvariant() -in $generatedExtensions })
  foreach ($artifact in $remainingArtifacts) {
    $artifactPath = [System.IO.Path]::GetFullPath($artifact.FullName)
    if (-not $artifactPath.StartsWith(
        $candidate.Path + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
      throw "Refusing generated artifact outside its build tree: $artifactPath"
    }
    if ($artifact.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
      throw "Refusing generated artifact reparse point: $artifactPath"
    }
  }
  $remainingBytes = ($remainingArtifacts | Measure-Object -Property Length -Sum).Sum
  foreach ($artifact in $remainingArtifacts) {
    Remove-Item -LiteralPath $artifact.FullName -Force
  }
  $remainingSize = [math]::Round($remainingBytes / 1GB, 3)
  Write-Output "Cleaned generated outputs from $($candidate.Path) ($size GiB estimated; $remainingSize GiB were orphaned files removed after Ninja clean)"
}

# Old installed-package prefixes and oversized one-off TCK drivers are also
# generated outputs. Preserve reports/source beside the drivers and preserve
# the build tree caches/dependencies handled above.
$prefixCandidates = [System.Collections.Generic.List[object]]::new()
foreach ($directory in Get-ChildItem -LiteralPath $buildRoot -Directory -Force) {
  if ($directory.Name -notmatch '(?i)(^|[-_])(install|provider)([-_]|$)') {
    continue
  }
  $resolved = [System.IO.Path]::GetFullPath($directory.FullName)
  if (-not [System.StringComparer]::OrdinalIgnoreCase.Equals(
      [System.IO.Path]::GetDirectoryName($resolved), $buildRoot)) {
    throw "Refusing package prefix outside the direct MinGW build root: $resolved"
  }
  if ($directory.Name -in $preservedBuilds -or
      (Test-Path -LiteralPath (Join-Path $resolved 'CMakeCache.txt')) -or
      (Test-Path -LiteralPath (Join-Path $resolved 'build.ninja'))) {
    continue
  }
  $files = @(Get-ChildItem -LiteralPath $resolved -File -Recurse -Force -ErrorAction Stop)
  if (-not $files.Count) { continue }
  if (@(Get-ChildItem -LiteralPath $resolved -Recurse -Force -ErrorAction Stop |
      Where-Object { $_.Attributes -band [System.IO.FileAttributes]::ReparsePoint }).Count) {
    throw "Refusing package prefix containing a reparse point: $resolved"
  }
  $hasUmbraLibrary = $files | Where-Object {
    $_.Name -match '(?i)^libumbra_rti\.(a|dll\.a|so|dylib)$'
  } | Select-Object -First 1
  if (-not $hasUmbraLibrary) { continue }
  $latestWrite = ($files | Measure-Object -Property LastWriteTime -Maximum).Maximum
  if ($latestWrite -ge $cutoff) { continue }
  $bytes = ($files | Measure-Object -Property Length -Sum).Sum
  $prefixCandidates.Add([pscustomobject]@{
    Name = $directory.Name
    Path = $resolved
    Bytes = [long]$bytes
    LastWrite = $latestWrite
  })
}

$prefixPaths = @($prefixCandidates | ForEach-Object { $_.Path })
$oneOffExecutables = [System.Collections.Generic.List[object]]::new()
foreach ($file in Get-ChildItem -LiteralPath $buildRoot -File -Recurse -Force -ErrorAction Stop |
    Where-Object { $_.Extension -ieq '.exe' -and $_.Length -gt 500MB -and $_.LastWriteTime -lt $cutoff }) {
  $resolvedFile = [System.IO.Path]::GetFullPath($file.FullName)
  if (-not $resolvedFile.StartsWith(
      $buildRoot + [System.IO.Path]::DirectorySeparatorChar,
      [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing executable outside the MinGW build root: $resolvedFile"
  }
  if ($file.Attributes -band [System.IO.FileAttributes]::ReparsePoint -or
      $resolvedFile.StartsWith(
        [System.IO.Path]::GetFullPath((Join-Path $buildRoot 'shared')) + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    continue
  }
  $isPackagePrefix = $false
  foreach ($prefixPath in $prefixPaths) {
    if ($resolvedFile.StartsWith($prefixPath + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
      $isPackagePrefix = $true
      break
    }
  }
  if ($isPackagePrefix) { continue }

  $parent = [System.IO.Path]::GetDirectoryName($resolvedFile)
  $insideCMakeTree = $false
  while ($parent -and $parent.StartsWith(
      $buildRoot + [System.IO.Path]::DirectorySeparatorChar,
      [System.StringComparison]::OrdinalIgnoreCase)) {
    if ((Test-Path -LiteralPath (Join-Path $parent 'CMakeCache.txt')) -or
        (Test-Path -LiteralPath (Join-Path $parent 'build.ninja'))) {
      $insideCMakeTree = $true
      break
    }
    $parent = [System.IO.Path]::GetDirectoryName($parent)
  }
  if (-not $insideCMakeTree) {
    $oneOffExecutables.Add([pscustomobject]@{
      Path = $resolvedFile
      Bytes = [long]$file.Length
      LastWrite = $file.LastWriteTime
    })
  }
}

if ($prefixCandidates.Count) {
  $bytes = ($prefixCandidates | Measure-Object -Property Bytes -Sum).Sum
  Write-Output ("Found {0} stale installed Umbra prefixes ({1:N2} GiB); only generated install/provider trees are eligible." -f
    $prefixCandidates.Count, ($bytes / 1GB))
  foreach ($candidate in $prefixCandidates | Sort-Object Bytes -Descending) {
    $size = [math]::Round($candidate.Bytes / 1GB, 2)
    if (-not $Apply) {
      Write-Output "Would remove $($candidate.Path) ($size GiB; latest content $($candidate.LastWrite))"
      continue
    }
    Remove-Item -LiteralPath $candidate.Path -Recurse -Force
    Write-Output "Removed stale installed prefix $($candidate.Path) ($size GiB)"
  }
} else {
  Write-Output "No stale installed Umbra prefixes older than $MinimumAgeHours hours."
}

if ($oneOffExecutables.Count) {
  $bytes = ($oneOffExecutables | Measure-Object -Property Bytes -Sum).Sum
  Write-Output ("Found {0} stale standalone executables over 500 MiB ({1:N2} GiB)." -f
    $oneOffExecutables.Count, ($bytes / 1GB))
  foreach ($candidate in $oneOffExecutables | Sort-Object Bytes -Descending) {
    $size = [math]::Round($candidate.Bytes / 1GB, 2)
    if (-not $Apply) {
      Write-Output "Would remove $($candidate.Path) ($size GiB; last write $($candidate.LastWrite))"
      continue
    }
    Remove-Item -LiteralPath $candidate.Path -Force
    Write-Output "Removed stale standalone executable $($candidate.Path) ($size GiB)"
  }
} else {
  Write-Output "No stale standalone executables over 500 MiB older than $MinimumAgeHours hours."
}
