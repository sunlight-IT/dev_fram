# Historical multichannel PWM evidence

> Authority: historical source excerpt, not current API or current verification.
> Source: S014, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/completions/2026-10-09-multichannel-pwm.md`.

Old document used channels 1..4; current enums are 0..3. No physical PWM waveform verification was recorded.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-45

<!-- SOURCE_FRAGMENT S014-F1 -->
# 2026-10-09 - 定时器多通道 PWM

## 任务范围

将精简 TIM/PWM 框架扩展为一个 `pwm_device` 控制定时器上的多路 PWM。
每组最多 4 个通道，共享 timer clock、PSC 和 ARR；每个通道单独配置 GPIO、
AF、极性和初始 CCR。未添加未经确认的实际板级 PWM 引脚。

## 接口和行为

- `dev_pwm_set_compare(dev, channel, compare)` 接收逻辑通道号 `1..4` 和原始
  CCR 计数值，只更新目标通道；compare 上限为 `ARR + 1`。
- 静态通道表保存逻辑/HAL 通道映射、初始 CCR、GPIO/AF 和极性。重复或不匹配
  的映射及非法参数在 HAL/GPIO 副作用前拒绝。
- `start/stop` 对整组通道生效。初始化或启动中途失败时回滚已执行的资源；
  回滚失败进入 `FAULT`。PWM deinit 只允许在 `READY` 状态执行。
- TIM2 维持 BASIC 模式、48 MHz、PSC 0、ARR 47999；当前
  `get_pwm_device()` 仍返回 `NULL`。

## 验证命令

```powershell
python .\scripts\timer_port_lifecycle_test.py
python -B .\scripts\timer_semantic_test.py
python -B .\scripts\peripheral_resource_structure_test.py all
python .\scripts\build.py
git diff --check
git -C Usr/hal diff --check -- README.md bus/dev/dev_pwm.c bus/dev/dev_pwm.h bus/dev/dri_ops.h port_bsp/port_hal_tim.c port_bsp/port_hal_tim.h board_support/kg200z/board_support.c
powershell -ExecutionPolicy Bypass -File .\scripts\docs-index.ps1 -Update
powershell -ExecutionPolicy Bypass -File .\scripts\docs-index.ps1 -Check
```

## 验证结果

- 生命周期测试：`PASS: compact TIM/PWM lifecycle`。
- 静态配置测试：`PASS: static TIM/PWM configuration contract`。
- 资源结构检查：`PASS: 25 resource ownership checks`。
- `build.py`：`[BUILD] SUCCESS (0 errors, 77 warnings)`，生成 ELF、HEX、BIN。

## 未验证项目和剩余风险

- 未烧录目标板，未做 RTT、定时器寄存器、GPIO 复用或 PWM 波形测试。
- 当前板没有已确认 PWM 定时器/引脚/AF 组合，因此没有真实板级 PWM resource。
- 增加板级 PWM 输出前需确认可用 TIM、每路 GPIO/AF、引脚冲突和定时器输入时钟。

<!-- END_SOURCE_FRAGMENT S014-F1 -->
