# ReSTIR DI No-Reuse 阶段验证记录

本文记录 ReSTIR DI 第一阶段，也就是 **per-pixel RIS without temporal/spatial reuse** 的实现口径、验证方法和当前结论。这个阶段的目标不是完成完整 ReSTIR DI，而是先证明：

1. 当前 direct-light backend 可以在 NEE 和 no-reuse ReSTIR/RIS 之间切换。
2. `initialCandidates = 1` 时，no-reuse RIS 能退化到普通 one-sample NEE。
3. `initialCandidates > 1` 时，在 many-light 场景下可以降低 direct-light variance，同时不引入明显平均亮度偏移。

## 当前实现范围

当前 GPU 侧 direct lighting 有两个模式：

```text
--direct-light nee
--direct-light restir --restir-initial-candidates N
```

在 no-reuse 阶段，ReSTIR/RIS 的工作方式可以理解为：

1. 对同一个 shading point 生成 `N` 个 direct-light candidates。
2. 对每个 candidate 计算未遮挡 direct-light contribution、target 和 source pdf。
3. 用 reservoir importance sampling 只保留一个代表性 light sample。
4. 最终只对选中的 sample 发一次 shadow ray。
5. 用 reservoir weight 补偿采样概率，得到 direct-light estimate。

所以它不是“免费得到 `N` 条 shadow ray 的结果”。更准确地说，它是用 `N` 次较便宜的 candidate evaluation 加上 1 次 visibility ray，尽量接近多 light sample 的质量。many-light 场景下，这一点尤其有价值，因为 visibility ray 通常比 candidate 生成和 target 计算更贵。

## 为什么现在还要用 Reservoir

如果只看 no-reuse 且 `N` 很小的情况，reservoir 不是唯一实现方式。完全可以先把 `N` 个候选存在数组里，计算权重后做一次 weighted sampling。

但 reservoir 对后续阶段更重要：

- 它支持 streaming update，不需要保存全部候选。
- 它把 selected sample、`wSum`、`W`、`M` 压缩成一个可复用状态。
- Temporal reuse 时，上一帧 reservoir 可以作为一个候选继续 merge。
- Spatial reuse 时，邻居 pixel 的 reservoir 也可以作为候选 merge。
- Initial candidates、temporal candidate、spatial candidates 可以走同一套 update/finalize 接口。

因此当前阶段使用 reservoir 更多是为了让后续 temporal/spatial reuse 的数据模型提前稳定下来。

## 核心数学验证

CPU 侧已经补了面向 common ReSTIR helper 的单元测试，覆盖内容包括：

- reservoir 清空、更新、finalize 的基本行为；
- candidate weight 和零权重安全性；
- selection probability 是否符合权重比例；
- target function、luminance、geometry term 的边界情况；
- no-reuse RIS estimator 在 toy lights 上是否匹配 reference integral；
- `initialCandidates = 1` 是否退化到 one-sample NEE；
- 增加 initial candidates 是否降低估计方差。

这部分测试的意义是把 GPU 集成之前最容易写错的数学细节先隔离出来验证，避免在 OptiX path tracing 里同时 debug target、pdf、reservoir 和 visibility。

## Many-Light 验证场景

为了让 no-reuse RIS 的优势更明显，新增了一个 Cornell box many-light validation scene：

```text
assets/validation/cornell_many_lights_16.xml
assets/validation/cornell_many_lights_16/light_00.obj
...
assets/validation/cornell_many_lights_16/light_15.obj
```

这个场景保留 Cornell box 的墙面、地面和天花板结构，在 ceiling 上放置 4x4 共 16 个小 area lights。每个 light 是独立 OBJ shape 和独立 area emitter，radiance 有意设置为不均匀分布，使 light selection 的重要性更明显。

GPU smoke test 能正常加载该场景：

```text
loaded Mitsuba XML subset: assets\validation\cornell_many_lights_16.xml (21 meshes, 21 materials)
scene: 21 meshes, bounds = [(-1,0,-1) .. (1,2,1)]
```

注意：当前 `.gitignore` 忽略了 `assets/`，如果需要提交这些 validation assets，需要使用 `git add -f`。

## 实验设置

为了把变量控制在 direct lighting，本轮实验使用：

```text
resolution: 512x512
max-depth: 1
gamma: 1
tonemap: clamp
background: 0,0,0
camera-origin: 0,1,-3.2
camera-target: 0,1,0
scene: assets/validation/cornell_many_lights_16.xml
```

使用高 spp NEE 作为 reference：

```text
many_lights_ref_nee_512_spp1024.png
```

然后比较：

```text
many_lights_nee_512_spp16.png
many_lights_restir_n1_512_spp16.png
many_lights_restir_n4_512_spp16.png
many_lights_nee_512_spp64.png
many_lights_restir_n4_512_spp64.png
```

图像比较使用 `tools/compare_images.ps1`，指标包括 RGB MAE/RMSE/PSNR 和 luminance MAE/RMSE/PSNR、mean luminance ratio。

## 实验结果

低 spp 对高 spp reference 的结果如下：

| Candidate | RGB PSNR | Luma PSNR | Luma RMSE | Mean Luma Ratio |
| --- | ---: | ---: | ---: | ---: |
| NEE, spp16 | 24.72 dB | 24.65 dB | 0.058536 | 0.997219 |
| ReSTIR N=1, spp16 | 24.73 dB | 24.67 dB | 0.058444 | 0.997848 |
| ReSTIR N=4, spp16 | 29.47 dB | 29.50 dB | 0.033510 | 0.999469 |

同样在 `spp64` 下：

| Candidate | RGB PSNR | Luma PSNR | Luma RMSE | Mean Luma Ratio |
| --- | ---: | ---: | ---: | ---: |
| NEE, spp64 | 43.96 dB | 44.16 dB | 0.006197 | 1.000286 |
| ReSTIR N=4, spp64 | 44.01 dB | 44.23 dB | 0.006146 | 1.000115 |

此外，在同样 `spp64` 下直接比较 NEE 和 ReSTIR：

```text
NEE vs ReSTIR N=1:
RGB PSNR  = 52.26 dB
Luma PSNR = 52.77 dB
Luma ratio = 0.999922

NEE vs ReSTIR N=4:
RGB PSNR  = 41.71 dB
Luma PSNR = 41.87 dB
Luma ratio = 0.999829
```

这些结果说明：

- `N=1` 基本退化到 NEE，说明模式切换和基本权重公式没有明显问题。
- `N=4` 在 many-light / low-spp 场景下显著降低 variance。
- `N=4` 的 mean luminance ratio 接近 1，没有观察到明显系统性变暗或变亮。

## 当前结论

当前 no-reuse RIS direct lighting baseline 可以认为通过第一阶段 sanity check。

更具体地说，当前通过的是：

```text
Done: No-reuse RIS direct lighting baseline
```

尚未证明的是完整 ReSTIR DI。当前还没有：

- persistent per-pixel reservoir buffer；
- temporal reuse；
- spatial reuse；
- temporal rejection；
- neighbor reuse bias correction；
- reservoir debug views；
- HDR/EXR 级别的非 clamped validation。

## 后续建议

下一步建议进入 persistent reservoir 和 debug buffer 阶段，而不是马上写复杂 temporal reuse。推荐顺序：

1. 增加 GPU per-pixel reservoir buffer，并在 no-reuse 模式下写入当前帧 reservoir。
2. 增加 ReSTIR debug views：`reservoir-weight`、`reservoir-m`、`reservoir-target`、`restir-light-id`。
3. 在 camera static 的情况下验证 reservoir debug output 是否稳定。
4. 再开始 temporal reuse：上一帧 reservoir buffer、current/previous swap、camera reset、resolution reset。
5. temporal reuse 稳定后再做 spatial reuse。

在进入 temporal reuse 之前，建议保留当前 many-light no-reuse 实验作为 regression test：如果后续改动导致 `N=1` 不再接近 NEE，或 `N=4` 出现明显 mean luminance drift，应先回到 no-reuse 阶段修正。
