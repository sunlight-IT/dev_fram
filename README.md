# dev_fram

设备驱动框架子模块，包含总线接口、STM32 HAL adapter、公共 helper、ring buffer 和日志声明。
真实板级资源及厂商 SDK 由宿主工程提供；当前子模块只有 demo 桩，不声称已完成独立构建或所有 MCU 的运行验证。

AI 修改框架前读取 [AGENTS.md](AGENTS.md) 与 [文档导航](docs/README.md)。
文档库随本子模块版本迁移，保存当前状态、接口快照、现行规则、规范草案及有源头的历史记录。
资料基线和来源 hashes 见 [来源清单](scripts/docs-sources.json)。

## 目录结构

```
dev_fram/
├── common/                  通用工具宏
│   └── usr_check.h
├── bus/dev/                 总线设备驱动接口（与 MCU 无关）
│   ├── dri_ops.h            驱动操作基类
│   ├── dev_gpio.c/h
│   ├── dev_i2c.c/h
│   ├── dev_can.c/h
│   ├── dev_pwm.c/h
│   ├── dev_spi.c/h
│   ├── dev_timer.c/h
│   └── dev_uart.c/h
├── port_bsp/                BSP 移植层（STM32 HAL 实现）
│   ├── port.h
│   ├── port_hal_clock.h
│   ├── port_hal_dma.c/h
│   ├── port_hal_gpio.c/h
│   ├── port_hal_i2c.c/h
│   ├── port_hal_can.c/h
│   ├── port_hal_spi.c/h
│   ├── port_hal_tim.c/h     TIM basic adapter
│   ├── port_hal_pwm.c/h     independent PWM adapter
│   └── port_hal_uart.c/h
├── board_support/           板级支持包（仅 demo 桩实现）
│   └── demo/                无真实资源的示例桩；不代表完整独立构建
│       ├── board_support.c
│       ├── board_support.h
│       ├── usr_common.h
│       └── usr_port_platform.c
├── component/               独立组件
│   ├── ring_buf.c
│   └── ring_buf.h
├── log/                     日志（仅头文件）
│   └── my_log.h
├── AGENTS.md                AI 入口与生成文档索引
├── rules/                   现行框架编码与文档规则
├── docs/                    specs/plans/logs/completions
├── scripts/                 文档检查、来源与 ZIP 导出
├── tests/                   文档工具与 HAL 无关的公共驱动测试
├── usr_port.h               Port 层总头文件
└── usr_port_hal.h           Port HAL 层总头文件
```

## 层级依赖

```
宿主 BSP 绑定 bus device 的 ops/context/resource
bus/dev -> ops(ctx) -> port_bsp -> HAL/hardware
port_bsp -> 宿主 board_support.h 和资源查询函数
bus/dev -> 宿主 usr_common.h
```

- `bus/dev/`：总线接口与封装，通过 `dri_ops.h` 使用宿主 `usr_common.h` 契约
- `port_bsp/`：依赖 STM32 HAL 库，实现 `bus/dev/` 定义的接口
- 实际 BSP：由宿主提供，定义资源与绑定，并按所用 SDK 接入句柄、MSP、IRQ、时钟和系统服务

## 添加新板子

在主工程的 `Usr/board_support/` 下新建目录，实现 `board_support.c`、`board_support.h`、`usr_common.h` 和 `usr_port_platform.c`：

```
Usr/board_support/
└── new_board/       <- 宿主新增的板级支持
    ├── board_support.c
    ├── board_support.h
    ├── usr_common.h
    └── usr_port_platform.c
```

切换板子前核对 [宿主接入契约](docs/specs/host-integration.md)，包括公共类型、资源查询、
日志实现、MSP/vector 唯一所有权和系统 timebase；再调整宿主源文件与 include 路径。
当前 TIM2 hook、固定实例索引、demo 声明差异等限制见 [当前架构](docs/specs/current-architecture.md)。

## 文档检查与打包

从子模块根目录运行：

文档工具使用 Python 3.10+ 标准库，无需安装额外依赖。

```powershell
python scripts/docs-library.py update
python scripts/docs-library.py check
python tests/docs_library_test.py
python scripts/docs-library.py export
```

导出 `exports/dev-fram-docs-日期-源码基线.zip` 和 `.zip.sha256`；ZIP 内有逐文件
`SHA256SUMS`。包不含 `.git`、固件源码/产物、宿主固件验证脚本或本机配置。
解压后无需原工程或 Git 即可执行文档检查；重新导入历史需原 Git 对象。
规范草案仍待审查，历史验证不能替代新工程验证。

## 作为子模块使用

```bash
# 主工程中添加
git submodule add git@github.com:sunlight-IT/dev_fram.git Usr/hal

# clone 主工程时自动拉取子模块
git clone --recurse-submodules <主工程仓库>

# 恢复宿主锁定的子模块提交
git submodule update --init Usr/hal

# 有意升级时才更新，并重新核对/验证接口
git submodule update --remote Usr/hal
```

## 在子模块中修改并推送

```bash
cd Usr/hal
git add -A
git commit -m "fix: SPI DMA 超时处理"
git push origin main

# 回到主工程，更新子模块指针
cd ../..
git add Usr/hal
git commit -m "chore: update dev_fram submodule"
```
