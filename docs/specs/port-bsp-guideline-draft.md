# Port and BSP guideline - review draft

> Authority: review draft, not an active rule.
> Source: S011, host `8ab7a95033b30b5848def22971a35bacab766645`,
> original path `docs/specs/2026-09-29-cross-project-port-bsp-guideline.md`.

Draft only. Preserves XLY-007..011 and PORT-001..017; excludes KG200Z mapping and broken source paths.

The source manifest preserves original file/blob hashes and selected ranges.
Old paths and commands describe the historical host, not this library.

## Source lines 13-23

<!-- SOURCE_FRAGMENT S011-F1 -->
## 1. 目的与边界

port 或板级支持包（BSP）是硬件资源所有权边界，负责将平台专属 API 和资源转换为
驱动与组合根使用的契约。设备 facade 通常通过驱动或应用能力抽象间接获得硬件能力，
而不是直接消费板级资源。

port 层 MAY 使用厂商 HAL、直接寄存器、内核 port API 或它们的组合。具体选择取决于平台，
但资源所有权边界和对外抽象边界必须保持稳定。port 契约面向驱动和组合根；设备 facade
通常通过驱动或应用能力抽象间接获得硬件能力，而不是直接消费板级资源。

port 层 MUST NOT 实现芯片协议状态机、应用策略、业务流程或设备专属寄存器语义。
<!-- END_SOURCE_FRAGMENT S011-F1 -->

## Source lines 67-198

<!-- SOURCE_FRAGMENT S011-F2 -->
### XLY-007 硬件边界

厂商头文件、HAL 类型、直接寄存器、物理引脚、IRQ 编号、DMA descriptor 和时钟门控操作
MUST 限定在 port/BSP 或明确保留的 legacy 平台层。

### XLY-008 公共接口泄漏

可复用驱动或 facade 消费的 port 接口 MUST NOT 暴露厂商类型、物理引脚标识、原始句柄或板级私有资源结构。

### XLY-009 显式所有权

每项硬件资源 MUST 有一个有记录的所有者或共享所有权协议。只写“平台拥有”是不足的，
还必须说明生命周期和仲裁机制。

### XLY-010 生命周期证据

port 资源 MUST 针对支持的平台 profile 定义初始化、使用、停止、反初始化、复位、睡眠、唤醒和恢复行为。

### XLY-011 平台差异

没有厂商 HAL、DMA、硬件片选、缓存或硬件 timeout MAY 改变实现方式，
但 MUST NOT 以此为理由将平台细节泄漏到可复用上层。

### PORT-001 Descriptor 与 Context 分离

不可变资源描述 MUST 与可变运行时状态分开。资源描述 SHOULD 包含物理能力和所有权元数据；
运行时 context SHOULD 包含状态、活动事务、错误、callback 和 producer/consumer 字段。

### PORT-002 资源完整性

资源记录 MUST 描述安全使用所需的全部资源，包括外设选择、时钟、GPIO、DMA、IRQ、电源/复位以及适用的能力限制。
缺失字段 MUST 在绑定或初始化阶段拒绝，不能延迟到后续空指针解引用。

### PORT-003 多实例

同一外设类别的多个实例 MUST 拥有独立运行时状态。只有在每次调用都能获得正确实例 context 时，
才 MAY 共享 operation table。

### PORT-004 资源冲突检测

涉及引脚、DMA 通道、IRQ vector、时钟、电源域或独占总线的冲突 MUST 在编译、绑定或初始化阶段检测。
禁止静默采用“最后写入者拥有”策略。

### PORT-005 初始化校验

port 初始化 MUST 在启动硬件前校验 context、资源 descriptor、必需 callback、句柄状态和能力假设。

### PORT-006 初始化回滚

如果 port 初始化在获取资源后失败，MUST 按依赖逆序撤销资源或记录明确的保留状态。重试 MUST NOT 继承含糊的部分初始化状态。

### PORT-007 幂等性

重复 `init`、`deinit`、`start`、`stop` 和 `recover` 的行为 MUST 有定义。操作 MAY 返回明确状态错误，
也 MAY 设计为幂等，但 MUST NOT 造成所有权不明确。

### PORT-008 活动事务关闭

反初始化 MUST 定义活动传输的处理方式。MUST 停止或取消 DMA 和中断、注销 callback、处理锁，
并根据 API 契约等待或明确放弃活动工作。

### PORT-009 时钟与电源所有权

时钟门控、电源域、复位线和低功耗迁移 MUST 有所有者。共享门控 MUST 使用引用计数、中央策略或其他显式机制；
单个设备 MUST NOT 禁用其他所有者仍在使用的资源。

### PORT-010 IRQ 所有权

每个中断 vector MUST 有单一 dispatch 所有者或明确的 dispatcher 协议。ISR 代码 MUST 只进行有界确认、
最小状态发布和向允许的 handler 分发。

### PORT-011 DMA Buffer 契约

每个 DMA buffer MUST 定义可寻址范围、对齐、大小、缓存策略、producer、consumer 和生命周期。
port MUST 说明 buffer 是静态、调用者提供、内存池拥有还是内核分配。

### PORT-012 DMA 同步

DMA 完成、半传输、错误和溢出语义 MUST 明确。硬件与前台代码共享内存时，port MUST 使用所需的原子操作、
内存屏障、缓存维护或临界区。

### PORT-013 Callback 上下文

port callback MUST 声明执行上下文和限制。port MUST NOT 从 ISR 调用 task-only callback，
也不得在没有明确适配器的情况下从 task 调用 ISR-only callback。

### PORT-014 错误映射

port MUST 将平台错误映射为稳定的上层错误契约，并保留恢复或诊断所需的区别。除非接口明确声明为平台专属，
原始 HAL 或厂商状态值 MUST NOT 穿过平台无关接口。

### PORT-015 Legacy 平台边界

平台需要时 MAY 保留 legacy 启动、vector、MSP 或生成初始化代码。项目 MUST 说明这些代码是拥有配置、
向 port 资源分发，还是仅保留 legacy 服务。新代码 MUST NOT 静默创建第二个所有者。

### PORT-016 协议隔离

port 代码 MUST NOT 决定芯片命令序列、寄存器含义、设备协议重试策略或应用级降级行为。

### PORT-017 能力报告

如果平台缺少所需能力，port MUST 通过稳定的 capability 或 unsupported 结果报告事实，
MUST NOT 使用未记录副作用模拟该能力。

## 5. 执行 profile

### 5.1 裸机协作式 profile

port MAY 暴露非阻塞 start/stop/process hook。中断处理器 MUST 发布有界状态，并将解析或复杂状态迁移延迟处理。

### 5.2 RTOS profile

port MAY 使用 mutex、semaphore、queue、event flag、task 或内核 DMA 集成。MUST 记录调度器状态要求、
ISR-safe API、锁所有权、timeout 行为和优先级反转处理。

### 5.3 无 HAL 或直接寄存器 profile

port MAY 直接使用寄存器定义。寄存器访问 MUST 保留在 port 边界内，且 port MUST 向上层提供相同的抽象契约和错误/生命周期语义。

## 6. 测试与验收清单

- [ ] 每项资源都有唯一所有者或明确共享协议。
- [ ] 能区分资源 descriptor 和可变 context。
- [ ] 多实例不会意外共享运行时状态。
- [ ] 已检测引脚、DMA、IRQ、时钟和电源冲突。
- [ ] 已测试初始化失败回滚。
- [ ] 已测试重复 init/deinit/reinit 行为。
- [ ] 已测试活动 DMA 和 IRQ 的关闭行为。
- [ ] 已记录并测试 callback 上下文和 buffer 生命周期。
- [ ] 已测试 timeout、busy、非法状态和硬件失败的错误映射。
- [ ] 构建/链接和硬件测试与主机/静态测试分别报告。
<!-- END_SOURCE_FRAGMENT S011-F2 -->
