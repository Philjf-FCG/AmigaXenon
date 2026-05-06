Set-Location "c:\DevProjects\AmigaXenon"

function Add-Dep {
    param(
        [string]$Blocked,
        [string]$Blocker
    )

    $result = & npx -y @beads/bd dep add $Blocked $Blocker 2>&1
    if ($LASTEXITCODE -ne 0) {
        $text = $result -join "`n"
        if ($text -match "already" -or $text -match "exists") {
            Write-Output "SKIP_DEP_EXISTS $Blocked <- $Blocker"
        }
        else {
            throw "Failed dependency: $Blocked depends on $Blocker`n$text"
        }
    }
    else {
        Write-Output "ADDED_DEP $Blocked <- $Blocker"
    }
}

function Set-Owner {
    param(
        [string]$Issue,
        [string]$Owner
    )

    $result = & npx -y @beads/bd assign $Issue $Owner 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Failed assign: $Issue -> $Owner`n$($result -join "`n")"
    }
    Write-Output "ASSIGNED $Issue -> $Owner"
}

function Add-Tag {
    param(
        [string]$Issue,
        [string]$Label
    )

    $result = & npx -y @beads/bd tag $Issue $Label 2>&1
    if ($LASTEXITCODE -ne 0) {
        $text = $result -join "`n"
        if ($text -match "already" -or $text -match "exists") {
            Write-Output "SKIP_TAG_EXISTS $Issue #$Label"
        }
        else {
            throw "Failed tag: $Issue #$Label`n$text"
        }
    }
    else {
        Write-Output "TAGGED $Issue #$Label"
    }
}

function Set-Due {
    param(
        [string]$Issue,
        [string]$DueExpr
    )

    $result = & npx -y @beads/bd update $Issue --due $DueExpr 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Failed due date: $Issue due $DueExpr`n$($result -join "`n")"
    }
    Write-Output "DUE_SET $Issue -> $DueExpr"
}

$program = "AmigaXenon-5qx"

$phaseEpics = @(
    "AmigaXenon-5qx.1",
    "AmigaXenon-5qx.2",
    "AmigaXenon-5qx.3",
    "AmigaXenon-5qx.4",
    "AmigaXenon-5qx.5",
    "AmigaXenon-5qx.6"
)

# Epic-level phase ordering from implementation plan.
$epicDeps = @(
    @{ Blocked = "AmigaXenon-5qx.2"; Blocker = "AmigaXenon-5qx.1" },
    @{ Blocked = "AmigaXenon-5qx.3"; Blocker = "AmigaXenon-5qx.2" },
    @{ Blocked = "AmigaXenon-5qx.4"; Blocker = "AmigaXenon-5qx.3" },
    @{ Blocked = "AmigaXenon-5qx.5"; Blocker = "AmigaXenon-5qx.3" },
    @{ Blocked = "AmigaXenon-5qx.6"; Blocker = "AmigaXenon-5qx.4" },
    @{ Blocked = "AmigaXenon-5qx.6"; Blocker = "AmigaXenon-5qx.5" }
)

# Intra-phase task chains.
$taskDeps = @()

foreach ($phase in 1..6) {
    $prefix = "AmigaXenon-5qx.$phase"
    $count = if ($phase -eq 1) { 5 } else { 8 }
    foreach ($i in 2..$count) {
        $taskDeps += @{ Blocked = "$prefix.$i"; Blocker = "$prefix.$($i-1)" }
    }
}

# Cross-phase handoff gates.
$taskDeps += @(
    @{ Blocked = "AmigaXenon-5qx.2.1"; Blocker = "AmigaXenon-5qx.1.5" },
    @{ Blocked = "AmigaXenon-5qx.3.1"; Blocker = "AmigaXenon-5qx.2.8" },
    @{ Blocked = "AmigaXenon-5qx.4.1"; Blocker = "AmigaXenon-5qx.2.8" },
    @{ Blocked = "AmigaXenon-5qx.6.1"; Blocker = "AmigaXenon-5qx.3.8" },
    @{ Blocked = "AmigaXenon-5qx.6.1"; Blocker = "AmigaXenon-5qx.5.8" }
)

# Owner map
$ownerMap = @{}
$ownerMap[$program] = "philj"
foreach ($epic in $phaseEpics) { $ownerMap[$epic] = "philj" }

$ownerMap["AmigaXenon-5qx.1.1"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.1.2"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.1.3"] = "philj"
$ownerMap["AmigaXenon-5qx.1.4"] = "qa-release"
$ownerMap["AmigaXenon-5qx.1.5"] = "philj"

foreach ($i in 1..8) { $ownerMap["AmigaXenon-5qx.2.$i"] = "ue-dev" }
foreach ($i in 1..8) { $ownerMap["AmigaXenon-5qx.3.$i"] = "ue-dev" }
foreach ($i in 1..8) { $ownerMap["AmigaXenon-5qx.4.$i"] = "ue-dev" }
foreach ($i in 1..8) { $ownerMap["AmigaXenon-5qx.5.$i"] = "art-env" }
$ownerMap["AmigaXenon-5qx.5.8"] = "ue-dev"

$ownerMap["AmigaXenon-5qx.6.1"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.6.2"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.6.3"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.6.4"] = "qa-release"
$ownerMap["AmigaXenon-5qx.6.5"] = "ue-dev"
$ownerMap["AmigaXenon-5qx.6.6"] = "philj"
$ownerMap["AmigaXenon-5qx.6.7"] = "qa-release"
$ownerMap["AmigaXenon-5qx.6.8"] = "qa-release"

# Sprint 1 cut: foundation + early emulator integration path.
$sprint1 = @(
    "AmigaXenon-5qx.1",
    "AmigaXenon-5qx.2",
    "AmigaXenon-5qx.1.1",
    "AmigaXenon-5qx.1.2",
    "AmigaXenon-5qx.1.3",
    "AmigaXenon-5qx.1.4",
    "AmigaXenon-5qx.1.5",
    "AmigaXenon-5qx.2.1",
    "AmigaXenon-5qx.2.2",
    "AmigaXenon-5qx.2.3",
    "AmigaXenon-5qx.2.4",
    "AmigaXenon-5qx.2.5"
)

Write-Output "== Applying Dependencies =="
foreach ($d in $epicDeps) { Add-Dep -Blocked $d.Blocked -Blocker $d.Blocker }
foreach ($d in $taskDeps) { Add-Dep -Blocked $d.Blocked -Blocker $d.Blocker }

Write-Output "== Applying Owners =="
foreach ($issue in $ownerMap.Keys) { Set-Owner -Issue $issue -Owner $ownerMap[$issue] }

Write-Output "== Applying Sprint 1 Tags and Due Dates =="
foreach ($issue in $sprint1) {
    Add-Tag -Issue $issue -Label "sprint-1"
    Add-Tag -Issue $issue -Label "sprint-2026-05-a"
    Set-Due -Issue $issue -DueExpr "+2w"
}

$sprintFile = "DOCS\Sprint1_IssueCut_2026-05-05.txt"
$sprint1 | Set-Content -Encoding UTF8 $sprintFile

Write-Output "DEPENDENCIES_APPLIED=$($epicDeps.Count + $taskDeps.Count)"
Write-Output "OWNERS_ASSIGNED=$($ownerMap.Keys.Count)"
Write-Output "SPRINT1_ISSUES=$($sprint1.Count)"
Write-Output "SPRINT_FILE=$sprintFile"
