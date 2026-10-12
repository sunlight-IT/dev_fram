# Reusable bus guideline - review draft

> Authority: review draft, not an active rule.
> Source: S010, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/specs/2026-09-29-cross-project-device-driver-guideline.md`.

Draft only. Preserves XLY-001..006 and DRV-001..015 rule IDs as bus-applicable guidance; excludes device-protocol sections and old host mappings.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 63-77

<!-- SOURCE_FRAGMENT S010-F1 -->
### 4.1 总线驱动

总线驱动提供字节流 I/O、寄存器事务、帧、片选生命周期、GPIO 或时间等传输能力。

总线驱动 SHOULD 负责：

- 事务边界和事务所有权。
- 传输超时处理。
- 传输层恢复。
- 地址宽度、传输模式等能力检查。
- 锁，或明确记录的外部加锁契约。
- buffer 容量和部分传输语义。

总线驱动 SHOULD NOT 解释某个芯片的寄存器映射或业务协议。

<!-- END_SOURCE_FRAGMENT S010-F1 -->

## Source lines 94-207

<!-- SOURCE_FRAGMENT S010-F2 -->
### XLY-001 依赖方向

可复用驱动 MUST 依赖抽象或平台无关契约，不能依赖具体板卡、MCU 外设、厂商 HAL 或物理引脚。

### XLY-002 显式依赖

硬件、时间、同步、存储和日志依赖 MUST 通过显式注入、上下文传递或已记录的平台接口提供。
正常使用 MUST NOT 依赖隐式全局查找。

### XLY-003 所有权

每个可变状态对象和异步 buffer MUST 有明确记录的所有者、生命周期和访问上下文。

### XLY-004 执行上下文

每个 callback 或完成函数 MUST 明确运行于 ISR、task、thread、deferred-work 或协作式主循环上下文。
未声明 callback 上下文 MUST 视为接口缺陷。

### XLY-005 错误语义

当调用者能够观察到这些状态时，接口 MUST 区分“请求已接受”“操作已完成”“忙”“未就绪”、
“超时”和“失败”。

### XLY-006 验证边界

主机测试、静态检查、固件构建、烧录校验和硬件运行观察 MUST 作为不同证据类型分别报告。

### DRV-001 硬件隔离

驱动源码 MUST NOT 引入 MCU 厂商头文件，也不得调用 MCU 外设 HAL 或平台寄存器 API。
平台适配器 MUST 将驱动契约转换为硬件 API。外部设备协议寄存器事务只能通过注入的总线契约执行，
并仍由芯片驱动负责。

### DRV-002 禁止物理资源硬编码

驱动代码 MUST NOT 硬编码外设实例、GPIO 端口/引脚、DMA 通道/请求、IRQ 编号、时钟源或板级角色名。

### DRV-003 显式接口契约

每个公开驱动 API MUST 定义输入范围、指针可空性、零长度行为、输出容量、单位、字节序、对齐要求，
以及每个输入/输出 buffer 的所有权。

### DRV-004 配置分离

配置、不可变设备描述和可变运行时状态 SHOULD 分开表示。运行时操作 MUST NOT 静默修改调用者的配置对象或平台句柄。

### DRV-005 状态机

具有生命周期或异步行为的驱动 MUST 定义状态、合法迁移、重复调用行为和失败迁移。
驱动 MUST 拒绝当前状态下非法的操作。

### DRV-006 初始化回滚

如果初始化包含多个对外可见步骤，失败后 MUST 使驱动处于已记录状态，并 MUST 释放或撤销本次尝试获取的资源。

### DRV-007 重试安全

重试 MUST 有上限，并 MUST 根据操作语义选择。没有协议级安全保证时，非幂等写操作 MUST NOT 重试。

### DRV-008 超时预算

API MUST 定义 timeout 适用于一次传输、整个事务还是完整重试预算。实现 MUST NOT 通过无上限重试静默放大调用者声明的 timeout。

### DRV-009 错误映射

驱动 MUST 保留参数错误、传输错误、协议/数据错误、设备不可用、忙/未就绪和 buffer 错误之间的区别。
平台专属错误细节 MAY 保留在诊断信息中，但公开驱动契约 MUST 保持可移植。

### DRV-010 Callback 安全

ISR callback MUST NOT 阻塞、执行无上限内存分配、进行协议解析或调用未明确支持 ISR 的 API。
重工作 MUST 延迟到 task、worker 或协作式处理路径。

### DRV-011 Buffer 生命周期

异步 API MUST 说明输入 buffer 是被复制、借用至完成，还是由调用者在整个操作期间持有。
驱动 MUST NOT 在记录的生命周期之外保留借用 buffer。

### DRV-012 并发

如果驱动不可重入，其公开接口 MUST 说明该事实，并定义所需的外部锁或所有者。
`volatile` MUST NOT 替代原子性、锁或内存顺序。

### DRV-013 反初始化

反初始化前，驱动 MUST 停止或取消异步活动、禁用或注销自己拥有的 callback，
并按照已记录策略处理活动事务。

### DRV-014 日志隔离

可复用驱动头文件 MUST NOT 强制依赖项目专属 logger。日志 SHOULD 使用可选 sink、注入接口、编译期适配器，
或由调用者处理的事件。使用 ISR 日志前 MUST 明确支持该行为。

### DRV-015 测试可替换性

驱动 SHOULD 可以使用 fake bus、time、GPIO、storage 和 synchronization backend 测试。
协议测试 MUST 覆盖传输失败、格式错误和无效设备数据，不能只覆盖成功路径。

## 6. 并发与执行 profile

驱动契约 MAY 使用以下任一种 profile 实现：

- 裸机协作式 `poll` 或 `process` 调用。
- 独立 RTOS task。
- event 或 callback 完成。
- DMA 完成后进入 deferred work。
- 同步阻塞操作。

`process()` 不是通用接口要求。选定的 profile MUST 说明 ISR、task、thread 或主循环上下文允许调用哪些 API，
以及总线锁是内部提供还是由调用者提供。

对于 DMA 或 ring-buffer 设计，契约 MUST 定义 producer/consumer 同步、原子字段、必要的内存屏障、覆盖或丢弃策略，
以及 overflow counter 的确切含义。

<!-- END_SOURCE_FRAGMENT S010-F2 -->
