# 龙芯 2K0300 学习与 Demo 工程 (基于逐飞开源库)

> 目的：面向 ASC 实验室成员的快速上手与二次开发示例，涵盖电机/编码器/摄像头/参数持久化/自启动/日志调试等通用能力。

## 1. 当前进度概览
已完成：
- 电机驱动、双轮编码器读取
- 方向 / 速度双通道 PID（菜单实时调参 + 参数持久化）
- UVC 摄像头采集 + 图像显示模式切换（Gray / Track） + 裁切参数保存
- 按键扫描 & 短按 / 长按加速逻辑统一（菜单、PID、图像参数界面）
- 基础文件读写、配置文件存储（PID: `param_cfg.txt` / 图像: `image_cfg.txt`）
- 单实例自启动脚本体系（`run_project_single.sh`、`boot_project_start.sh`、`rc.local` 模板）
- 日志输出与分文件记录：
	- `project.log` 主程序
	- `boot_project.log` 启动包装脚本
	- `net_boot.log`（若使用网络启动逻辑）
- 参数保存格式升级：PID 使用带标签 P2 版本，图像参数使用带标签文本

进行 / 预研：
- 更稳定的 WiFi/SDIO 驱动接入（当前 wlan0 尚未稳定）
- 网络启动等待/重试策略（DHCP + 关联判定）
- 日志轮转、运行状态信号触发（如 `SIGUSR1` 打印快照）

## 2. 目录简述
```
Seekfree_LS2K0300_Opensource_Library/
	libraries/                # 逐飞公共/设备/驱动层封装 (common, components, device, driver)
	project/
		code/                   # 业务与菜单逻辑（PID、图像、按键、控制等）
		user/                   # 构建脚本与 CMake 入口 (build.sh, CMakeLists.txt)
scripts/ (若存在)            # 自启动 / 单实例管理脚本（建议集中放置）
docs/                       # 补充文档（host_usage_guide.md 等）
```

主要源文件说明：
- `project/code/menu.cpp`：主菜单、PID 参数菜单、图像参数菜单、参数读取写入封装
- `project/code/image.cpp`：图像参数加载/保存、裁切应用、图像模式控制
- `project/code/PID.cpp / PID.h`：PID 结构与计算逻辑（方向/速度）
- `project/code/motor.cpp`：电机控制输出
- `project/code/encoder.cpp`：编码器反馈
- `project/code/key.cpp`：按键扫描、状态判定

## 3. 构建环境
交叉编译工具链：`loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6`。

最小构建步骤（在 `project/user/` 目录）：
```
./build.sh
```
生成产物通常位于 `project/user/build/`（或脚本指定输出）。

若需手动 CMake：
```
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cross.cmake ..
make -j$(nproc)
```

## 4. 部署与运行
使用 `scp` 拷贝：
```
scp project root@<BOARD_IP>:/home/root/project
```
手动运行：
```
chmod +x /home/root/project
/home/root/project
```

## 5. 自启动方案（单实例）
推荐链路：`rc.local` → （可选等待/网络）→ `run_project_single.sh start` → `project`。

关键脚本（示例，不一定都在仓库中）：
- `run_project_single.sh`：
	- 防重复：PID 文件检测
	- 提供 `start|stop|restart|status|tail`
- `boot_project_start.sh`：
	- 可添加网络就绪等待（例如接口/IP 检测）
	- 调用单实例脚本并保证 `rc.local` 不因错误中断
- `/etc/rc.local`：
	- 仅保留一条启动路径，避免重复进程

最简 `rc.local` 示例：
```sh
#!/bin/sh
nohup /home/root/run_project_single.sh start >> /home/root/boot_project.log 2>&1 &
exit 0
```

## 6. PID 参数持久化
文件：`/home/root/param_cfg.txt`

格式 (P2 版本，带标签便于阅读)：
```
P2
Dir_Kp <float>
Dir_Ki <float>
Dir_Kd <float>
Dir_ILim <float>
Dir_OMax <float>
Dir_OMin <float>
Spd_Kp <float>
Spd_Ki <float>
Spd_Kd <float>
Spd_ILim <float>
Spd_OMax <float>
Spd_OMin <float>
Base <float>
Target <float>
```
读取逻辑：
1. 首行检测到 `P2` → 按 14 个浮点顺序解析
2. 否则尝试旧 8 参数格式 → 自动升级（未提供限幅字段时用默认值）

## 7. 图像参数持久化
文件：`/home/root/image_cfg.txt`（保存）

标签格式：
```
IMG
Mode <0|1>      # 0=Gray 1=Track
Crop_Top <int>
Crop_Bottom <int>
Crop_Left <int>
Crop_Right <int>
```
运行期通过全局变量 `g_img_display_mode / g_crop_*` 控制；菜单保存时调用 `image_param_save()`。

## 8. 菜单/按键交互
按键定义（示例，对应 KEY_1..KEY_4）：
- PID 菜单：
	- K1 减 / K3 加（长按加速）
	- K2 切换下一项
	- K4 保存退出
- 图像菜单：同样的 K1/K3/K2 逻辑，`Save & Apply` 条目按 K4 保存。

## 9. 日志与调试
默认输出到：
- `/home/root/project.log`：程序运行 / PID 调试信息
- `/home/root/boot_project.log`：启动包装脚本阶段记录
- `/home/root/net_boot.log`：网络驱动加载 / DHCP / 关联等待（若启用）

建议增强（后续可加）：
- 日志分级 (INFO/WARN/ERROR/DEBUG)
- 日志轮转（>1MB 重命名）
- `kill -USR1 <PID>` 触发状态快照打印

## 10. 常见问题 (FAQ)
| 问题 | 可能原因 | 处理 |
|------|----------|------|
| 自启动后有两个 project | rc.local 与旧脚本同时启动 | 保留单一启动路径，使用 run_project_single.sh |
| 参 数 文 件 不更新 | 未调用保存函数或权限不足 | 确认保存菜单退出按 K4，检查路径可写 |
| PID 文件残留无法启动 | 上次异常退出残留 pid | 手动 `ps` 确认后删除 `project.pid` |
| wlan0 不存在 | 驱动/固件/内核无线栈不足 | 重新编译/补固件/检查 cfg80211/mac80211 |

## 11. 后续计划 (TODO)
- 网络驱动稳定化与接口等待策略（超时日志）
- PID 参数范围/约束 UI 提示
- 图像调试：帧率统计、ROI 预览框
- Watchdog / Crash 自动重启与崩溃转储
- 远程调试控制接口（TCP 简单指令：状态/重载参数）

## 12. 开发贡献
欢迎通过 Issue / PR 反馈改进（建议记录：硬件环境、复现步骤、日志片段）。

## 13. 版权与许可
本仓库继承逐飞开源库原始许可；自编写示例代码默认同样开源（如需特殊授权请在后续补充 LICENSE 说明）。

## 14. 上位机操作指南
详见 `docs/host_usage_guide.md` ：交叉编译工具链配置、构建与部署、自启动 (systemd / rc.local)、参数文件、日志调试、更新回滚等更完整细节。

---
> 更新日期：2025-09-30
> 若文档与实际代码不一致，以代码为准；请及时提交改动补充文档。
