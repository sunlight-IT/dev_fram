# Historical compact TIM/PWM evidence

> Authority: historical source excerpt, not current API or current verification.
> Source: S013, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/completions/2026-10-09-compact-timer-pwm-port.md`.

Earlier software checks. Separate TIM/PWM adapters and later API changes are not covered by these results.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 1-66

<!-- SOURCE_FRAGMENT S013-F1 -->
# 2026-10-09 - 精简 TIM/PWM port 框架

## 任务范围

删除当前未使用的输入捕获路径、定时器 owner/claim 仲裁、PSC/ARR 量化搜索
和 PWM 频率/占空比反算。保留 `dev_timer`、`dev_pwm` 两个 facade，使用同一
个静态 `port_hal_tim` instance；每个 TIM resource 只能配置 BASIC 或 PWM
一种模式。

## 变更文件

- `Usr/hal/bus/dev/dev_timer.[ch]`、`dev_pwm.[ch]`、`dri_ops.h`
- `Usr/hal/port_bsp/port_hal_tim.[ch]`
- `Usr/hal/board_support/kg200z/board_support.c`
- `Usr/app/usr_app.c`、`Usr/app/usr_app_config.h`、`CMakeLists.txt`
- 定时器生命周期、静态配置和结构检查脚本
- 删除 `Usr/hal/port_bsp/port_hal_pwm.[ch]` 和
  `Usr/hal/port_bsp/port_tim_semantic.[ch]`

## 事实依据

- 当前 board resource 只确认 TIM2 scheduler，没有确认的 PWM/capture 引脚。
- TIM2 参数为 `timer_clock_hz=48000000`、`prescaler=0`、
  `auto_reload=47999`，对应原有 1 ms 更新中断。
- 应用调用链使用 `dev_timer_init(get_timer_device())`；PWM facade 使用
  `dev_pwm_set_compare()` 更新原始 CCR 计数。

## 验证命令

```powershell
python -B .\scripts\peripheral_resource_structure_test.py all
python .\scripts\timer_port_lifecycle_test.py
python .\scripts\timer_semantic_test.py
python .\scripts\build.py
git diff --check
git -C Usr/hal diff --check
```

## 验证结果

- 结构检查：`PASS: 25 resource ownership checks`。
- 生命周期测试：`PASS: compact TIM/PWM lifecycle`。
- 静态配置测试：`PASS: static TIM/PWM configuration contract`。
- `build.py`: `[BUILD] SUCCESS (0 errors, 148 warnings)`，生成非空
  `cmake-build/KG200Z_OPEN.elf/.hex/.bin`。
- 构建警告包含既有 RTC 宏重定义、UART enum/未使用诊断和链接器 RWX
  LOAD segment；没有编译错误。

## 未验证项目

- 未烧录目标板，未读取 TIM2 实际寄存器或 RTT。
- 当前板没有确认的 PWM 引脚，因此没有物理 PWM 波形验证。
- 输入捕获已删除，未来重新添加时需要单独设计和验证。

## 剩余风险

- `timer_clock_hz` 是板级静态声明；时钟树或板卡变化时必须同步更新。
- 旧 UART/SPI 生命周期夹具仍包含旧接口类型假设，不能作为本次 TIM/PWM
  重构的验证证据。
- `port_hal_uart.c` 有并发未提交改动，本次未修改该文件。

## 后续动作

- 获得真实 PWM 引脚/AF/定时器资源依据后，再添加 PWM board row 并进行波形验证。
- 目标板验证时单独记录 flash programming、RTT 和寄存器/波形证据。

<!-- END_SOURCE_FRAGMENT S013-F1 -->
