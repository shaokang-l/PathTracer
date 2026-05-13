param(
  [string]$SceneXml = "assets/validation/cornell_box.xml",
  [string]$OutDir = "artifacts/xml_convergence",
  [int]$Width = 96,
  [int]$Height = 96,
  [int]$Spp = 64,
  [int]$MaxDepth = 4,
  [double]$Gamma = 2.2,
  [string]$Background = "0,0,0",
  [double]$MaxRmse = 0.03,
  [double]$MaxMae = 0.01,
  [switch]$SkipBuild,
  [switch]$HeadlessGpu
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$alignScript = Join-Path $PSScriptRoot "run_xml_alignment_smoke.ps1"

Push-Location $repo
try {
  & $alignScript `
    -SceneXml $SceneXml `
    -OutDir $OutDir `
    -Width $Width `
    -Height $Height `
    -Spp $Spp `
    -MaxDepth $MaxDepth `
    -Gamma $Gamma `
    -Tonemap "clamp" `
    -Background $Background `
    -SkipBuild:$SkipBuild `
    -HeadlessGpu:$HeadlessGpu

  if ($LASTEXITCODE -ne 0) {
    throw "Alignment render failed"
  }

  Add-Type -AssemblyName System.Drawing
  $cpuPath = Resolve-Path (Join-Path $OutDir "cpu.png")
  $gpuPath = Resolve-Path (Join-Path $OutDir "gpu.png")
  $cpu = [System.Drawing.Bitmap]::FromFile($cpuPath)
  $gpu = [System.Drawing.Bitmap]::FromFile($gpuPath)
  try {
    if ($cpu.Width -ne $gpu.Width -or $cpu.Height -ne $gpu.Height) {
      throw "Image size mismatch: CPU $($cpu.Width)x$($cpu.Height), GPU $($gpu.Width)x$($gpu.Height)"
    }

    $count = [double]($cpu.Width * $cpu.Height * 3)
    $sumAbs = 0.0
    $sumSq = 0.0
    $cpuMax = 0
    $gpuMax = 0

    for ($y = 0; $y -lt $cpu.Height; ++$y) {
      for ($x = 0; $x -lt $cpu.Width; ++$x) {
        $a = $cpu.GetPixel($x, $y)
        $b = $gpu.GetPixel($x, $y)
        $cpuMax = [Math]::Max($cpuMax, [Math]::Max($a.R, [Math]::Max($a.G, $a.B)))
        $gpuMax = [Math]::Max($gpuMax, [Math]::Max($b.R, [Math]::Max($b.G, $b.B)))

        foreach ($channel in @("R", "G", "B")) {
          $d = (($a.$channel - $b.$channel) / 255.0)
          $sumAbs += [Math]::Abs($d)
          $sumSq += $d * $d
        }
      }
    }

    if ($cpuMax -eq 0 -or $gpuMax -eq 0) {
      throw "Convergence smoke produced a black image: cpuMax=$cpuMax gpuMax=$gpuMax"
    }

    $mae = $sumAbs / $count
    $rmse = [Math]::Sqrt($sumSq / $count)
    Write-Host ("[convergence] size={0}x{1} spp={2} maxDepth={3} cpuMax={4} gpuMax={5} MAE={6:F6} RMSE={7:F6}" -f `
      $cpu.Width, $cpu.Height, $Spp, $MaxDepth, $cpuMax, $gpuMax, $mae, $rmse)

    if ($MaxRmse -ge 0 -and $rmse -gt $MaxRmse) {
      throw ("RMSE {0:F6} exceeded threshold {1:F6}" -f $rmse, $MaxRmse)
    }
    if ($MaxMae -ge 0 -and $mae -gt $MaxMae) {
      throw ("MAE {0:F6} exceeded threshold {1:F6}" -f $mae, $MaxMae)
    }
  }
  finally {
    $cpu.Dispose()
    $gpu.Dispose()
  }
}
finally {
  Pop-Location
}
