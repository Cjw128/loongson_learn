# ��о2K0300 ��Ŀ ��λ���˲���ָ��

> ���ö����� PC/Linux �Ͻ��н�����롢���𡢵��ԡ�ά������Ŀ�Ŀ����ߡ�
> �������裺x86_64 Ubuntu / Debian ϵ��Ŀ��壺��о 2K0300 (loongarch64)��

## 1. Ŀ¼�ṹ������֪
- `Seekfree_LS2K0300_Opensource_Library/project/user/` ��Ŀ�����Դ�루CMakeLists.txt ��ͬ�� `../user` ָ�򣩡�
- `Seekfree_LS2K0300_Opensource_Library/project/out/` �������Ŀ¼����ִ���ļ� `project` ���ɺ� scp ��������� `/home/root/`����
- `scripts/` �������ű��� systemd ����ģ�塣
- `param_cfg.txt` / `image_cfg.txt`��������к�����/��ȡ������ PID ��ͼ������־û��ļ���

## 2. ���湤����׼��
����ѽ�ѹ�ٷ��ṩ�� `loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6.tar.xz`�������ѹ����`/opt/loongarch-toolchain`��

��ӵ� PATH����д�� `~/.bashrc`����
```bash
export LOONG_TCHAIN=/opt/loongarch-toolchain
export PATH="$LOONG_TCHAIN/bin:$PATH"
```
��֤��
```bash
loongarch64-linux-gnu-gcc -v
```

## 3. һ����������������
```bash
sudo apt update
sudo apt install -y build-essential cmake rsync openssh-client tmux
```
�����Ҫ OpenCV����ǰ�����߼���ȥ��Ӳ����������������
```bash
sudo apt install -y libopencv-dev
```

## 4. ��������
���� `Seekfree_LS2K0300_Opensource_Library/project/user` Ŀ¼��ִ�У�
```bash
./build.sh
```
�ű�������Ҫ��
1. ���� `../out` Ŀ¼������˵���ļ�����
2. ���� `cmake ../user` ���ɹ����ļ���
3. `make -j` �������ɿ�ִ���ļ��������������Ŀ¼����ͬ��һ��Ϊ `project`����
4. ʹ�� `scp` �����ɵ� `project` ���䵽��� `/home/root/`��

�ɹ�����Ŀ�����Ӧ���ڣ�`/home/root/project`��

## 5. ����״����У��ֶ���
SSH ��¼��
```bash
ssh root@<BOARD_IP>
cd /home/root/
./project
```
�����ڵ�¼��ͬ�������Ŀ¼ִ�С�

## 6. ��������ʽ
### 6.1 systemd �����Ƽ���
`scripts/project.service` ģ������ʾ����
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

����
```bash
# �������ࣨ���赱ǰ·���� scripts/��
scp scripts/project.service root@<BOARD_IP>:/etc/systemd/system/project.service

# ���ִ��
ssh root@<BOARD_IP> <<'EOF'
systemctl daemon-reload
systemctl enable project.service
systemctl start project.service
systemctl status project.service --no-pager
EOF
```

�鿴��־��
```bash
tail -f /home/root/project/project.log
# ��
journalctl -u project.service -f -n 100
```
ֹͣ/������
```bash
systemctl stop project.service
systemctl restart project.service
```
���ã�
```bash
systemctl disable project.service
```

### 6.2 �����ű���ʽ
`scripts/project_autostart.sh` ֧�֡����ÿ��ء�����־�����
- ������ã�`/home/root/project_autostart.sh`
- �� `/etc/rc.local`�������ã����Զ��忪���ű��е�������

ʹ�ã�
```bash
scp scripts/project_autostart.sh root@<BOARD_IP>:/home/root/
ssh root@<BOARD_IP> 'chmod +x /home/root/project_autostart.sh'
```
����һ��������
```bash
touch /home/root/disable_project_autostart
```
ɾ�����ļ��ָ��Զ�������
```bash
rm -f /home/root/disable_project_autostart
```

### 6.3 tmux ����ģʽ����ѡ��
���� `start_tmux.sh`�����δ�ṩ����
```bash
#!/bin/sh
SESSION=proj
cd /home/root/project || exit 1
tmux has-session -t $SESSION 2>/dev/null || tmux new-session -d -s $SESSION './project 2>&1 | tee -a project_tmux.log'

echo "Attach: tmux attach -t $SESSION"
```
ִ�У�
```bash
sh /home/root/start_tmux.sh
```
�жϣ��� tmux �� Ctrl+C ����룺`Ctrl+B D`��

## 7. �����ļ�˵��
������к����ɣ������ֹ����ã������ļ���
- `/home/root/param_cfg.txt`��PID �������汾ͷ P2��14 �����㣬���ݾ� 8 ���
- `/home/root/image_cfg.txt`��ͼ��ģʽ��ü�������mode + top/bottom/left/right����

���ݣ�
```bash
scp root@<BOARD_IP>:/home/root/param_cfg.txt backups/param_$(date +%Y%m%d_%H%M).txt
```
�ָ���
```bash
scp backups/param_xxx.txt root@<BOARD_IP>:/home/root/param_cfg.txt
```

## 8. ����������ع�
�����°汾��
1. �޸�Դ�� �� `./build.sh` �� �Զ������� `project`��
2. ���ʹ�� systemd���ɽ��̻��� `systemctl restart project` ���滻��

�ع�������һ���ɰ汾������
```bash
ssh root@<BOARD_IP> 'cp /home/root/project /home/root/project.bak_20250929'
# ����ع���
ssh root@<BOARD_IP> 'mv /home/root/project.bak_20250929 /home/root/project'
```

## 9. ��־���Ŵ�
���ٲ鿴��� 50 �У�
```bash
tail -n 50 /home/root/project/project.log
```
��������
```bash
grep -i error /home/root/project/project.log | tail
```
������־���ɣ����Ȩ�޻� systemd ��Ԫ�� WorkingDirectory ·����

## 10. �������� FAQ
1. �����ɹ�����˲���ִ�У�
   - ȷ���ļ��ܹ���`file project` Ӧ��ʾ `LoongArch64`����Ϊ `ELF 64-bit LSB executable, x86-64` ��δʹ�ý��湤������
2. SSH ���䳬ʱ��
   - ��� IP�����ߡ������������Ƿ����� (`ip addr`)��
3. PID ��������Ч��
   - ȷ�� `param_cfg.txt` �汾�������Ƿ�Ϊ `P2`����Ϊ�ջ��𻵳�������Ĭ�ϡ�
4. ͼ��ģʽ�ü��ı���Ч����
   - TRACK ģʽ����Ҫ������֡���ü��������ԣ���Ϊ GRAY ģʽ���ԡ�
5. ������������ʱ��ֹ��
   - systemd��`systemctl stop project.service`��
   - �ű������� `disable_project_autostart` �ļ���

## 11. ����ĺ����Ľ�����ѡ��
- ֧�ֻ����������ǲ����ļ�·����`PROJECT_PARAM_FILE`��`PROJECT_IMAGE_FILE`��
- ���� `--defaults` �������������������á�
- ������־��ת��logrotate ���û��ڲ�����С�и��
- systemd ���� `Restart=always` �� `StartLimitIntervalSec` ����Ƶ������������

## 12. �����ٲ��
| ���� | ���� | ˵�� |
|------|------|------|
| ���� + ���� | `./build.sh` | �Զ� scp ����� |
| �ֹ����� | `ssh root@IP './project'` | �������� |
| �������� | `systemctl start project` | systemd ģʽ |
| �鿴��־ | `tail -f project.log` | ʵʱ��� |
| ֹͣ���� | `systemctl stop project` | ��ʱ�ر� |
| �������� | `systemctl disable project` | �´β������� |
| ���� PID | `scp root@IP:/home/root/param_cfg.txt .` | ������� |
| �ع������� | ���ݲ����� | �ָ��ɰ汾 |

---
�������������ϲ����Զ����ű��������ṩ CI/CD ��һ������ű��������ɰ��貹�䡣
