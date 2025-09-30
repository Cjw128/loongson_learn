#!/bin/sh
# ��ʵ����������ű�
# ����: start / stop / restart / status / tail
# ���: rc.local �Ѿ���̨������, ��Ϊ�ٴ� ./project ���˫ʵ�������⡣
# ����: ʹ�� flock(������) ���Թ� lock �ļ� + pid У�顣

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
        echo "[single] ERROR: $APP �����ڻ򲻿�ִ��"
        return 2
    fi
    if is_running; then
        echo "[single] �������� (PID=$(cat $PID_FILE))"
        return 0
    fi
    echo "[single] ���� $(date)" >> "$LOG"
    ( cd "$WORKDIR" || exit 1; \
      nohup "$APP" >>"$LOG" 2>&1 & echo $! >"$PID_FILE" )
    sleep 0.3
    if is_running; then
        echo "[single] �����ɹ� PID=$(cat $PID_FILE)"
        return 0
    else
        echo "[single] ����ʧ�� (��鿴��־ $LOG)"
        return 1
    fi
}

stop_app() {
    if ! is_running; then
        echo "[single] δ������"
        rm -f "$PID_FILE" "$LOCK_FILE" 2>/dev/null
        return 0
    fi
    pid=$(cat "$PID_FILE")
    echo "[single] ֹͣ PID=$pid" | tee -a "$LOG"
    kill "$pid" 2>/dev/null || true
    # �ȴ��˳�
    for i in 1 2 3 4 5; do
        if ! kill -0 "$pid" 2>/dev/null; then
            break
        fi
        sleep 0.5
    done
    if kill -0 "$pid" 2>/dev/null; then
        echo "[single] ���� SIGKILL" | tee -a "$LOG"
        kill -9 "$pid" 2>/dev/null || true
    fi
    rm -f "$PID_FILE" "$LOCK_FILE" 2>/dev/null
    echo "[single] ��ֹͣ" | tee -a "$LOG"
}

status_app() {
    if is_running; then
        pid=$(cat "$PID_FILE")
        echo "[single] ������ PID=$pid"; return 0
    else
        echo "[single] δ����"; return 1
    fi
}

tail_log() {
    if [ -f "$LOG" ]; then
        tail -n "$TAIL_LINES" -f "$LOG"
    else
        echo "[single] ��־������: $LOG"
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
    *) echo "�÷�: $0 {start|stop|restart|status|tail}" ; exit 1 ;;
esac
