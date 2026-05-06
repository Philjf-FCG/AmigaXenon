param(
    [string]$TaskName = "AmigaXenon-Sprint1-AutoAdvance",
    [string]$Label = "sprint-1",
    [int]$EveryMinutes = 5,
    [int]$Take = 1,
    [string]$LogPath = "DOCS\\logs\\beads_auto_advance.log"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repo = "C:\DevProjects\AmigaXenon"
$script = "$repo\Scripts\Beads\auto_advance_sprint.ps1"

if (-not (Test-Path $script)) {
    throw "Missing script: $script"
}

$actionArgs = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -Label "{1}" -Take {2} -LogPath "{3}"' -f $script, $Label, $Take, $LogPath
$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument $actionArgs -WorkingDirectory $repo
$trigger = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1) -RepetitionInterval (New-TimeSpan -Minutes $EveryMinutes) -RepetitionDuration (New-TimeSpan -Days 3650)
$settings = New-ScheduledTaskSettingsSet -StartWhenAvailable -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger -Settings $settings -Force | Out-Null
Write-Output "Registered scheduled task '$TaskName' (every $EveryMinutes minute(s))."
