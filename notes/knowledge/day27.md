# Day27 · 发布检查与 Hub 打磨（详细预习）

日期：2026-09-13

预习目标：先学会用 `ldd`/`strip` 和一键 `run.sh` 把程序交得出去，再补 README、架构图和简单断线重连。上机先把发布命令跑通，再写文档和重连。

四天地图见 [day24-27-hub.md](day24-27-hub.md)。

---

# 一、Linux：`ldd`、`strip`、一键 `run.sh`

别人（或明天的你）要能在干净目录里把 Hub 跑起来。发布侧三件套：**依赖看见、二进制可瘦身、一条脚本从配置到跑。**

## 1. `ldd`：这个可执行文件依赖哪些 .so

```bash
ldd ./build/hub_server
```

典型几行：`linux-vdso`、`libstdc++.so`、`libm`、`libc`、`libasan`（若是 ASan 构建）。认：

- **路径存在**：动态链接器找得到。
- **`not found`**：这台机器缺库，拷二进制过去会立刻失败。发布说明里要写「需要 g++ 运行时 / 在 Ubuntu xx」。
- **ASan 的库**：Debug+ASan 的二进制不要当「发布版」给人。Release 再 `ldd` 一次，应没有 `libasan`。

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel
ldd ./build-rel/hub_server
```

`ldd` 是看**动态**依赖。静态链进去的看不到。面试：`ldd` 检查运行时依赖，不是列出所有编译期库。

不要对不信任的二进制随便 `ldd`（极老环境有过加载副作用）。自己编的没问题。

## 2. `strip`：去掉符号，体积变小

```bash
ls -lh build-rel/hub_server
cp build-rel/hub_server /tmp/hub_server.unstripped
strip build-rel/hub_server
ls -lh build-rel/hub_server
```

`strip` 删调试符号。体积下降，`gdb` 的函数名会变差。所以：

- 发布给演示、嵌入式盘小：可以 strip。
- 自己留一份 **unstripped** 或单独 `-g` 的 debuginfo，崩了才能 `bt`。
- **不要 strip 还开着 ASan 的 Debug 包**当日常调试用。

对照：`file hub_server` 能看到 stripped / not stripped。

## 3. `run.sh`：别人只想敲一行

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$ROOT"

PORT=${1:-7777}
BUILD=${BUILD_DIR:-build-rel}

if [[ ! -x $BUILD/hub_server ]]; then
  cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$BUILD"
fi

echo "LISTEN check after start: ss -lntp | grep $PORT"
exec "$BUILD/hub_server" "$PORT" --log="${HUB_LOG:-info}"
```

再给 client 一个 `run-client.sh`，或同一脚本 `./run.sh server|client`。要点：

- `set -euo pipefail`：出错停，未定义变量炸。
- 以脚本所在目录为根，不要假设用户的 `cwd`。
- 没有二进制就配置+编译（Day15 的 `-S -B`）。
- 端口、日志级别可覆盖。
- `exec` 把进程替换成 server，Ctrl+C 信号直接给它（Day18）。

可执行：`chmod +x run.sh`。Windows 用户在 WSL 里跑，README 写明。

快速自检：

```bash
./run.sh 7777 &
sleep 1
ss -lntp | grep 7777
./run-client.sh 127.0.0.1 7777
kill -TERM %1          # 或按 PID
```

## 4. 易错点（Linux 侧）

1. **把 ASan Debug 二进制 strip 后当发布**  
   又慢又难调。Release 一份，Debug 一份。
2. **`run.sh` 写死 `/home/你的名字/...`**  
   用脚本相对路径。
3. **`ldd` 有 `not found` 却说「单文件即可」**  
   依赖要写进 README。
4. **脚本里后台起 server 不记 PID**  
   演示会留下孤儿。`exec` 前台，或记 `$!`。

## Linux 口述（预习时自己答）

1. **`ldd` 看什么？**  
   动态库依赖和加载路径。`not found` 就是换机器跑不起来的原因。
2. **为什么要 `strip`？有什么代价？**  
   减小体积、少暴露符号；代价是 gdb 回溯变差。发布与调试包分开。
3. **`run.sh` 最小要做哪些事？**  
   定位目录、必要时 cmake 构建、带默认端口启动、让信号打到 server。

## Linux 上机（预习不用敲）

对 Release 二进制 `ldd`、对照 `strip` 前后大小，写 `run.sh`（和 client）。步骤写进 README。见下面 C++ 上机。

---

# 二、C++：README、架构图、简单重连

## 为什么需要

Day24–26 已经能跑、能测。今天让**另一个人**（或 Day30 的面试官）按文档 5 分钟内复现；并补上真机常见的「线掉了再来」。代码仍在 `telemetry-hub/`。

上机任务：`telemetry-hub/README.md`（编译、运行、架构图）+ 简单断线重连。

## 1. README 最低目录（按这个写，不要散文）

```markdown
# Telemetry Hub

一句话：100Hz 模拟传感 → RingBuffer → 轻处理 → TCP 推最新状态。

## 依赖
- Linux / WSL，g++ 支持 C++17，CMake ≥ 3.16
- `ss`（iproute2）；可选 `perf`

## 编译
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel

## 运行
./run.sh 7777
# 另一终端
./run-client.sh 127.0.0.1 7777

## 架构
（下面那张 ASCII 图）

## 协议
uint32 length（网络序）+ payload；只要最新；满缓冲覆盖。

## 指标（Day26）
贴你的 P50/P99/丢包，注明 Release、是否 ASan。

## 信号
Ctrl+C / SIGTERM 优雅退出；SIGKILL 不能清理。
```

架构图（必须有，面试画这个）：

```text
sensor 100Hz ──push──► RingBuffer ──pop──► control（滤波/检测）
                                              │
                                              ▼
                                           latest_
                                              │
net: accept/send ◄──────── snapshot ─────────┘
        │
        ▼
   TCP 定长头 ──► client Framer
```

别人按「编译 + 运行」两节必须能看到 seq 在涨。做不到就把 `run.sh` 修到能。

## 2. 简单断线重连（不要做成完整中间件）

最小语义：

**Client**：`connect` 失败或 `recv==0` / 错误 → 关 fd → 睡 200～500ms → 再 `socket`+`connect`。循环受 `g_running` 管。成功后 Framer 缓冲要**清空**（半帧是旧连接的，不能拼进新连接）。

**Server**：当前 `cfd` 断开 → 回到 `accept`，等下一个 client。不要把整个进程退出。`latest_` 继续被控制线程更新，没人连就只覆盖不发送（可计 `no_client`）。

```cpp
while (g_running.load()) {
    int cfd = accept(lfd, nullptr, nullptr);
    if (cfd < 0) {
        if (errno == EINTR) continue;
        perror("accept");
        break;
    }
    serve_one_client(cfd);   // 内部 send 循环，对端关则 return
    close(cfd);
}
```

验收：

1. 先开 server，再开 client — 通。
2. 杀 client，再开一个新的 — 通，不必重启 server。
3. 先开 client（连不上）再开 server — client 在重试几次后自己连上。
4. Ctrl+C 两边都停，无僵尸。

进阶（可选，别喧宾夺主）：心跳、序号重置日志一行。不要上 epoll 集群。Day28 口述 epoll 即可，代码保持阻塞。

## 3. 打磨清单（做完打勾）

- [ ] Release `run.sh` 一键起
- [ ] `ldd` 无 `not found`，README 写了依赖
- [ ] 可选 strip，unstripped 自己留着
- [ ] README 有架构图、协议、信号、指标
- [ ] client 重连、server 再 accept
- [ ] 日志默认不刷每帧
- [ ] ASan Debug 短跑仍干净

## 4. 易错点

1. **重连后 Framer 缓冲没清空**  
   新连接解析出天文 length，立刻被你的上限踢掉。
2. **server 断开后不回 accept**  
   「简单重连」只做了 client，演示时仍要重启 server。
3. **README 只贴源码结构，没有复制即用的命令**  
   面试官/未来的你会恨你。
4. **在 README 写「很简单」却漏端口占用**  
   加一行：`ss -lntp` 查占用，先 TERM 旧进程。

## C++ 面试口述（预习时自己答一遍）

1. **用 30 秒讲 Hub**  
   「固定频率模拟传感，环形缓冲控内存，控制线程轻处理，TCP 推最新；压过延迟和丢包；Ctrl+C 与断线能恢复。」
2. **断线你怎么处理？**  
   Client 循环 connect，收包缓冲按连接重置；server 回到 accept。半帧不跨连接。
3. **怎么让别人跑起来？**  
   README + `run.sh` + `ldd` 声明依赖；Release 与 Debug 分开。

## C++ 上机（预习不用写）

见 `week4-project/day27-polish/TASK.md`：

- `telemetry-hub/README.md`：编译、运行、架构图
- 简单断线重连
- `strip` / `ldd` / `run.sh` 写入文档
