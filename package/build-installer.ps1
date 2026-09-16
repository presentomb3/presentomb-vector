param(
    [string] $BuildDir = "build-gon",
    [string] $Configuration = "Release",
    [string] $InnoSetupCompiler = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    [string] $CMake = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$resolvedBuildDir = Join-Path $repoRoot $BuildDir
$installerScript = Join-Path $repoRoot "installer\presentomb-vector.iss"
$version = ([regex]::Match((Get-Content (Join-Path $repoRoot "CMakeLists.txt") -Raw), 'project\([^\)]*VERSION\s+([0-9.]+)')).Groups[1].Value

if (-not $CMake) {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCommand) {
        $CMake = $cmakeCommand.Source
    } else {
        $visualStudioCMake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $visualStudioCMake) { $CMake = $visualStudioCMake }
    }
}

if (-not $CMake -or -not (Test-Path $CMake)) { throw "CMake was not found. Install CMake or pass -CMake." }
if (-not (Test-Path $resolvedBuildDir)) { throw "Build directory '$resolvedBuildDir' does not exist. Configure it with CMake first." }
if (-not $version) { throw "Could not read project version from CMakeLists.txt." }

& $CMake --build $resolvedBuildDir --config $Configuration --target PresentombDSO2_VST3
if ($LASTEXITCODE -ne 0) { throw "VST3 build failed with exit code $LASTEXITCODE." }
& $CMake --build $resolvedBuildDir --config $Configuration --target PresentombDSO2_Standalone
if ($LASTEXITCODE -ne 0) { throw "Standalone build failed with exit code $LASTEXITCODE." }

$artifactRoot = Join-Path $resolvedBuildDir "PresentombDSO2_artefacts\$Configuration"
$vst3 = Join-Path $artifactRoot "VST3\presentomb vector.vst3"
$standalone = Join-Path $artifactRoot "Standalone\presentomb vector.exe"
if (-not (Test-Path $vst3)) { throw "Missing VST3 artifact: '$vst3'." }
if (-not (Test-Path $standalone)) { throw "Missing standalone artifact: '$standalone'." }

if (-not (Test-Path $InnoSetupCompiler)) {
    throw "Inno Setup compiler was not found at '$InnoSetupCompiler'. Install Inno Setup 6 or pass -InnoSetupCompiler."
}

& $InnoSetupCompiler "/DAppVersion=$version" "/DBuildRoot=$artifactRoot" $installerScript
if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed with exit code $LASTEXITCODE." }

$installer = Join-Path $repoRoot "dist\presentomb-vector-$version-windows.exe"
if (-not (Test-Path $installer)) { throw "Installer compiler succeeded but '$installer' was not created." }
Write-Host "Created $installer"
