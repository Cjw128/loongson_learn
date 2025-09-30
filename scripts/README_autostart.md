# �����������˵��

## Ŀ¼���ļ�
- project�������ƣ����� CMake �����������踴�Ƶ��忨 `/home/root/project/`
- scripts/project.service��systemd ����Ԫģ��
- scripts/project_autostart.sh�������ű�������ʽ����ʹ�� systemd ʱ��

## ����һ���Ƽ�ʹ�� systemd ������
1. �ڰ忨�ϴ���Ŀ¼��`mkdir -p /home/root/project`
2. �����ɵĿ�ִ���ļ� `project` �Լ������������Դ�����á�����ȣ����ƽ�ȥ��
3. ���������ļ���
   `scp scripts/project.service root@BOARD_IP:/etc/systemd/system/project.service`
4. ��Ȩ��ˢ�£�
   `ssh root@BOARD_IP 'systemctl daemon-reload && systemctl enable project.service'`
5. ����������
   `ssh root@BOARD_IP 'systemctl start project.service'`
6. �鿴״̬/��־��
   - `systemctl status project.service`
   - `journalctl -u project.service -f`
   - ��ֱ�Ӳ鿴 `/home/root/project/project.log`
7. ֹͣ�Ա���ԣ�`systemctl stop project.service`
8. �޸ĳ�������²���
   - ���ǿ�ִ���ļ���`systemctl restart project.service`

### ��ʱ���ÿ�������
`systemctl disable project.service`

## ���������ű��������� systemd ����ʱ���ԣ�
1. ������`scp scripts/project_autostart.sh root@BOARD_IP:/home/root/project_autostart.sh`
2. ��Ȩ��`ssh root@BOARD_IP 'chmod +x /home/root/project_autostart.sh'`
3. ��ӵ� rc.local�������ڣ��� profile��
   - `/etc/rc.local` ��׷��һ�У�`/home/root/project_autostart.sh &`
4. ���ã��ڰ忨�ϴ��� `/home/root/disable_project_autostart` �ļ���

## Ctrl+C ���Է�ʽ
ʹ�� systemd ʱ��
1. `systemctl stop project.service`
2. ����Ŀ¼��`cd /home/root/project`
3. ǰ̨���У�`./project`
4. Ctrl+C ���������ã�`systemctl start project.service`

## ��������
| ���� | ��� |
| ---- | ---- |
| ����δ���� | ��� `journalctl -u project.service` ��־��ȷ�� WorkingDirectory ���ļ�Ȩ�� |
| ��Ҫʵʱ����/����̨��� | �Ƴ� service �� `StandardOutput/StandardError` ���û��Ϊ `journal` |
| ��־���� | ��� logrotate ���ڳ���������С�ض� |
| ��ǰ����Ӳ��ʧ�� | �� Unit ������ `After=` �������� `After=dev-video0.device`�� |

## ��ѡ��ʹ�� tmux ��̨�Ự
�������ʱ attach��
```
tmux new -s proj -d '/home/root/project/project'
# �鿴
 tmux attach -t proj
# �˳��Ự��ɱ���̣�Ctrl+B Ȼ�� D
# ֹͣ���򣺽���Ự Ctrl+C �� kill ����
```

## ����������չ
���� service �� `[Service]` ��׷�ӣ�
```
Environment=PID_CFG_PATH=/home/root/param_cfg.txt
Environment=IMAGE_CFG_PATH=/home/root/image_cfg.txt
```
�ڴ�����ͨ�� `getenv()` �жϸ���Ĭ��·����

---
����Ҫ��ֱ�Ӱѻ�������֧�ּӽ����룬���Զ���� camera ��ʼ��ʧ�ܺ����������ټ��������ҡ�
