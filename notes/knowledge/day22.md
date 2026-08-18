# Day22 · TCP Echo

日期：2026-09-08

## 为什么学这个
机器人和上位机/云边常用 TCP。先搞通阻塞模型的 server/client。

## 核心知识
1. **套接字步骤（server）**：`socket` → `bind` → `listen` → `accept` → `recv/send` → `close`。
2. **client**：`socket` → `connect` → `send/recv`。
3. **TCP 是字节流**：无消息边界（为 Day23 粘包埋伏笔）。
4. **回环测试**：`127.0.0.1`；先本机再谈跨机。
5. **错误处理**：连接断开、`recv==0` 表示对端关闭。

## 易错点
- 当「每次 recv 就是一条完整消息」
- 忘记处理部分 send
- 防火墙/权限导致 bind 失败不看 errno

## 面试常问
- TCP 和 UDP 区别？机器人状态上报为何常用 TCP？
- 什么是粘包？
