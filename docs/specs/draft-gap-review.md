# 规范草案条款核查

两份规范继续为审查草案。本表是初始静态核对，不是全面合规审计或新规则批准。
基线 `d29419b`；来源为原两份规范、当前 ops/adapter/common/ring 源码。
“未核查”必须在后续相应任务中获得证据后才可升级。

| 条款 | 初始状态 | 当前证据或限制 |
| --- | --- | --- |
| XLY-001 | 存在差距 | bus 依赖宿主 usr_common，固定实例 enum/表；未证明完全平台独立 |
| XLY-002 | 存在差距 | ops/context 注入存在，同时有静态 getter 和宿主查询依赖 |
| XLY-003 | 未核查 | resource/instance 分离可见，全部 buffer/lifetime 合约未审计 |
| XLY-004 | 存在差距 | callback 类型和 IRQ 路径存在，所有 API 执行上下文未完整声明 |
| XLY-005 | 存在差距 | USR_BUSY=1；宿主旧注释将非负统称成功，不能复制该歧义 |
| XLY-006 | 当前满足 | 本库将静态、构建、编程、运行证据分开记录；不声称固件合规 |
| DRV-001 | 未核查 | ops 头依赖 usr_common；需查完整间接 include 后确认 |
| DRV-002 | 存在差距 | bus 有固定实例索引；物理 resource 是否全部隔离须另审 |
| DRV-003 | 存在差距 | 真实签名已归档，null/容量/上下文等契约尚未完整声明 |
| DRV-004 | 当前满足 | 多数总线使用 config、ops、ctx 分离；仅确认这种结构模式 |
| DRV-005 | 未核查 | TIM/PWM 生命周期 enum 可见，合法迁移需逐实现验证 |
| DRV-006 | 未核查 | adapter 有回滚路径，所有初始化失败路径未动态审计 |
| DRV-007 | 未核查 | I2C 部分重试代码已注释，不宣称重试保障 |
| DRV-008 | 未核查 | timeout 参数存在，事务/重试总预算未完整定义 |
| DRV-009 | 未核查 | 使用 USR 状态映射；所有错误区别需逐接口核对 |
| DRV-010 | 未核查 | 存在 ISR callbacks；上下文安全未完成审计 |
| DRV-011 | 未核查 | UART DMA/ring 有共享 storage；借用 buffer 合约未完整核对 |
| DRV-012 | 存在差距 | in_use/volatile 不是 RTOS 锁或线程安全证明 |
| DRV-013 | 未核查 | deinit 接口与实现存在；活动事务停止策略需核查 |
| DRV-014 | 存在差距 | common/logger 仍依赖宿主公共头及日志实现 |
| DRV-015 | 未核查 | 旧 host 夹具不代表当前接口已可脱离 MCU 验证 |
| XLY-007 | 未核查 | vendor 主要在 adapter/宿主；需核查间接公共头泄漏 |
| XLY-008 | 未核查 | ops 以 void ctx 隔离；全部公共头和宿主合约需审计 |
| XLY-009 | 存在差距 | TIM2 强定义及 SYSTEM_TIMER 特例需与宿主明确所有权 |
| XLY-010 | 未核查 | init/start/stop/deinit 存在，睡眠/恢复等 profile 未核查 |
| XLY-011 | 未核查 | demo 非完整跨平台构建证明；当前 HAL adapter 不能推广所有平台 |
| PORT-001 | 当前满足 | UART/SPI/I2C/TIM/PWM resource 与 instance 类型分开 |
| PORT-002 | 未核查 | descriptor 字段存在，能力/资源完整性需逐平台验证 |
| PORT-003 | 未核查 | 静态 instance/handle 表存在，隔离性未实测 |
| PORT-004 | 存在差距 | 旧全局 claim 仲裁已移除，无全面冲突检测证明 |
| PORT-005 | 未核查 | 局部 valid/helper 存在，全部初始化前条件未审计 |
| PORT-006 | 未核查 | PWM/UART 失败清理路径存在，当前版本未执行回滚测试 |
| PORT-007 | 未核查 | 生命周期重复调用行为需逐接口核对 |
| PORT-008 | 未核查 | DMA/IRQ 关闭和 callback 清理需硬件/生命周期证据 |
| PORT-009 | 未核查 | clock callback 注入存在，共享时钟仲裁未全面核对 |
| PORT-010 | 存在差距 | TIM2 vector 和 HAL callback 强定义，宿主集成可能重叠 |
| PORT-011 | 未核查 | ring storage 类型可见，cache/地址范围依平台验证 |
| PORT-012 | 存在差距 | ring 使用 DMB/compiler barrier；无任意 RTOS/cache 安全证明 |
| PORT-013 | 存在差距 | 完成 callbacks 上下文及约束未完整公开 |
| PORT-014 | 未核查 | HAL-to-USR 映射存在，错误覆盖未全面核对 |
| PORT-015 | 存在差距 | 系统 TIM 仅部分路径特殊处理，需宿主列出 legacy owner |
| PORT-016 | 未核查 | 本次未发现新增协议策略，但未做全子模块逐函数审计 |
| PORT-017 | 未核查 | 不能把 demo NULL/no-op 等同统一 unsupported 能力报告 |

本表不包含 facade/芯片协议规则。所有“当前满足”仅确认表内描述的静态文档或结构事实，
不表示整条规范的全部运行行为已验证。未来批准条款需单独记录批准人、范围、版本及合规证据。
