param(
  [string]$SceneXml = "assets/validation/cornell_box.xml",
  [string]$OutDir = "artifacts/xml_alignment",
  [int]$Width = 64,
  [int]$Height = 64,
  [int]$Spp = 1,
  [int]$MaxDepth = 2,
  [double]$Gamma = 2.2,
  [string]$Tonemap = "clamp",
  [string]$Background = "0,0,0",
  [string]$CameraOrigin = "",
  [string]$CameraTarget = "",
  [string]$CameraUp = "0,1,0",
  [double]$Fov = 0,
  [switch]$SkipBuild,
  [switch]$HeadlessGpu
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $repo
try {
  New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

  $cpuOut = Join-Path $OutDir "cpu.png"
  $gpuOut = Join-Path $OutDir "gpu.png"
  Write-Host "[xml-smoke] SceneXml=$SceneXml Width=$Width Height=$Height Spp=$Spp MaxDepth=$MaxDepth Gamma=$Gamma Tonemap=$Tonemap Background=$Background"

  if (-not $SkipBuild) {
    cmake --build --preset cpu-relwithdebinfo
    if ($LASTEXITCODE -ne 0) { throw "CPU build failed" }

    cmake --build --preset gpu-relwithdebinfo
    if ($LASTEXITCODE -ne 0) { throw "GPU build failed" }
  }

  $cpuExe = "build-cpu/cpu/src/RelWithDebInfo/PathTracer.exe"
  $gpuExe = "build-gpu/gpu/RelWithDebInfo/mypt.exe"

  $cpuArgs = @(
    "--scene-xml", $SceneXml,
    "--width", $Width,
    "--height", $Height,
    "--spp", $Spp,
    "--max-depth", $MaxDepth,
    "--gamma", $Gamma,
    "--background", $Background,
    "--output", $cpuOut
  )
  if ($CameraOrigin -ne "" -or $CameraTarget -ne "") {
    $cpuArgs += @("--camera-origin", $CameraOrigin, "--camera-target", $CameraTarget, "--camera-up", $CameraUp)
    if ($Fov -gt 0) {
      $cpuArgs += @("--fov", $Fov)
    }
  }

  & $cpuExe @cpuArgs
  if ($LASTEXITCODE -ne 0) { throw "CPU XML render failed" }

  $gpuArgs = @(
    "--scene-xml", $SceneXml,
    "--width", $Width,
    "--height", $Height,
    "--spp", $Spp,
    "--max-depth", $MaxDepth,
    "--gamma", $Gamma,
    "--tonemap", $Tonemap,
    "--miss-color", $Background,
    "--frames", "1",
    "--output", $gpuOut
  )
  if ($CameraOrigin -ne "" -or $CameraTarget -ne "") {
    $gpuArgs += @("--camera-origin", $CameraOrigin, "--camera-target", $CameraTarget, "--camera-up", $CameraUp)
    if ($Fov -gt 0) {
      $gpuArgs += @("--fov", $Fov)
    }
  }
  if ($HeadlessGpu) {
    $gpuArgs += "--headless"
  }

  & $gpuExe @gpuArgs
  if ($LASTEXITCODE -ne 0) { throw "GPU XML render failed" }

  Write-Host "[xml-smoke] CPU image: $cpuOut"
  Write-Host "[xml-smoke] GPU image: $gpuOut"
}
finally {
  Pop-Location
}
