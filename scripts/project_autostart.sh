#!/bin/sh
# �Զ�������װ�ű�
# �ŵ��忨�� /home/root/loongson_learn_autorun/project_autostart.sh �������ִ��Ȩ�ޡ�
# ͨ������ /home/root/disable_project_autostart �ļ�����ʱ��ֹ������
# ��־�����/home/root/project.log ������ĳ� logrotate ����ⲿ��ת��
# ����Ҫ�������ԣ�ssh ������ֱ�� systemctl stop project.service �� kill.

APP_NAME="project"                   # ��ִ���ļ���
APP_BASE="/home/root"                # ������·��
APP_DIR="${APP_BASE}/${APP_NAME}"    # ����Ŀ¼��/home/root/project ��Ŀ¼��ʽ��
LOG_FILE="${APP_BASE}/${APP_NAME}.log"
DISABLE_FLAG="${APP_BASE}/disable_${APP_NAME}_autostart"

# �� /home/root �����ڻ򲻿�д�������ڿ�������ͨ�û�ֱ�Ӳ��ԣ������˵���ǰĿ¼��־
if [ ! -d "$APP_BASE" ] || [ ! -w "$APP_BASE" ]; then
  LOG_FILE="./project_autostart_test.log"
fi

# ����������״��
# 1) ֻ scp �� /home/root/project   -> �����һ�����ļ���������Ŀ¼
# 2) �������ܴ���Ŀ¼ /home/root/project/ ��������λ�� /home/root/project/project
# ����߼����� APP_DIR ��Ŀ¼�����д���ͬ����ִ�У�������˵� APP_BASE ��Ϊ�ļ�ֱ�ӷ�������

if [ -d "$APP_DIR" ]; then
  if [ -x "$APP_DIR/$APP_NAME" ]; then
    EXEC_PATH="$APP_DIR/$APP_NAME"
  else
    echo "[autorun] Ŀ¼ $APP_DIR ���ڵ�ȱ�ٿ�ִ�� $APP_NAME" >> "$LOG_FILE" 2>&1
    exit 1
  fi
elif [ -f "$APP_DIR" ]; then
  # /home/root/project ��������ļ�
  EXEC_PATH="$APP_DIR"
  APP_DIR="$APP_BASE"   # ����Ŀ¼���˵� /home/root
else
  echo "[autorun] δ�ҵ��ļ���Ŀ¼: $APP_DIR" >> "$LOG_FILE" 2>&1
  exit 1
fi

# ͨ������� echo Ҳ���Լ�����֤��־·���Ƿ��д
echo "[autorun] log init path=$LOG_FILE" >> "$LOG_FILE" 2>&1 || {
  echo "[autorun] ERROR: cannot write log file $LOG_FILE" >&2
  exit 1
}

# ��ѡ���ȴ�����/�ļ�ϵͳ�ȶ�
sleep 1

if [ -f "$DISABLE_FLAG" ]; then
  echo "[autorun] disable flag present: $DISABLE_FLAG, skip start" >> "$LOG_FILE" 2>&1
  exit 0
fi

cd "$APP_DIR" || exit 1
echo "[autorun] starting: $EXEC_PATH (workdir=$APP_DIR log=$LOG_FILE)" >> "$LOG_FILE" 2>&1
exec "$EXEC_PATH" >> "$LOG_FILE" 2>&1
