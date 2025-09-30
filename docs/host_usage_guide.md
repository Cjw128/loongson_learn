# 龙芯2K0300 项目 上位机端操作指南

> 适用对象：在 PC/Linux 上进行交叉编译、部署、调试、维护本项目的开发者。
> 主机假设：x86_64 Ubuntu / Debian 系。目标板：龙芯 2K0300 (loongarch64)。

## 1. 目录结构快速认知
- `Seekfree_LS2K0300_Opensource_Library/project/user/` 项目主入口源码（CMakeLists.txt 在同级 `../user` 指向）。
- `Seekfree_LS2K0300_Opensource_Library/project/out/` 构建输出目录（可执行文件 `project` 生成后被 scp 推送至板端 `/home/root/`）。
- `scripts/` 自启动脚本与 systemd 服务模板。
- `param_cfg.txt` / `image_cfg.txt`（板端运行后生成/读取）—— PID 与图像参数持久化文件。

## 2. 交叉工具链准备
如果已解压官方提供的 `loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6.tar.xz`，假设解压到：`/opt/loongarch-toolchain`。

添加到 PATH（可写入 `~/.bashrc`）：
```bash
export LOONG_TCHAIN=/opt/loongarch-toolchain
export PATH="$LOONG_TCHAIN/bin:$PATH"
```
验证：
```bash
loongarch64-linux-gnu-gcc -v
```

## 3. 一次性依赖（主机）
```bash
sudo apt update
sudo apt install -y build-essential cmake rsync openssh-client tmux
```
如果需要 OpenCV（当前核心逻辑已去除硬依赖，可跳过）：
```bash
sudo apt install -y libopencv-dev
```

## 4. 编译流程
进入 `Seekfree_LS2K0300_Opensource_Library/project/user` 目录，执行：
```bash
./build.sh
```
脚本动作概要：
1. 清理 `../out` 目录（保留说明文件）。
2. 运行 `cmake ../user` 生成构建文件。
3. `make -j` 编译生成可执行文件（名称与最外层目录名相同，一般为 `project`）。
4. 使用 `scp` 将生成的 `project` 传输到板端 `/home/root/`。

成功后在目标板上应存在：`/home/root/project`。

## 5. 板端首次运行（手动）
SSH 登录：
```bash
ssh root@<BOARD_IP>
cd /home/root/
./project
```
若串口登录，同样进入该目录执行。

## 6. 自启动方式
### 6.1 systemd 服务（推荐）
`scripts/project.service` 模板内容示例：
```
[Unit]
Description=LS2K0300 Project Auto Start
After=network.target

[Service]
Type=simple
WorkingDirectory=/home/root/project
ExecStart=/home/root/project/project
Restart=on-failure
RestartSec=2
StandardOutput=append:/home/root/project/project.log
StandardError=append:/home/root/project/project.log

[Install]
WantedBy=multi-user.target
```

部署：
```bash
# 在主机侧（假设当前路径含 scripts/）
scp scripts/project.service root@<BOARD_IP>:/etc/systemd/system/project.service

# 板端执行
ssh root@<BOARD_IP> <<'EOF'
systemctl daemon-reload
systemctl enable project.service
systemctl start project.service
systemctl status project.service --no-pager
EOF
```

查看日志：
```bash
tail -f /home/root/project/project.log
# 或
journalctl -u project.service -f -n 100
```
停止/重启：
```bash
systemctl stop project.service
systemctl restart project.service
```
禁用：
```bash
systemctl disable project.service
```

### 6.2 轻量脚本方式
`scripts/project_autostart.sh` 支持“禁用开关”与日志输出：
- 建议放置：`/home/root/project_autostart.sh`
- 在 `/etc/rc.local`（若启用）或自定义开机脚本中调用它。

使用：
```bash
scp scripts/project_autostart.sh root@<BOARD_IP>:/home/root/
ssh root@<BOARD_IP> 'chmod +x /home/root/project_autostart.sh'
```
禁用一次启动：
```bash
touch /home/root/disable_project_autostart
```
删除该文件恢复自动启动：
```bash
rm -f /home/root/disable_project_autostart
```

### 6.3 tmux 调试模式（可选）
创建 `start_tmux.sh`（如果未提供）：
```bash
#!/bin/sh
SESSION=proj
cd /home/root/project || exit 1
tmux has-session -t $SESSION 2>/dev/null || tmux new-session -d -s $SESSION './project 2>&1 | tee -a project_tmux.log'

echo "Attach: tmux attach -t $SESSION"
```
执行：
```bash
sh /home/root/start_tmux.sh
```
中断：在 tmux 内 Ctrl+C 或分离：`Ctrl+B D`。

## 7. 参数文件说明
板端运行后生成（或需手工放置）以下文件：
- `/home/root/param_cfg.txt`：PID 参数（版本头 P2，14 个浮点，兼容旧 8 项）。
- `/home/root/image_cfg.txt`：图像模式与裁剪参数（mode + top/bottom/left/right）。

备份：
```bash
scp root@<BOARD_IP>:/home/root/param_cfg.txt backups/param_$(date +%Y%m%d_%H%M).txt
```
恢复：
```bash
scp backups/param_xxx.txt root@<BOARD_IP>:/home/root/param_cfg.txt
```

## 8. 迭代更新与回滚
更新新版本：
1. 修改源码 → `./build.sh` → 自动推送新 `project`。
2. 如果使用 systemd，旧进程会在 `systemctl restart project` 后被替换。

回滚：保留一个旧版本副本：
```bash
ssh root@<BOARD_IP> 'cp /home/root/project /home/root/project.bak_20250929'
# 如需回滚：
ssh root@<BOARD_IP> 'mv /home/root/project.bak_20250929 /home/root/project'
```

## 9. 日志与排错
快速查看最近 50 行：
```bash
tail -n 50 /home/root/project/project.log
```
搜索错误：
```bash
grep -i error /home/root/project/project.log | tail
```
若无日志生成：检查权限或 systemd 单元的 WorkingDirectory 路径。

## 10. 常见问题 FAQ
1. 构建成功但板端不可执行：
   - 确认文件架构：`file project` 应显示 `LoongArch64`；若为 `ELF 64-bit LSB executable, x86-64` 则未使用交叉工具链。
2. SSH 传输超时：
   - 检查 IP、网线、板端网络服务是否启动 (`ip addr`)。
3. PID 参数不生效：
   - 确认 `param_cfg.txt` 版本号首行是否为 `P2`；若为空或损坏程序会回退默认。
4. 图像模式裁剪改变无效果：
   - TRACK 模式下若要求完整帧，裁剪将被忽略；改为 GRAY 模式测试。
5. 自启动后想临时阻止：
   - systemd：`systemctl stop project.service`。
   - 脚本：创建 `disable_project_autostart` 文件。

## 11. 建议的后续改进（可选）
- 支持环境变量覆盖参数文件路径：`PROJECT_PARAM_FILE`、`PROJECT_IMAGE_FILE`。
- 增加 `--defaults` 启动参数快速重置配置。
- 增加日志轮转（logrotate 配置或内部按大小切割）。
- systemd 增加 `Restart=always` 与 `StartLimitIntervalSec` 控制频繁崩溃保护。

## 12. 快速速查表
| 操作 | 命令 | 说明 |
|------|------|------|
| 编译 + 部署 | `./build.sh` | 自动 scp 到板端 |
| 手工运行 | `ssh root@IP './project'` | 交互调试 |
| 启动服务 | `systemctl start project` | systemd 模式 |
| 查看日志 | `tail -f project.log` | 实时输出 |
| 停止服务 | `systemctl stop project` | 临时关闭 |
| 禁用自启 | `systemctl disable project` | 下次不再启动 |
| 备份 PID | `scp root@IP:/home/root/param_cfg.txt .` | 保存参数 |
| 回滚二进制 | 备份并覆盖 | 恢复旧版本 |

---
如需把上述步骤合并成自动化脚本，可再提供 CI/CD 或一键部署脚本，后续可按需补充。
