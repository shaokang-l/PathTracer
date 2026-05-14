# ReSTIR, N=1
.\build-gpu-ninja\gpu\mypt.exe `
  --scene-xml assets\validation\cornell_box.xml `
  --headless --width 64 --height 64 --spp 16 --frames 1 `
  --max-depth 1 --gamma 1 --tonemap clamp --background 0,0,0 `
  --camera-origin 0,1,-18 --camera-target 0,1,0 `
  --direct-light restir --restir-initial-candidates 4 `
  --output artifacts\restir\gpu_restir_n4.png