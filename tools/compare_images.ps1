param(
  [Parameter(Mandatory = $true)]
  [string]$Reference,

  [Parameter(Mandatory = $true)]
  [string]$Candidate
)

$ErrorActionPreference = "Stop"

function LinearLuminanceFromColor($c) {
  $r = $c.R / 255.0
  $g = $c.G / 255.0
  $b = $c.B / 255.0
  return 0.2126 * $r + 0.7152 * $g + 0.0722 * $b
}

Add-Type -AssemblyName System.Drawing

$refPath = Resolve-Path $Reference
$candPath = Resolve-Path $Candidate
$ref = [System.Drawing.Bitmap]::FromFile($refPath)
$cand = [System.Drawing.Bitmap]::FromFile($candPath)

try {
  if ($ref.Width -ne $cand.Width -or $ref.Height -ne $cand.Height) {
    throw "Image size mismatch: ref=$($ref.Width)x$($ref.Height), candidate=$($cand.Width)x$($cand.Height)"
  }

  $pixelCount = [double]($ref.Width * $ref.Height)
  $rgbCount = $pixelCount * 3.0
  $sumAbs = 0.0
  $sumSq = 0.0
  $sumAbsLuma = 0.0
  $sumSqLuma = 0.0
  $sumRefLuma = 0.0
  $sumCandLuma = 0.0
  $refMax = 0
  $candMax = 0

  for ($y = 0; $y -lt $ref.Height; ++$y) {
    for ($x = 0; $x -lt $ref.Width; ++$x) {
      $a = $ref.GetPixel($x, $y)
      $b = $cand.GetPixel($x, $y)
      $refMax = [Math]::Max($refMax, [Math]::Max($a.R, [Math]::Max($a.G, $a.B)))
      $candMax = [Math]::Max($candMax, [Math]::Max($b.R, [Math]::Max($b.G, $b.B)))

      foreach ($channel in @("R", "G", "B")) {
        $d = (($a.$channel - $b.$channel) / 255.0)
        $sumAbs += [Math]::Abs($d)
        $sumSq += $d * $d
      }

      $refLuma = LinearLuminanceFromColor $a
      $candLuma = LinearLuminanceFromColor $b
      $lumaDiff = $refLuma - $candLuma
      $sumRefLuma += $refLuma
      $sumCandLuma += $candLuma
      $sumAbsLuma += [Math]::Abs($lumaDiff)
      $sumSqLuma += $lumaDiff * $lumaDiff
    }
  }

  $mae = $sumAbs / $rgbCount
  $rmse = [Math]::Sqrt($sumSq / $rgbCount)
  $psnr = if ($rmse -gt 0.0) { 20.0 * [Math]::Log10(1.0 / $rmse) } else { [Double]::PositiveInfinity }

  $meanRefLuma = $sumRefLuma / $pixelCount
  $meanCandLuma = $sumCandLuma / $pixelCount
  $meanLumaDelta = $meanCandLuma - $meanRefLuma
  $meanLumaRatio = if ($meanRefLuma -gt 0.0) { $meanCandLuma / $meanRefLuma } else { [Double]::PositiveInfinity }
  $lumaMae = $sumAbsLuma / $pixelCount
  $lumaRmse = [Math]::Sqrt($sumSqLuma / $pixelCount)
  $lumaPsnr = if ($lumaRmse -gt 0.0) { 20.0 * [Math]::Log10(1.0 / $lumaRmse) } else { [Double]::PositiveInfinity }

  Write-Host ("[compare] reference={0}" -f $refPath)
  Write-Host ("[compare] candidate={0}" -f $candPath)
  Write-Host ("[compare] size={0}x{1} refMax={2} candMax={3}" -f $ref.Width, $ref.Height, $refMax, $candMax)
  Write-Host ("[compare] RGB   MAE={0:F6} RMSE={1:F6} PSNR={2:F2} dB" -f $mae, $rmse, $psnr)
  Write-Host ("[compare] Luma  refMean={0:F6} candMean={1:F6} delta={2:F6} ratio={3:F6}" -f `
    $meanRefLuma, $meanCandLuma, $meanLumaDelta, $meanLumaRatio)
  Write-Host ("[compare] Luma  MAE={0:F6} RMSE={1:F6} PSNR={2:F2} dB" -f $lumaMae, $lumaRmse, $lumaPsnr)
}
finally {
  $ref.Dispose()
  $cand.Dispose()
}

