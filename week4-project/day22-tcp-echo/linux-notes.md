# Day22 · Linux 笔记（ss 看监听）

日期：2026-09-08  
环境：（WSL2 Ubuntu / 其他：______）

对着空写。写完丢过来批。

---

## `ss -lntp` 每个字母

- `-l`：listen
- `-n`：num
- `-t`：tcp
- `-p`：process

## 看懂一行（可贴自己的输出）

```text
（server 起来后 ss -lntp 贴这里）
```

- State = LISTEN 表示：
- `127.0.0.1:端口` 和 `0.0.0.0:端口` 差在哪：
- 谁占着这个端口（PID/程序名）：

## LISTEN vs ESTAB

- LISTEN：
- ESTAB（`ss -tnp`，client 连上之后）：

## 我踩过的坑

- 

