# Historical semantic TIM/PWM evidence

> Authority: historical source excerpt, not current API or current verification.
> Source: S012, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/completions/2026-09-30-semantic-timer-pwm-resource-architecture.md`.

Historical software evidence only; capture and arbitration no longer describe current code. No new hardware verification inferred.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 3-10

<!-- SOURCE_FRAGMENT S012-F1 -->
## 任务范围

本次按 `docs/plans/2026-09-30-semantic-timer-pwm-resource-architecture.md`
实现应用层可理解的 UART、SPI、TIM、PWM 配置接口，并在 `Usr/port` 完成
STM32 HAL 参数换算、时钟选择、静态资源所有权、生命周期、IRQ 和回滚。
TIM2 继续专属于应用调度器的 1 ms 基本计数。新增了输入捕获事件环和
TIM-backed PWM 端口适配器，但当前板卡没有可由仓库事实确认的 PWM/捕获
引脚资源，因此没有伪造板级资源，也没有在启动流程中创建或启动它们。
<!-- END_SOURCE_FRAGMENT S012-F1 -->

## Source lines 34-133

<!-- SOURCE_FRAGMENT S012-F2 -->
## 事实依据

- 当前工程为 STM32WLE5JC 裸机工程，既有依赖方向为
  `Usr/app -> Usr/device -> Usr/drivers -> Usr/port -> HAL`。
- 当前 board resource table 只有已确认的 TIM2 scheduler row；PWM 和
  capture live resource count 为零。该限制来自现有板级映射，而不是为了
  通过测试添加的假引脚。
- `Core/Src/sys_app.c` 仍使用现有 `UTIL_TIMER_Init()` / `TIMER_IF` 时间基准；
  CMake 未引入 HAL TIM timebase template；`TIM2_IRQHandler` 仅由
  `Usr/port/port_bsp/port_hal_tim.c` 提供。
- TIM 计算器使用实际 timer clock，以 64 位中间值在 16/32 位计数器限制内
  计算 PSC/ARR；PWM 同一 TIM 的通道共享 PSC/ARR，并在频率更新失败时恢复
  旧寄存器和公开状态。
- UART、TIM 和 PWM 在回滚或清理失败时保留 claim 并进入 fault 状态，普通
  操作拒绝继续使用该资源；调用方可显式重试 `deinit`，只有 HAL 清理成功后
  才释放 claim 和复位状态。
- SPI 可复用接口的传输长度单位保持为字节；16-bit 模式由 port 校验偶数字节
  长度和缓冲区对齐，再换算为 STM32 HAL 所需的 frame count。
- `conflict_domain` 当前仅作为板级资源描述元数据。由于 live table 只有一个
  TIM2 row，本次不实现也不声称验证跨 descriptor 的全局冲突仲裁。

## 验证命令

已执行：

```powershell
git diff --check
python .\scripts\peripheral_resource_structure_test.py all
python .\scripts\timer_semantic_test.py
python .\scripts\bus_semantic_test.py
python .\scripts\bus_port_lifecycle_test.py
python .\scripts\timer_port_lifecycle_test.py
python .\scripts\build.py
```

文档元数据补齐后还需执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\docs-index.ps1 -Update
powershell -ExecutionPolicy Bypass -File .\scripts\docs-index.ps1 -Check
powershell -ExecutionPolicy Bypass -File .\scripts\check.ps1
```

硬件阶段待执行：

```powershell
python .\scripts\flash.py
```

并使用 MKLink 读取 RTT 及 TIM2 `CR1/DIER/SR/CNT/PSC/ARR`。

## 验证结果

当前已获得的结果：

- `git diff --check`: 退出码 `0`。
- 资源结构检查：`PASS: 25 resource ownership checks`。
- 定时器语义测试：`PASS: timer semantic calculator, capture ring, and claim policy`。
- 总线语义测试：`PASS: UART semantic validation and port capability boundary`。
- 总线生命周期测试：`PASS: UART/SPI HAL lifecycle and rollback behavior`。
- 定时器/PWM 生命周期测试：`PASS: TIM/PWM HAL lifecycle and rollback behavior`。
- ARM fresh build：`[BUILD] SUCCESS (0 errors, 119 warnings)`；产物尺寸为
  `text=258364`, `data=2004`, `bss=18044`，Flash 装载量为 `260368`
  bytes、余量为 `1776` bytes，并已生成非空
  `cmake-build/KG200Z_OPEN.elf/.hex/.bin`。警告包括重复出现的既有 RTC
  宏重定义、既有 unused/return/type/format 诊断，以及既有链接器 RWX
  LOAD segment 警告。

软件证据与硬件证据分开记录。以下项目在完成本轮硬件阶段后补入：

- 烧录命令退出码、`Verified OK`、`Resetting Target` 和 `[FLASH] SUCCESS`。
- MKLink RTT 启动/运行日志。
- TIM2 寄存器地址 `0x40000000`、`0x4000000C`、`0x40000010`、
  `0x40000024`、`0x40000028`、`0x4000002C` 的读取值及两次 CNT 差异。

## 未验证项目

- 尚未完成本轮目标板烧录、RTT 观察和 TIM2 寄存器读取。
- 由于没有确认的板级资源，物理 PWM 输出、物理输入捕获和 BOTH-edge
  运行行为未验证。
- `get_pwm_device()` 在当前板上返回 `NULL`；没有可用于真实 PWM 波形的
  live device。

## 剩余风险

- BOTH-edge 捕获在 ISR 中通过 GPIO IDR 推断边沿，快速翻转或 ISR 延迟时
  可能导致边沿分类不准确；必须在补充真实输入引脚后重新验证。
- Flash 余量为 1776 bytes；新增语义外设源文件使用局部 `-Os` 编译属性。
- 捕获事件环采用固定容量和 drop-new 策略，调用方必须读取状态中的
  `dropped_count` 和错误统计。

## 后续动作

- 完成本轮 flash、RTT 和 TIM2 寄存器观察，并把原始输出补充到本记录。
- 由只读审查代理按基线 `ff4a79469ee3b52f1d444b47ae0860c7575a9467`
  审查 tracked diff 和所有 untracked 文件；主代理修复 Critical/Important
  后重跑完整软件验证并关闭审查代理。
- 只有在获得真实 `.ioc`/原理图/板级引脚依据后，才新增 capture/PWM
  resource row 和对应硬件验证。

<!-- END_SOURCE_FRAGMENT S012-F2 -->
