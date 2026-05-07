param(
    [ValidateSet("run", "build", "ui", "quick")]
    [string]$Mode = "run",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [switch]$Rebuild,

    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectRoot

function Write-Step {
    param([string]$Text)
    Write-Host "`n=== $Text ===" -ForegroundColor Cyan
}

function Resolve-SolverPath {
    param([string]$Config)

    $candidates = @(
        (Join-Path $ProjectRoot "build\$Config\bvp_solver.exe"),
        (Join-Path $ProjectRoot "build\bvp_solver.exe"),
        (Join-Path $ProjectRoot "build\bvp_solver")
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $candidates[0]
}

function Ensure-Build {
    param([string]$Config, [bool]$DoRebuild)

    Write-Step "CMake configure"
    cmake -S . -B build

    Write-Step "CMake build ($Config)"
    if ($DoRebuild) {
        cmake --build build --config $Config --clean-first
    }
    else {
        cmake --build build --config $Config
    }
}

function Run-Solver {
    param(
        [string]$Solver,
        [string]$InputJson,
        [string]$OutDir
    )

    if (-not (Test-Path $InputJson)) {
        throw "Input JSON not found: $InputJson"
    }

    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    Write-Step "Run solver: $(Split-Path -Leaf $InputJson) -> $OutDir"
    & $Solver $InputJson $OutDir
}

$needBuild = $Mode -in @("run", "build", "quick")
if ($needBuild) {
    Ensure-Build -Config $Configuration -DoRebuild:$Rebuild
}

$solverPath = Resolve-SolverPath -Config $Configuration
if (($Mode -in @("run", "quick")) -and -not (Test-Path $solverPath)) {
    throw "Solver executable not found: $solverPath"
}

switch ($Mode) {
    "build" {
        Write-Host "Build completed." -ForegroundColor Green
    }
    "run" {
        $input = Join-Path $ProjectRoot "input_examples\default_input.json"
        $out = if ($OutputDir) { $OutputDir } else { Join-Path $ProjectRoot "output" }
        Run-Solver -Solver $solverPath -InputJson $input -OutDir $out
    }
    "quick" {
        $input = Join-Path $ProjectRoot "input_examples\quick_input.json"
        $out = if ($OutputDir) { $OutputDir } else { Join-Path $ProjectRoot "output_quick" }
        Run-Solver -Solver $solverPath -InputJson $input -OutDir $out
    }
    "ui" {
        Write-Step "Install UI dependencies"
        python -m pip install -r (Join-Path $ProjectRoot "ui\requirements.txt")

        Write-Step "Run UI"
        python (Join-Path $ProjectRoot "ui\app.py")
    }
}

Write-Host "`nDone." -ForegroundColor Green
