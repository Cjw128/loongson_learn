#!/bin/sh
# 自动启动封装脚本
# 放到板卡上 /home/root/loongson_learn_autorun/project_autostart.sh 并赋予可执行权限。
# 通过创建 /home/root/disable_project_autostart 文件可临时禁止自启。
# 日志输出：/home/root/project.log （按需改成 logrotate 配合外部轮转）
# 若需要交互调试：ssh 上来后直接 systemctl stop project.service 或 kill.

APP_NAME="project"                   # 可执行文件名
APP_BASE="/home/root"                # 根部署路径
APP_DIR="${APP_BASE}/${APP_NAME}"    # 期望目录：/home/root/project （目录形式）
LOG_FILE="${APP_BASE}/${APP_NAME}.log"
DISABLE_FLAG="${APP_BASE}/disable_${APP_NAME}_autostart"

# 若 /home/root 不存在或不可写（例如在开发机普通用户直接测试），回退到当前目录日志
if [ ! -d "$APP_BASE" ] || [ ! -w "$APP_BASE" ]; then
  LOG_FILE="./project_autostart_test.log"
fi

# 兼容两种现状：
# 1) 只 scp 到 /home/root/project   -> 这个是一个“文件”而不是目录
# 2) 将来可能创建目录 /home/root/project/ ，二进制位于 /home/root/project/project
# 检测逻辑：若 APP_DIR 是目录且其中存在同名可执行；否则回退到 APP_BASE 视为文件直接放在其下

if [ -d "$APP_DIR" ]; then
  if [ -x "$APP_DIR/$APP_NAME" ]; then
    EXEC_PATH="$APP_DIR/$APP_NAME"
  else
    echo "[autorun] 目录 $APP_DIR 存在但缺少可执行 $APP_NAME" >> "$LOG_FILE" 2>&1
    exit 1
  fi
elif [ -f "$APP_DIR" ]; then
  # /home/root/project 本身就是文件
  EXEC_PATH="$APP_DIR"
  APP_DIR="$APP_BASE"   # 工作目录回退到 /home/root
else
  echo "[autorun] 未找到文件或目录: $APP_DIR" >> "$LOG_FILE" 2>&1
  exit 1
fi

# 通过这里的 echo 也可以及早验证日志路径是否可写
echo "[autorun] log init path=$LOG_FILE" >> "$LOG_FILE" 2>&1 || {
  echo "[autorun] ERROR: cannot write log file $LOG_FILE" >&2
  exit 1
}

# 可选：等待网络/文件系统稳定
sleep 1

if [ -f "$DISABLE_FLAG" ]; then
  echo "[autorun] disable flag present: $DISABLE_FLAG, skip start" >> "$LOG_FILE" 2>&1
  exit 0
fi

cd "$APP_DIR" || exit 1
echo "[autorun] starting: $EXEC_PATH (workdir=$APP_DIR log=$LOG_FILE)" >> "$LOG_FILE" 2>&1
exec "$EXEC_PATH" >> "$LOG_FILE" 2>&1
