param(
    [string] $Configuration = "Release",
    [string] $ProductVersion = "0.1.0",
    [switch] $AcceptEula
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildOutput = Join-Path $repoRoot "build\SGMReverb_artefacts\$Configuration"
$installerOutput = Join-Path $repoRoot "build\installer"
$wxsPath = Join-Path $PSScriptRoot "Product.wxs"
$msiPath = Join-Path $installerOutput "SGM-Reverb-$ProductVersion-win64.msi"

if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    throw "WiX Toolset v4 CLI was not found. Install it with: dotnet tool install --global wix"
}

if (-not (Test-Path (Join-Path $buildOutput "Standalone\SGM Reverb.exe"))) {
    throw "Standalone build output was not found. Run: cmake --build build --config $Configuration"
}

if (-not (Test-Path (Join-Path $buildOutput "VST3\SGM Reverb.vst3\Contents\x86_64-win\SGM Reverb.vst3"))) {
    throw "VST3 build output was not found. Run: cmake --build build --config $Configuration"
}

New-Item -ItemType Directory -Force -Path $installerOutput | Out-Null

$wixArguments = @(
    "build",
    $wxsPath,
    "-arch", "x64",
    "-d", "BuildOutput=$buildOutput",
    "-d", "ProjectRoot=$repoRoot",
    "-d", "ProductVersion=$ProductVersion",
    "-ext", "WixToolset.UI.wixext",
    "-out", $msiPath
)

if ($AcceptEula) {
    & wix eula accept wix7
    if ($LASTEXITCODE -ne 0) {
        throw "WiX EULA acceptance failed with exit code $LASTEXITCODE."
    }
}

& wix @wixArguments
if ($LASTEXITCODE -ne 0) {
    throw "WiX failed with exit code $LASTEXITCODE."
}

Write-Host "Created $msiPath"
