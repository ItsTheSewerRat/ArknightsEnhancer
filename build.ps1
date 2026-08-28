[CmdletBinding()]
param(
    [string]$ReShadeSdkDir = "",
    [string]$BuildDirectory = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($ReShadeSdkDir)) {
    $ReShadeSdkDir = Join-Path $projectRoot "external\reshade-sdk"
}

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $projectRoot "build"
}

if (-not (Test-Path -LiteralPath (Join-Path $ReShadeSdkDir "include\reshade.hpp"))) {
    $externalDirectory = Split-Path -Parent $ReShadeSdkDir
    New-Item -ItemType Directory -Path $externalDirectory -Force | Out-Null

    Write-Host "Downloading the ReShade 6.7.3 SDK..."
    & git clone --depth 1 --branch v6.7.3 https://github.com/crosire/reshade.git $ReShadeSdkDir
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to download ReShade 6.7.3. Pass -ReShadeSdkDir with an existing source tree."
    }
}

$imguiHeader = Join-Path $ReShadeSdkDir "deps\imgui\imgui.h"
if (-not (Test-Path -LiteralPath $imguiHeader)) {
    Write-Host "Downloading the ReShade ImGui dependency..."
    & git -C $ReShadeSdkDir submodule update --init --depth 1 deps/imgui
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to download the ReShade ImGui dependency."
    }
}

$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($null -eq $cmakeCommand) {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswherePath)) {
        throw "CMake was not found. Install Visual Studio with Desktop development with C++."
    }

    $visualStudioRoot = & $vswherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $bundledCmake = Join-Path $visualStudioRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (-not (Test-Path -LiteralPath $bundledCmake)) {
        throw "Visual Studio's bundled CMake was not found."
    }
    $cmakeExecutable = $bundledCmake
} else {
    $cmakeExecutable = $cmakeCommand.Source
}

& $cmakeExecutable -S $projectRoot -B $BuildDirectory -A x64 "-DRESHADE_SDK_DIR=$ReShadeSdkDir"
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed."
}

& $cmakeExecutable --build $BuildDirectory --config Release
if ($LASTEXITCODE -ne 0) {
    throw "The Release build failed."
}

$outputPath = Join-Path $BuildDirectory "Release\ArknightsEnhancer.addon64"
Write-Host "Built: $outputPath"
