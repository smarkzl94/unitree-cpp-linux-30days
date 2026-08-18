# 工作流：Cursor + Notion + GitHub

## 目标架构

| 工具 | 职责 |
|------|------|
| **Cursor** | 写代码、答疑、每日下发需求、帮你提交推送 |
| **Notion** | 知识点预习 + 学完复盘（路上可看） |
| **GitHub** | 每天代码与练习进度备份（公开） |

## 每日一句话触发

学完后说：

> 今天学完了，同步 Notion 和 GitHub

助手会：
1. 整理当日知识卡 → Notion「每日知识卡」
2. `git add` 当天代码 → commit → `git push`
3. 准备发往 `zhanglei_104@outlook.com` 的邮件正文（未配 SMTP 则给可粘贴版）

## 本地

- 路径：`D:\unitree-cpp-linux-30days`
- 分支：`main`
- 已完成：本地 `git init` + 首次提交

## Remote（待你确认仓库 URL 后写上）

创建公开仓库后把地址填在这里，例如：

```text
https://github.com/<你的用户名>/unitree-cpp-linux-30days
```

## Notion

- 主页：https://app.notion.com/p/3bf503fbd69f81b8b622f96163176f95
- 知识点（预习）：https://app.notion.com/p/169312b8aba34bb3949ae2e52c69611b
- 每日知识卡：https://app.notion.com/p/7a569afe814642eea07c629c7c7378be
