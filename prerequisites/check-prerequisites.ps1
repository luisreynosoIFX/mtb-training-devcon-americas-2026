[CmdletBinding()]
param(
    [string]$OutputPath = (Join-Path (Get-Location) 'PSOC_DevCon_prerequisites_report.html')
)

$ErrorActionPreference = 'SilentlyContinue'

# Pattern: regex on the uninstall-registry DisplayName (with ™/(TM) stripped). Commands/Paths: CLI-only fallbacks that are safe to run with --version.
$requirements = @(
    [pscustomobject]@{ Name = 'ModusToolbox Tools Package'; Minimum = '3.9'; Tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Pattern = '^ModusToolbox Tools Package' }
    [pscustomobject]@{ Name = 'Eclipse IDE for ModusToolbox'; Minimum = '2026.3.0'; Tracks = @('PSOC Control','Gold'); Pattern = '^Eclipse (IDE )?for ModusToolbox' }
    [pscustomobject]@{ Name = 'ModusToolbox Programming Tools'; Minimum = '1.9.0'; Tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Pattern = '^ModusToolbox Programming Tools' }
    [pscustomobject]@{ Name = 'Arm GNU Toolchain (GCC)'; Minimum = '14.2.1'; Tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Pattern = '^Arm GNU Toolchain' }
    [pscustomobject]@{ Name = 'ModusToolbox Edge Protect Security Suite'; Minimum = '2.2.0'; Tracks = @('PSOC Edge','Gold','General MTB'); Pattern = '^ModusToolbox Edge Protect Security Suite' }
    [pscustomobject]@{ Name = 'DEEPCRAFT Audio Enhancement Tech Pack'; Minimum = '1.3.0'; Tracks = @('PSOC Edge','Gold'); Pattern = '^DEEPCRAFT Audio Enhancement Tech Pack' }
    [pscustomobject]@{ Name = 'ModusToolbox Audio SW Codecs Tech Pack'; Minimum = '1.0.3'; Tracks = @('PSOC Edge','Gold'); Pattern = '^ModusToolbox Audio SW Codecs Tech Pack' }
    [pscustomobject]@{ Name = 'ModusToolbox CAPSENSE and Multi-Sense'; Minimum = '1.6'; Tracks = @('PSOC Multi-Sense','Gold'); Pattern = '^ModusToolbox CAPSENSE and Multi-Sense' }
    [pscustomobject]@{ Name = 'ModusToolbox Motor Suite'; Minimum = '2.9'; Tracks = @('PSOC Control','Gold'); Pattern = '^ModusToolbox Motor Suite' }
    [pscustomobject]@{ Name = 'ModusToolbox Power Suite'; Minimum = '1.2.0*'; Tracks = @('PSOC Control','Gold'); Pattern = '^ModusToolbox Power Suite' }
    [pscustomobject]@{ Name = 'ModusToolbox LiveMonitoring'; Minimum = '1.0.x'; Tracks = @('PSOC Control','Gold'); Pattern = '^ModusToolbox Live ?Monitor' }
    [pscustomobject]@{ Name = 'ModusToolbox Machine Learning Pack'; Minimum = '3.3.0'; Tracks = @('PSOC Edge','Gold'); Pattern = '^ModusToolbox Machine Learning Pack' }
    [pscustomobject]@{ Name = 'DEEPCRAFT Studio'; Minimum = '5.14.5788'; Tracks = @('PSOC Edge','Gold'); Pattern = '^(DEEPCRAFT|Imagimob) Studio' }
    [pscustomobject]@{ Name = 'DEEPCRAFT ModelConverter'; Minimum = '5.11.5765'; Tracks = @('PSOC Edge','Gold'); Pattern = '^DEEPCRAFT Model ?Converter' }
    [pscustomobject]@{ Name = 'Microsoft Visual Studio Code'; Minimum = '1.139'; Tracks = @('PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Pattern = '^Microsoft Visual Studio Code'; Commands = @('code') }
    [pscustomobject]@{ Name = 'ModusToolbox for VS Code extension'; Minimum = '-'; Tracks = @('PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Extension = 'infineonag.modustoolbox-for-vscode'; NotFound = 'ModusToolbox for VS Code extension not found in VS Code' }
    [pscustomobject]@{ Name = 'Tera Term, or other serial terminal'; Minimum = '-'; Tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','General MTB'); Pattern = '^(Tera Term|PuTTY|CoolTerm|MobaXterm)' }
    [pscustomobject]@{ Name = 'Git for Windows'; Minimum = '2.55'; Tracks = @('General MTB'); Pattern = '^Git( version [\d.]+)?$'; Commands = @('git') }
    # Zephyr tools from zephyr-windows-training-prereqs; minimums from the Zephyr Getting Started Guide (others: presence only).
    [pscustomobject]@{ Name = 'Zephyr: CMake'; Minimum = '3.28.0'; Tracks = @('Zephyr'); Commands = @('cmake') }
    [pscustomobject]@{ Name = 'Zephyr: Ninja'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('ninja') }
    [pscustomobject]@{ Name = 'Zephyr: Python'; Minimum = '3.12'; Tracks = @('Zephyr'); Commands = @('python'); WarnAbove = '3.12'; Warning = 'Zephyr recommends Python 3.12; newer versions may fail on Windows' }
    [pscustomobject]@{ Name = 'Zephyr: Git'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('git') }
    [pscustomobject]@{ Name = 'Zephyr: wget'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('wget') }
    [pscustomobject]@{ Name = 'Zephyr: 7-Zip (7z)'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('7z'); VersionArgs = '' }
    [pscustomobject]@{ Name = 'Zephyr: gperf'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('gperf') }
    [pscustomobject]@{ Name = 'Zephyr: Device Tree Compiler (dtc)'; Minimum = '1.4.6'; Tracks = @('Zephyr'); Commands = @('dtc') }
    [pscustomobject]@{ Name = 'Zephyr: west'; Minimum = '-'; Tracks = @('Zephyr'); Commands = @('west'); Optional = $true; NotFound = 'Not found in PATH; west is installed into a Python virtual environment during the lab' }
    [pscustomobject]@{ Name = 'PSoC Programmer'; Minimum = '3.29.6.4838'; Tracks = @('PSOC Multi-Sense'); Pattern = '^PSoC Programmer' }
    [pscustomobject]@{ Name = 'J-Link Driver'; Minimum = '9.74'; Tracks = @('PSOC Control','Gold'); Pattern = '^J-Link' }
    [pscustomobject]@{ Name = 'LLVM ARM Compiler'; Minimum = '19.1.5'; Tracks = @('PSOC Edge','Gold'); Pattern = 'LLVM|Arm Toolchain for Embedded'; Commands = @('clang'); Paths = @("$env:USERPROFILE\*llvm*\bin\clang.exe", "$env:USERPROFILE\*llvm*\*\bin\clang.exe", "$env:USERPROFILE\Infineon\Tools\*llvm*\bin\clang.exe", "$env:USERPROFILE\Infineon\Tools\*llvm*\*\bin\clang.exe", "C:\Infineon\Tools\*llvm*\bin\clang.exe", "C:\Infineon\Tools\*llvm*\*\bin\clang.exe") }
    [pscustomobject]@{ Name = 'Audacity'; Minimum = '3.7.5'; Tracks = @('PSOC Edge'); Pattern = '^Audacity' }
    [pscustomobject]@{ Name = 'EEZ-Studio'; Minimum = '0.29.0'; Tracks = @('PSOC Edge','Gold'); Pattern = '^EEZ.?Studio' }
    [pscustomobject]@{ Name = 'Python3 with pypng and lz4 modules'; Minimum = '3.12'; Tracks = @('PSOC Edge','Gold'); PythonModules = @('png','lz4') }
    [pscustomobject]@{ Name = 'Pngquant'; Minimum = '2.17.0'; Tracks = @('PSOC Edge','Gold'); Pattern = '^pngquant'; Commands = @('pngquant') }
    [pscustomobject]@{ Name = 'DEEPCRAFT Voice Assistant account'; Minimum = '-'; Tracks = @('PSOC Edge','Gold'); Manual = $true }
    [pscustomobject]@{ Name = 'Nmap'; Minimum = '7.98'; Tracks = @('PSOC Edge'); Pattern = '^Nmap'; Commands = @('nmap') }
    [pscustomobject]@{ Name = 'Wireshark'; Minimum = '4.6.3'; Tracks = @('PSOC Edge'); Pattern = '^Wireshark' }
    [pscustomobject]@{ Name = 'GitHub Copilot or other AI assistant'; Minimum = '-'; Tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','Zephyr','General MTB'); Extension = 'github.copilot-chat'; NotFound = 'Copilot Chat not found in VS Code, but other AI assistants can be used'; Optional = $true }
)

function Get-VersionObject([string]$Value) {
    $match = [regex]::Match(($Value -replace ',', '.'), '(\d+(?:\.\d+){0,3})')
    if (-not $match.Success) { return $null }
    try { return [version]$match.Value } catch { return $null }
}

function Get-InstalledSoftware {
    $paths = @('HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*')
    Get-ItemProperty $paths | Where-Object DisplayName | ForEach-Object {
        $name = (($_.DisplayName -replace '\u2122|\(TM\)', '') -replace '\s+', ' ').Trim()
        $version = [string]$_.DisplayVersion
        if (-not (Get-VersionObject $version) -and $name -match '(\d+(?:\.\d+){1,3})') { $version = $Matches[1] }
        [pscustomobject]@{ Name = $name; Version = $version; Location = [string]$_.InstallLocation }
    }
}

$installed = @(Get-InstalledSoftware)

# Stdin is closed and a timeout applied so a probe can never block the script.
function Invoke-Probe([string]$Path, [string]$Arguments, [int]$TimeoutMs = 15000) {
    try {
        $psi = [Diagnostics.ProcessStartInfo]::new($Path, $Arguments)
        $psi.UseShellExecute = $false
        $psi.CreateNoWindow = $true
        $psi.RedirectStandardInput = $true
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $process = [Diagnostics.Process]::Start($psi)
        $process.StandardInput.Close()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutMs)) { $process.Kill(); return [pscustomobject]@{ ExitCode = -1; Output = '' } }
        [pscustomobject]@{ ExitCode = $process.ExitCode; Output = "$($stdout.Result)`n$($stderr.Result)" }
    } catch {
        [pscustomobject]@{ ExitCode = -1; Output = '' }
    }
}

function Get-CommandVersion([string]$Path, [string]$Arguments = '--version') {
    $line = (Invoke-Probe $Path $Arguments).Output -split "`r?`n" | Where-Object { $_ -match '\d+\.\d+' } | Select-Object -First 1
    if ($line -match '(\d+(?:\.\d+){1,3})') { return $Matches[1] }
    ''
}

function New-Result([bool]$Found, [string]$Version, [string]$Detail) {
    [pscustomobject]@{ Found = $Found; Version = $Version; Detail = $Detail }
}

# Checks user-installed extensions and extensions bundled with VS Code (e.g. built-in Copilot Chat).
function Find-VSCodeExtension([string]$Id) {
    $installRoots = @($installed | Where-Object { $_.Name -match '^Microsoft Visual Studio Code' -and $_.Location } | ForEach-Object Location) +
                    @("$env:LOCALAPPDATA\Programs\Microsoft VS Code", "$env:ProgramFiles\Microsoft VS Code")
    $extensionDirs = @("$env:USERPROFILE\.vscode\extensions", $env:VSCODE_EXTENSIONS) +
                     @($installRoots | ForEach-Object { "$_\resources\app\extensions"; "$_\*\resources\app\extensions" })
    $manifests = $extensionDirs | Where-Object { $_ } | ForEach-Object { Get-ChildItem "$_\*\package.json" -ErrorAction SilentlyContinue }
    $manifests | ForEach-Object {
        try { $package = Get-Content $_.FullName -Raw | ConvertFrom-Json } catch { return }
        if ("$($package.publisher).$($package.name)" -eq $Id) {
            [pscustomobject]@{ Version = [string]$package.version; Path = $_.DirectoryName }
        }
    } | Sort-Object { Get-VersionObject $_.Version } -Descending | Select-Object -First 1
}

function Find-Requirement([pscustomobject]$Requirement) {
    if ($Requirement.Manual) { return New-Result $false '' 'Manual confirmation required' }

    if ($Requirement.Extension) {
        $extension = Find-VSCodeExtension $Requirement.Extension
        if ($extension) { return New-Result $true $extension.Version "$($Requirement.Extension) $($extension.Path)" }
        return New-Result $false '' $Requirement.NotFound
    }

    if ($Requirement.PythonModules) {
        $python = Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $python) { return New-Result $false '' 'python not found on PATH' }
        $version = Get-CommandVersion $python.Source
        $missing = @($Requirement.PythonModules | Where-Object { (Invoke-Probe $python.Source "-c `"import $_`"").ExitCode -ne 0 })
        $detail = if ($missing.Count -eq 0) { "$($python.Source); modules png, lz4 OK" } else { "$($python.Source); missing modules: $($missing -join ', ')" }
        return New-Result ($missing.Count -eq 0) $version $detail
    }

    if ($Requirement.Pattern) {
        $best = $installed | Where-Object { $_.Name -match $Requirement.Pattern } | Sort-Object { Get-VersionObject $_.Version } -Descending | Select-Object -First 1
        if ($best) { return New-Result $true $best.Version "$($best.Name) $($best.Location)".Trim() }
    }

    # -CommandType Application skips PowerShell aliases such as the built-in 'wget'.
    $executables = @($Requirement.Commands | Where-Object { $_ } | ForEach-Object { (Get-Command $_ -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source }) +
                   @($Requirement.Paths | Where-Object { $_ } | ForEach-Object { Get-ChildItem $_ -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName })
    $versionArgs = if ($Requirement.PSObject.Properties['VersionArgs']) { $Requirement.VersionArgs } else { '--version' }
    $best = $executables | Where-Object { $_ } | Select-Object -Unique |
        ForEach-Object { [pscustomobject]@{ Path = $_; Version = Get-CommandVersion $_ $versionArgs } } |
        Sort-Object { Get-VersionObject $_.Version } -Descending | Select-Object -First 1
    if ($best) {
        $detail = $best.Path
        $actual = Get-VersionObject $best.Version
        if ($Requirement.WarnAbove -and $actual -and [version]"$($actual.Major).$($actual.Minor)" -gt [version]$Requirement.WarnAbove) {
            $detail = "$detail. Warning: $($Requirement.Warning)"
        }
        return New-Result $true $best.Version $detail
    }

    if ($Requirement.NotFound) { return New-Result $false '' $Requirement.NotFound }

    $searched = @()
    if ($Requirement.Pattern) { $searched += 'installed programs' }
    if ($Requirement.Commands) { $searched += 'PATH' }
    if ($Requirement.Paths) { $searched += "the User folder ($env:USERPROFILE)" }
    New-Result $false '' "Not found in $($searched -join ', ' -replace ', ([^,]+)$', ' or $1')"
}

function Get-RequirementStatus([pscustomobject]$Requirement, [pscustomobject]$Found) {
    if ($Requirement.Optional) { if ($Found.Found) { return 'Pass' } else { return 'Optional' } }
    if ($Requirement.Manual) { return 'Review' }
    if (-not $Found.Found) { return 'Missing' }
    if ($Requirement.Minimum -eq '-') { return 'Pass' }
    $minimum = Get-VersionObject $Requirement.Minimum
    $actual = Get-VersionObject $Found.Version
    if (-not $actual) { return 'Review' }
    if ($actual -ge $minimum) { return 'Pass' }
    'Outdated'
}

$results = foreach ($requirement in $requirements) {
    $found = Find-Requirement $requirement
    [pscustomobject]@{ Requirement = $requirement; Found = $found; Status = Get-RequirementStatus $requirement $found }
}

$tracks = @('PSOC Control','PSOC Edge','PSOC Multi-Sense','Gold','Zephyr','General MTB')
$style = @'
body { font-family: Segoe UI, sans-serif; color: #17202a; margin: 2rem; background: #f4f7f8; }
h1 { color: #0b4f6c; } h2 { border-bottom: 2px solid #0b4f6c; padding-bottom: .35rem; margin-top: 2rem; }
.summary { padding: 1rem; background: white; border-left: 6px solid #0b4f6c; margin-bottom: 1rem; }
table { width: 100%; border-collapse: collapse; background: white; margin: .75rem 0 2rem; }
th, td { text-align: left; padding: .55rem .7rem; border: 1px solid #d7dfe3; vertical-align: top; } th { background: #dcecf1; }
.Pass { color: #087f23; font-weight: 600; } .Missing, .Outdated { color: #b42318; font-weight: 600; } .Review { color: #9a6700; font-weight: 600; } .Optional { color: #52606d; font-weight: 600; }
small { color: #52606d; }
'@

function Html([string]$Text) { [System.Net.WebUtility]::HtmlEncode($Text) }
$generated = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$html = "<!doctype html><html><head><meta charset='utf-8'><title>PSOC DevCon prerequisite report</title><style>$style</style></head><body><h1>PSOC DevCon prerequisite report</h1><p>Generated $generated on $env:COMPUTERNAME. Minimum versions come from PSOC_DevCon_class_prework_v1.xlsx.</p>"
foreach ($track in $tracks) {
    $trackResults = @($results | Where-Object { $_.Requirement.Tracks -contains $track })
    $blocking = @($trackResults | Where-Object Status -in @('Missing','Outdated','Review')).Count
    $summary = if ($blocking -eq 0) { 'All automated requirements passed' } else { "$blocking item(s) need attention" }
    $html += "<section><h2>$(Html $track)</h2><div class='summary'><strong>$(Html $summary)</strong><br><small>Optional items do not affect this result. Review means the script needs user confirmation.</small></div><table><thead><tr><th>Tool</th><th>Required</th><th>Status</th><th>Detected version</th><th>Details</th></tr></thead><tbody>"
    foreach ($item in $trackResults) {
        $html += "<tr><td>$(Html $item.Requirement.Name)</td><td>$(Html $item.Requirement.Minimum)</td><td class='$($item.Status)'>$(Html $item.Status)</td><td>$(Html $item.Found.Version)</td><td>$(Html $item.Found.Detail)</td></tr>"
    }
    $html += '</tbody></table></section>'
}
$html += '<p><small>Detection uses Windows uninstall registry entries and commands available on PATH. Product package names and account prerequisites may require manual confirmation.</small></p></body></html>'
Set-Content -Path $OutputPath -Value $html -Encoding UTF8
Write-Output "Report written to $OutputPath"