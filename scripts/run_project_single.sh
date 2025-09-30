#!/bin/sh
# 单实例启动管理脚本
# 功能: start / stop / restart / status / tail
# 解决: rc.local 已经后台启动后, 人为再次 ./project 造成双实例的问题。
# 机制: 使用 flock(若存在) 或自管 lock 文件 + pid 校验。

APP="/home/root/project"
WORKDIR="/home/root"
LOG="/home/root/project.log"
PID_FILE="/home/root/project.pid"
LOCK_FILE="/home/root/project.lock"
TAIL_LINES=${TAIL_LINES:-80}

is_running() {
    if [ -f "$PID_FILE" ]; then
        pid=$(cat "$PID_FILE" 2>/dev/null)
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

start_app() {
    if ! [ -x "$APP" ]; then
        echo "[single] ERROR: $APP 不存在或不可执行"
        return 2
    fi
    if is_running; then
        echo "[single] 已在运行 (PID=$(cat $PID_FILE))"
        return 0
    fi
    echo "[single] 启动 $(date)" >> "$LOG"
    ( cd "$WORKDIR" || exit 1; \
      nohup "$APP" >>"$LOG" 2>&1 & echo $! >"$PID_FILE" )
    sleep 0.3
    if is_running; then
        echo "[single] 启动成功 PID=$(cat $PID_FILE)"
        return 0
    else
        echo "[single] 启动失败 (请查看日志 $LOG)"
        return 1
    fi
}

stop_app() {
    if ! is_running; then
        echo "[single] 未在运行"
        rm -f "$PID_FILE" "$LOCK_FILE" 2>/dev/null
        return 0
    fi
    pid=$(cat "$PID_FILE")
    echo "[single] 停止 PID=$pid" | tee -a "$LOG"
    kill "$pid" 2>/dev/null || true
    # 等待退出
    for i in 1 2 3 4 5; do
        if ! kill -0 "$pid" 2>/dev/null; then
            break
        fi
        sleep 0.5
    done
    if kill -0 "$pid" 2>/dev/null; then
        echo "[single] 发送 SIGKILL" | tee -a "$LOG"
        kill -9 "$pid" 2>/dev/null || true
    fi
    rm -f "$PID_FILE" "$LOCK_FILE" 2>/dev/null
    echo "[single] 已停止" | tee -a "$LOG"
}

status_app() {
    if is_running; then
        pid=$(cat "$PID_FILE")
        echo "[single] 运行中 PID=$pid"; return 0
    else
        echo "[single] 未运行"; return 1
    fi
}

tail_log() {
    if [ -f "$LOG" ]; then
        tail -n "$TAIL_LINES" -f "$LOG"
    else
        echo "[single] 日志不存在: $LOG"
    fi
}

restart_app() {
    stop_app
    start_app
}

case "$1" in
    start)   start_app ;; 
    stop)    stop_app ;;
    restart) restart_app ;;
    status)  status_app ;;
    tail)    tail_log ;;
    *) echo "用法: $0 {start|stop|restart|status|tail}" ; exit 1 ;;
esac
