# dev_fram

KG200Z 硬件抽象层（HAL）子模块，包含通用设备驱动接口、BSP 移植层及板级支持包。

## 目录结构

```
dev_fram/
├── common/                  通用类型与工具宏
│   ├── usr_common.h
│   └── usr_check.h
├── bus/dev/                 总线设备驱动接口（与 MCU 无关）
│   ├── dri_ops.h            驱动操作基类
│   ├── dev_gpio.c/h
│   ├── dev_i2c.c/h
│   ├── dev_pwm.c/h
│   ├── dev_spi.c/h
│   ├── dev_timer.c/h
│   └── dev_uart.c/h
├── port_bsp/                BSP 移植层（STM32 HAL 实现）
│   ├── port.h
│   ├── port_hal_clock.h
│   ├── port_hal_dma.c/h
│   ├── port_hal_gpio.c
│   ├── port_hal_i2c.c/h
│   ├── port_hal_pwm.c/h
│   ├── port_hal_spi.c/h
│   ├── port_hal_tim.c/h
│   ├── port_hal_uart.c/h
│   └── port_tim_semantic.c/h
├── board_support/           板级支持包（按板子区分）
│   └── kg200z/              KG200Z 板级资源定义
│       ├── board_support.c      GPIO/UART/SPI/I2C 引脚与时钟映射
│       └── usr_port_platform.c  平台初始化与系统复位
├── component/               独立组件
│   ├── ring_buf.c
│   └── ring_buf.h
├── log/                     日志（仅头文件）
│   └── my_log.h
├── board_support.h          板级 API 声明
├── usr_port.h               Port 层总头文件
└── usr_port_hal.h           Port HAL 层总头文件
```

## 层级依赖

```
board_support/<board>  ->  port_bsp  ->  bus/dev  ->  common
                      ->  common
                      ->  CubeMX 生成文件 (main.h, usart.h, gpio.h)
```

- `bus/dev/`：纯接口层，不依赖任何 MCU SDK
- `port_bsp/`：依赖 STM32 HAL 库，实现 `bus/dev/` 定义的接口
- `board_support/<board>/`：依赖 CubeMX 生成的句柄，定义具体引脚与时钟映射

## 添加新板子

在 `board_support/` 下新建目录，实现 `board_support.c` 和 `usr_port_platform.c`：

```
board_support/
├── kg200z/          <- 现有
└── kg200z_v2/       <- 新板子
    ├── board_support.c
    └── usr_port_platform.c
```

主工程切换板子时，修改 CMakeLists.txt 中的源文件路径和 include 路径即可。

## 作为子模块使用

```bash
# 主工程中添加
git submodule add git@github.com:sunlight-IT/dev_fram.git Usr/hal

# clone 主工程时自动拉取子模块
git clone --recurse-submodules <主工程仓库>

# 更新子模块到最新
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
