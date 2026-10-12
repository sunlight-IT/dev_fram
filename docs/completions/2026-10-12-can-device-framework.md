# 2026-10-12 - CAN/FDCAN 设备框架

## 范围与版本

- 宿主基线：`68d328f20128e9d94beb95033c9f608f167f8264`，开始时已有
  `Core/Src/fdcan.c` 未提交修改。
- 框架基线：`bb9c659fff03de48d35682f949bbed8b31e5473c`，开始时干净。
- 本记录描述当前未提交工作树；不代表框架子模块已经发布。
- `STM32H743.ioc` 已由维护者声明弃用，本次未修改、未把其中配置作为实现事实，
  也未把它纳入验收。

## 实现

- `bus/dev/dev_can.[ch]` 提供 HAL 无关的 Classic CAN/CAN FD frame、config、filter、status
  以及 `init/deinit/send/recv/get_status`。
- `port_bsp/port_hal_can.[ch]` 保存 FDCAN handle、生命周期、DLC 转换、完整帧 SPSC 队列、
  错误统计、MSP 和 HAL callback 分发。
- mode、Message RAM offset/capacity、各 RX/TX 区域数量和元素大小、TX FIFO/Queue mode、
  filter index/destination、RX location 和未匹配帧策略全部由宿主 resource 提供。
- adapter 在 HAL init 前校验 RX route、filter destination、元素容量和 Message RAM 预算；
  轮询与中断模式使用同一 RX location，支持 FIFO0、FIFO1 和 dedicated RX buffer。
- H743 宿主当前绑定 FDCAN1、PA11/PA12、PLL1Q 32 MHz、Classic CAN 500 kbit/s、
  CAN FD data phase 2 Mbit/s、标准 ID `0x321` 精确 mask filter、16 帧软件 RX 队列。
- 旧 `Usr/component/can.[ch]` 裸字节实现及重复 HAL callback 已移除。

## 验证

实际执行并通过：

```powershell
python Usr/hal/tests/port_hal_can_resource_test.py
python Usr/hal/tests/docs_library_test.py
python tool/build.py all
python tool/build.py all --mode Release
git diff --check
git -C Usr/hal diff --check
```

- `dev_can_test.c` 使用 MSYS2 UCRT64 GCC 严格编译并执行，输出 `dev_can_test: PASS`。
- resource 映射测试 1 项通过；文档工具测试 7 项通过。
- Debug 链接通过：FLASH `202804 B`，最终 size text/data/bss 为
  `202516/280/62979`。
- Release 链接通过：FLASH `110096 B`，最终 size text/data/bss 为
  `109844/244/62967`。
- `arm-none-eabi-nm` 确认 CAN public API、FDCAN MSP、FIFO0/FIFO1/RX buffer callback
  都链接进 Debug ELF，且 `hfdcan1` 只有一个全局存储。
- 构建仍报告已有的 core/I2C/UART/TIM/PWM 警告；新增 CAN 源文件无新增警告。
- `git diff --name-only` 未包含 `STM32H743.ioc`。

## 未验证

- 未执行 flash，因此没有编程验证证据。
- 未连接 CAN 收发器或分析仪；500 kbit/s 收发、ACK、`0x321` 过滤、FIFO burst、
  queue overflow、bus-off、deinit/reinit 和 CAN FD/BRS 均未做硬件观察。
- PA11/PA12 同时存在于保留的 USB FS 生成源码中；当前启动链不初始化 USB，宿主必须保持
  CAN 与 USB FS 互斥，除非后续重新分配物理引脚。
