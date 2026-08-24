# 邮箱发送说明

收件人：`zhanglei_104@outlook.com`

## 推荐工作流（当前默认）

1. 学完后说：「今天学完了，同步 Notion 和邮箱」
2. 助手会：
   - 写 `notes/daily/YYYY-MM-DD.md`
   - 同步到 Notion「每日知识卡」
   - 在聊天里给出可直接粘贴到 Outlook 的邮件标题 + 正文

## 若要真正自动发信（可选）

Outlook.com SMTP：

- 服务器：`smtp-mail.outlook.com`
- 端口：`587`（STARTTLS）
- 需要：邮箱 + **应用密码**（或允许的 SMTP 凭据）

不要把密码写进仓库。若你配置好后，可把应用密码放在本机用户环境变量，例如：

```powershell
[System.Environment]::SetEnvironmentVariable("UNITREE_SMTP_USER", "zhanglei_104@outlook.com", "User")
[System.Environment]::SetEnvironmentVariable("UNITREE_SMTP_PASS", "你的应用密码", "User")
```

然后告诉我「已配置 SMTP」，之后可用脚本自动发送。

## 邮件标题格式

`[宇树30天] YYYY-MM-DD DayXX · 主题`
