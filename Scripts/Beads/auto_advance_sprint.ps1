param(
    [string]$Label = "sprint-1",
    [int]$Take = 1,
    [object]$ReclaimAssigned = $true,
    [switch]$DryRun,
    [string]$LogPath = "DOCS\\logs\\beads_auto_advance.log"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:ResolvedLogPath = if ([System.IO.Path]::IsPathRooted($LogPath)) {
    $LogPath
}
else {
    Join-Path (Get-Location) $LogPath
}

$script:LogDir = Split-Path -Path $script:ResolvedLogPath -Parent
if ($script:LogDir -and -not (Test-Path $script:LogDir)) {
    New-Item -ItemType Directory -Path $script:LogDir -Force | Out-Null
}

function Write-RunLog {
    param(
        [string]$Message,
        [string]$Level = "INFO"
    )

    $timestamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
    $line = "[$timestamp] [$Level] label=$Label take=$Take dryRun=$($DryRun.IsPresent) :: $Message"
    Add-Content -Path $script:ResolvedLogPath -Value $line
}

function Invoke-Bd {
    param([string[]]$CliArgs)

    $hasNativeSetting = $null -ne (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue)
    if ($hasNativeSetting) {
        $previousNativeSetting = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
    }

    try {
        $output = & npx -y @beads/bd @CliArgs 2>&1
        $code = $LASTEXITCODE
    }
    finally {
        if ($hasNativeSetting) {
            $PSNativeCommandUseErrorActionPreference = $previousNativeSetting
        }
    }

    return [pscustomobject]@{
        ExitCode = $code
        Output = ($output -join "`n")
    }
}

function Convert-ToBoolean {
    param(
        [object]$Value,
        [bool]$Default = $true
    )

    if ($null -eq $Value) {
        return $Default
    }

    if ($Value -is [bool]) {
        return [bool]$Value
    }

    $text = $Value.ToString().Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $Default
    }

    switch -Regex ($text.ToLowerInvariant()) {
        '^(1|true|yes|y|on)$' { return $true }
        '^(0|false|no|n|off)$' { return $false }
        default {
            throw "Invalid boolean value '$text' for ReclaimAssigned. Use true/false."
        }
    }
}

function Get-ReadyIssues {
    param([string]$SprintLabel)

    $result = Invoke-Bd -CliArgs @("ready", "--label", $SprintLabel, "--limit", "0", "--json")
    if ($result.ExitCode -ne 0) {
        throw "Failed to query ready issues: $($result.Output)"
    }

    $text = $result.Output.Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
        return @()
    }

    try {
        $parsed = $text | ConvertFrom-Json
    }
    catch {
        throw "Failed to parse ready JSON output: $text"
    }

    if ($null -eq $parsed) {
        return @()
    }

    return @($parsed)
}

function Invoke-ClaimIssue {
    param(
        [string]$IssueId,
        [bool]$AllowReclaim
    )

    $claim = Invoke-Bd -CliArgs @("update", $IssueId, "--claim")
    if ($claim.ExitCode -eq 0) {
        return [pscustomobject]@{ Success = $true; Message = "Claimed" }
    }

    if ($AllowReclaim -and $claim.Output -match "already claimed by") {
        $clear = Invoke-Bd -CliArgs @("assign", $IssueId, "")
        if ($clear.ExitCode -ne 0) {
            return [pscustomobject]@{ Success = $false; Message = "Failed to clear assignee: $($clear.Output)" }
        }

        $reclaim = Invoke-Bd -CliArgs @("update", $IssueId, "--claim")
        if ($reclaim.ExitCode -eq 0) {
            return [pscustomobject]@{ Success = $true; Message = "Reclaimed from existing assignee" }
        }

        return [pscustomobject]@{ Success = $false; Message = "Failed reclaim after clear: $($reclaim.Output)" }
    }

    return [pscustomobject]@{ Success = $false; Message = $claim.Output }
}

try {
    $AllowReclaimAssigned = Convert-ToBoolean -Value $ReclaimAssigned -Default $true

    Write-RunLog -Message "Run started."

    $ready = @(Get-ReadyIssues -SprintLabel $Label)
    if ($ready.Count -eq 0) {
        Write-Output "No ready issues found for label '$Label'."
        Write-RunLog -Message "No ready issues found."
        exit 0
    }

    $targetCount = [Math]::Min($Take, $ready.Count)
    Write-Output "Found $($ready.Count) ready issue(s); attempting to start up to $targetCount."
    Write-RunLog -Message "Found $($ready.Count) ready issue(s); target start count $targetCount."

    $startedCount = 0
    $attemptedCount = 0
    foreach ($issue in $ready) {
        if ($startedCount -ge $Take) {
            break
        }

        $attemptedCount += 1
        $id = $issue.id
        $title = $issue.title

        if ($DryRun) {
            $dryMsg = "[DryRun] Would claim and start: $id - $title"
            Write-Output $dryMsg
            Write-RunLog -Message $dryMsg
            continue
        }

        $claimed = Invoke-ClaimIssue -IssueId $id -AllowReclaim $AllowReclaimAssigned
        if (-not $claimed.Success) {
            $skipMsg = "Skipped $id - unable to claim: $($claimed.Message)"
            Write-Output $skipMsg
            Write-RunLog -Message $skipMsg -Level "WARN"
            continue
        }

        $stamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
        $note = "Auto-advance kickoff at $stamp. Claimed and set to in_progress because dependencies were clear for label '$Label'."
        $noted = Invoke-Bd -CliArgs @("note", $id, $note)
        if ($noted.ExitCode -ne 0) {
            $noteErr = "Claimed $id but failed to write note: $($noted.Output)"
            Write-Output $noteErr
            Write-RunLog -Message $noteErr -Level "WARN"
            continue
        }

        $startedCount += 1
        $startMsg = "Started $id - $title ($($claimed.Message))"
        Write-Output $startMsg
        Write-RunLog -Message $startMsg
    }

    Write-RunLog -Message "Run completed. started=$startedCount attempted=$attemptedCount target=$Take ready=$($ready.Count)."
}
catch {
    Write-RunLog -Message "Run failed: $($_.Exception.Message)" -Level "ERROR"
    throw
}
