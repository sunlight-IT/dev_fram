# 历史路径映射

来源：host `Usr/DEVICE_DRIVER_FRAMEWORK.md`、历史计划和源码路径；framework Git 历史与 README。
映射只帮助阅读历史，不能保证旧 symbol/API 仍存在。

| 历史位置 | 当前含义 |
| --- | --- |
| `Usr/drivers/bus/dev/` | 子模块 `bus/dev/` |
| `Usr/port/port_bsp/` | 子模块 `port_bsp/` |
| `Usr/common/usr_check.h` | 子模块 `common/usr_check.h` |
| `Usr/component/ring_buf.*` | 子模块 `component/ring_buf.*` |
| `Usr/port/usr_port*.h` | 子模块根 `usr_port*.h` |
| `Usr/port/board_support.*` | 真实板级支持由宿主提供；当前子模块只有 `board_support/demo/` |
| `Usr/hal/board_support/kg200z/`、`H743/` | 历史子模块路径，已不在当前基线 |
| `Usr/common/usr_common.h` | 宿主公共契约；子模块 demo 仅有示例 |
| `port_tim_semantic.*` | 已移除的历史计算器，不存在对应当前模块 |
| `docs/change_log/`、`docs/superpowers/` | 主工程后来迁至 logs/plans/specs；不是本库目录入口 |
| `build.ps1`、`flash.ps1` | 主工程旧命令，本库不提供宿主构建/烧录工具 |

历史原文中的断链不会伪装成现行可解析链接。当前文档和工具只链接本库内的文档；源码依据通过
来源清单中的原仓库、提交、路径和 hash 核对。
