#!/bin/sh
# rc.local 安全包装启动脚本
# 作用：
#  1. 在有 -e 的 rc.local 中调用仍保证自身不会因局部失败终止整个 rc.local
#  2. 记录启动阶段日志，避免卡死时无法定位
#  3. 可配置等待网络 (wlan0 获取 IP) 的超时
#  4. 委托 run_project_single.sh start 实现单实例

APP_SINGLE="/home/root/run_project_single.sh"
BOOT_LOG="/home/root/boot_project.log"
WAIT_IF="wlan0"
WAIT_TIMEOUT_SEC=10      # 最多等待 IP 的秒数
SLEEP_INTERVAL=0.5

log() { echo "[boot_project] $*" >> "$BOOT_LOG"; }

log "==== start $(date) ===="

if [ ! -x "$APP_SINGLE" ]; then
  log "missing $APP_SINGLE"
  exit 0  # 不阻塞整体启动
fi

# 等待网络接口（可选）
count=0
while [ $count -lt $((WAIT_TIMEOUT_SEC*2)) ]; do
  if ip addr show "$WAIT_IF" 2>/dev/null | grep -q 'inet '; then
    log "interface $WAIT_IF got IP"
    break
  fi
  count=$((count+1))
  sleep "$SLEEP_INTERVAL"
done

if ! ip addr show "$WAIT_IF" 2>/dev/null | grep -q 'inet '; then
  log "interface $WAIT_IF no IP after ${WAIT_TIMEOUT_SEC}s (继续启动)"
fi

log "invoke single start"
"$APP_SINGLE" start >> "$BOOT_LOG" 2>&1 || log "single start returned non-zero($?)"

log "done $(date)"
exit 0
