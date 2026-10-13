# 2026-10-13 - SPI 裸机多模式收发

## 范围

- SPI adapter 删除 FreeRTOS mutex、semaphore、task owner、scheduler 判断和 ISR task
  notify；宿主工程整体仍保留 FreeRTOS。
- `spi_device_config_t` 的 TX/RX 分别选择 polling、interrupt 或 DMA；设备 init 后模式固定。
- RX/TX DMA 资源按方向独立，支持仅 RX DMA、仅 TX DMA 和无 DMA。
- 保留同步公开 API、CS 多帧事务、8/16 bit frame 转换、EOT 完成和 fault 隔离。
- 未提交、未 push、未烧录；目标板验证由用户执行。

## 传输语义

| 传输 | 实现 |
| --- | --- |
| 仅 TX / 仅 RX | 按方向 mode 选择 HAL polling、IT 或 DMA API |
| TX/RX 非等长 | TX 后 RX，两阶段共享总 timeout |
| 等长 DMA+DMA | `HAL_SPI_TransmitReceive_DMA` |
| 等长 IT+IT | `HAL_SPI_TransmitReceive_IT` |
| 等长其他七种组合 | 阻塞 `HAL_SPI_TransmitReceive` |

混合全双工回退不启动 DMA，也不做 DMA buffer 检查。TX polling 后 RX DMA 是合法的
两阶段命令读取流程；同一全双工阶段不支持 RX DMA 与阻塞 TX 并行。master RX-only
可由 HAL 产生 SCK，但 MOSI dummy 值无协议保证，需要固定 dummy 时使用显式 TxRx。

## 裸机状态与恢复

- adapter 只用短 PRIMASK 临界区保护 claimed 与前台/ISR 状态交换，DMB 发布终态；传输
  期间不关闭中断。claimed 非递归，不提供 RTOS 多任务互斥。
- 所有事务要求 `IPSR/PRIMASK/BASEPRI/FAULTMASK` 均为 0；`timeout_ms=0` 不启动传输。
- IT/DMA 同步等待 `HAL_GetTick()`；无符号差值覆盖 tick 回绕。不等长两个阶段共享预算。
- DMA TC 不是成功终点，只有 SPI EOT 对应的 HAL completion callback 发布成功。
- 超时/错误先发布 ABORTING，再执行阻塞式 `HAL_SPI_Abort()`，恢复预算从首次错误或超时
  起累计。HAL abort 返回后若总耗时已达到 10 ms，则进入 FAULTED；若 HAL error 已触发
  DMA 异步 abort，后续收敛轮询也受同一预算约束。由于 HAL abort 本身不可中断，这不是
  “函数保证在 10 ms 内返回”的墙钟硬截止。
- 返回 IDLE 前确认 SPI READY、已用 DMA stream EN=0、DMA handle READY/RESET，并清
  SPI/DMA flags 与 NVIC pending。失败进入 FAULTED，仅 deinit/reinit 可恢复。
- ABORTING 下的普通完成回调、重复回调和迟到回调不会发布成功或污染下一事务。

## 宿主改动

- 删除 `Usr/board_support/H743/usr_spi_sync.[ch]` 及 CMake 条目。
- 新增 `usr_spi_dma_policy.[ch]`；只验证实际 DMA 方向的地址范围和 cache policy；
  8/16 bit 长度与地址对齐由 adapter 在调用 board policy 前检查。
- SPI2/SPI3/SPI4 默认保持 DMA+DMA，并通过 `usr_spi_irq` 路由 SPI EOT 与 DMA IRQ。
- H743 policy 接受 AXI/D2/D3 SRAM，TX 另接受 Flash；拒绝 DTCM、RX Flash、越界、
  零长度、NULL 以及 D-Cache 开启状态。地址对齐由 adapter 在 policy 前检查。

## Host 测试

```powershell
python Usr/hal/tests/spi_test.py --cc D:/Tool/mingw/bin/gcc.exe --host-root . --build-dir build/host-spi-tests-final
```

测试覆盖配置枚举与 resource 匹配、TX-only/RX-only 三种模式、等长 3x3 矩阵、非等长
分阶段组合、单方向/无 DMA、8/16 bit、DMA start/partial init failure、EOT、error、timeout、
abort、FAULTED、上下文拒绝、非递归 claim、tick 回绕和共享 timeout。HAL 与 Cortex 状态
均为 host stub，不能替代目标板时序。最终命令 exit code 0，`dev_spi_test`、
`port_hal_spi_test`、`spi_dma_policy_test` 均 PASS。

实现审查后增加了 polling timeout/error 统一恢复、begin/end 上下文拒绝、HAL start
窗口中的迟到 callback 隔离、DMA error IRQ 起算的恢复预算测试，以及纯 polling resource
恢复期间不得操作 SPI IRQ 的回归测试。claim、is_owner 和 release 均覆盖四种非法上下文。
error callback 和 SPI IRQ error 都先发布 ABORTING，避免等待完整事务 timeout 后才恢复。

## 固件与静态验证

```powershell
python tool/build.py all
python tool/build.py all --mode Release
python tool/build.py clangd-check
```

- Debug：exit code 0，FLASH `221316 B`、RAM `45432 B`、RAM_D2 `19075 B`。
- Release：exit code 0，FLASH `119688 B`、RAM `45384 B`、RAM_D2 `19075 B`。
- clangd-check：exit code 0。
- 文档库：`check` 报告 `43 files, 67 sources` PASS，7 项文档测试 OK。
- diff 检查的最终结果在完成前重新执行并以命令输出为准。
- 构建仍报告 `Usr/core.c`、TIM/PWM、UART 和 board const qualifier 等既有告警；本次未扩展
  范围处理。

## 未验证

- 未烧录、未执行目标板 runtime 验证。
- 用户需验证 TX polling -> RX DMA、RX-only DMA 的 SCK/MOSI、DMA+DMA 与 IT+IT 全双工、
  混合模式阻塞回退、SPI4 FRAM、多帧 CS、timeout recovery 和 DTCM/cache buffer 拒绝。
- framework 与宿主工作树均 dirty；本记录不把未提交源码表示为固定 commit/blob。
