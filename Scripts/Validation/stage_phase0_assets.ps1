param(
    [string]$CoreDllPath = "",
    [string]$KickstartRomPath = "",
    [string[]]$AdfPaths = @(),
    [switch]$RunReadinessCheck
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-OptionalPath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path (Get-Location) $Path
}

function Copy-IfProvided {
    param(
        [string]$Source,
        [string]$DestinationDir,
        [string]$Description,
        [string]$DestinationName = ""
    )

    if ([string]::IsNullOrWhiteSpace($Source)) {
        Write-Output "[SKIP] $Description - no source path provided"
        return
    }

    $resolvedSource = Resolve-OptionalPath -Path $Source
    if (-not (Test-Path $resolvedSource -PathType Leaf)) {
        throw "Source file not found for $Description: $resolvedSource"
    }

    if (-not (Test-Path $DestinationDir)) {
        New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    }

    $destinationPath = if ([string]::IsNullOrWhiteSpace($DestinationName)) {
        Join-Path $DestinationDir (Split-Path -Path $resolvedSource -Leaf)
    }
    else {
        Join-Path $DestinationDir $DestinationName
    }

    Copy-Item -Path $resolvedSource -Destination $destinationPath -Force
    Write-Output "[OK] $Description -> $destinationPath"
}

$root = Get-Location
$coreDestinationDir = Join-Path $root "Plugins/UnrealLibretro/MyCores"
$kickstartDestinationDir = Join-Path $root "EmulatorData/Kickstart"
$adfDestinationDir = Join-Path $root "EmulatorData/ADF"

Copy-IfProvided -Source $CoreDllPath -DestinationDir $coreDestinationDir -Description "PUAE core DLL"
Copy-IfProvided -Source $KickstartRomPath -DestinationDir $kickstartDestinationDir -Description "Kickstart ROM"

if ($AdfPaths.Count -eq 0) {
    Write-Output "[SKIP] ADF staging - no ADF paths provided"
}
else {
    if (-not (Test-Path $adfDestinationDir)) {
        New-Item -ItemType Directory -Path $adfDestinationDir -Force | Out-Null
    }

    foreach ($adfPath in $AdfPaths) {
        Copy-IfProvided -Source $adfPath -DestinationDir $adfDestinationDir -Description "ADF image"
    }
}

if ($RunReadinessCheck) {
    & .\Scripts\Validation\check_phase0_readiness.ps1
}
