# Framework commit history

Source: local Git log ending at `d29419be990aa562bcbacabb17ac9c78b31a0908`.

Commit subjects describe changes; they do not prove builds or hardware checks.

```text
28b09a2f81b55d3cbf3ee9ad5ae379b2827d618c 2026-10-09 init: dev_fram HAL submodule - common, bus/dev, port_bsp, ring_buf, my_log.h
41a64d98ec497d6e9660669d84b8daf94e8c5d98 2026-10-09 feat: add board_support/kg200z with board-specific port implementations
fd4c89b49c0916a8454fdc1dcb26b749c0a1a355 2026-10-09 docs: add README.md
75bdcd6a5326b1b257cbdd6d62eec2d2d9d7a157 2026-10-09 1. 优化port头文件结构，将芯片hal文件移动进入support中
304d408c35304862485fe23a236a6f29ac1f9ad2 2026-10-09 Merge branch 'main' of github.com:sunlight-IT/dev_fram
1318c77ce56386f5ee129b54755c41b22b6a5ce2 2026-10-09 1. 优化tim驱动框架 2. 添加PWM设备
cfbe97fe66959d845d904d631f89b49e0eec3ce7 2026-10-10 1. 分离pwm与tim设备 2. 添加H7支持包
7eefd4f8d2f59a533f12360fb7a74d8914ff693f 2026-10-10 1. 更新h7支持包，以及common
ecdfa3c4f25e2bb5040e6ca5ac61c0b9e2b836ef 2026-10-10 更新readme
da3fcee71d7b01cc6129bbd22115d756a707b073 2026-10-10 refactor: remove kg200z board support package (moved to main project)
8ec515824a27e2338e24afdcab2e7a767c3ec3e5 2026-10-10 feat: add demo board support package (chip-independent stubs)
bde93b8a7bb8d1545842663cbb27b3190b5b3895 2026-10-10 1. 更新readme 2. 移除h7支持包
2ac426c53450a93fc99f1e07a5d16e582ed94d6a 2026-10-10 1. 添加定时器中断和串口中断 2. 由于存在rtos,需要使用TIM1作为系统时钟 而tim的初始化需要经过instance寻找，所以在msp中添加当定时器充当系统时钟时跳过
de60120590d0165fa135684b17a129316849c646 2026-10-10 1. 添加不需要定时器作为系统时钟时的条件编译
d29419be990aa562bcbacabb17ac9c78b31a0908 2026-10-10 1. 不同串口在不同板子对应的index不同，需要经中断服务函数放在it中实现
```
