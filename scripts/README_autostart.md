# 自启动与调试说明

## 目录与文件
- project（二进制）：由 CMake 构建产生，需复制到板卡 `/home/root/project/`
- scripts/project.service：systemd 服务单元模板
- scripts/project_autostart.sh：独立脚本启动方式（不使用 systemd 时）

## 方案一：推荐使用 systemd 自启动
1. 在板卡上创建目录：`mkdir -p /home/root/project`
2. 将生成的可执行文件 `project` 以及运行所需的资源（配置、字体等）复制进去。
3. 拷贝服务文件：
   `scp scripts/project.service root@BOARD_IP:/etc/systemd/system/project.service`
4. 赋权并刷新：
   `ssh root@BOARD_IP 'systemctl daemon-reload && systemctl enable project.service'`
5. 立即启动：
   `ssh root@BOARD_IP 'systemctl start project.service'`
6. 查看状态/日志：
   - `systemctl status project.service`
   - `journalctl -u project.service -f`
   - 或直接查看 `/home/root/project/project.log`
7. 停止以便调试：`systemctl stop project.service`
8. 修改程序后重新部署：
   - 覆盖可执行文件后：`systemctl restart project.service`

### 临时禁用开机启动
`systemctl disable project.service`

## 方案二：脚本自启（无 systemd 或临时调试）
1. 拷贝：`scp scripts/project_autostart.sh root@BOARD_IP:/home/root/project_autostart.sh`
2. 赋权：`ssh root@BOARD_IP 'chmod +x /home/root/project_autostart.sh'`
3. 添加到 rc.local（若存在）或 profile：
   - `/etc/rc.local` 中追加一行：`/home/root/project_autostart.sh &`
4. 禁用：在板卡上创建 `/home/root/disable_project_autostart` 文件。

## Ctrl+C 调试方式
使用 systemd 时：
1. `systemctl stop project.service`
2. 进入目录：`cd /home/root/project`
3. 前台运行：`./project`
4. Ctrl+C 结束后再用：`systemctl start project.service`

## 常见问题
| 问题 | 解决 |
| ---- | ---- |
| 程序未启动 | 检查 `journalctl -u project.service` 日志，确认 WorkingDirectory 与文件权限 |
| 需要实时串口/控制台输出 | 移除 service 中 `StandardOutput/StandardError` 配置或改为 `journal` |
| 日志过大 | 配合 logrotate 或在程序内做大小截断 |
| 提前访问硬件失败 | 在 Unit 中增加 `After=` 依赖（如 `After=dev-video0.device`） |

## 可选：使用 tmux 后台会话
如果想随时 attach：
```
tmux new -s proj -d '/home/root/project/project'
# 查看
 tmux attach -t proj
# 退出会话不杀进程：Ctrl+B 然后 D
# 停止程序：进入会话 Ctrl+C 或 kill 进程
```

## 环境变量扩展
可在 service 的 `[Service]` 中追加：
```
Environment=PID_CFG_PATH=/home/root/param_cfg.txt
Environment=IMAGE_CFG_PATH=/home/root/image_cfg.txt
```
在代码里通过 `getenv()` 判断覆盖默认路径。

---
若需要我直接把环境变量支持加进代码，或自动检测 camera 初始化失败后重启服务，再继续告诉我。
