# Telemetry Hub

最终可演示项目。Week 4 从 day24 起把稳定代码合并到这里。

建议布局：

```
include/     头文件（RingBuffer、协议、Hub）
src/         实现与 main
tests/       单测
```

目标链路：模拟传感器 → RingBuffer → 控制处理 → TCP 上报 → Ctrl+C 优雅退出。
