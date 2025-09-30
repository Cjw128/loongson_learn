#!/bin/sh
# rc.local ��ȫ��װ�����ű�
# ���ã�
#  1. ���� -e �� rc.local �е����Ա�֤��������ֲ�ʧ����ֹ���� rc.local
#  2. ��¼�����׶���־�����⿨��ʱ�޷���λ
#  3. �����õȴ����� (wlan0 ��ȡ IP) �ĳ�ʱ
#  4. ί�� run_project_single.sh start ʵ�ֵ�ʵ��

APP_SINGLE="/home/root/run_project_single.sh"
BOOT_LOG="/home/root/boot_project.log"
WAIT_IF="wlan0"
WAIT_TIMEOUT_SEC=10      # ���ȴ� IP ������
SLEEP_INTERVAL=0.5

log() { echo "[boot_project] $*" >> "$BOOT_LOG"; }

log "==== start $(date) ===="

if [ ! -x "$APP_SINGLE" ]; then
  log "missing $APP_SINGLE"
  exit 0  # ��������������
fi

# �ȴ�����ӿڣ���ѡ��
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
  log "interface $WAIT_IF no IP after ${WAIT_TIMEOUT_SEC}s (��������)"
fi

log "invoke single start"
"$APP_SINGLE" start >> "$BOOT_LOG" 2>&1 || log "single start returned non-zero($?)"

log "done $(date)"
exit 0
