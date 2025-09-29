#ifndef MENU_H
#define MENU_H

#include <stdint.h>

// �˵��ӿ�
int menu_main(void);                 // �����ܲ˵�ʾ��
int menu_image_view(void);               // ͼ����/������ʾ�˵�
int menu2_2(void);               // ��ȷ��/���ز˵�
int menu_pid_config(void);       // ֱ�ӱ༭ PID �����Ĳ˵�
int menu_image_config(void);      // ͼ��������ò˵�

// ---- ���ݾɰ汾�ӿڣ�Ǩ���ڱ����� ----
// �ɴ�����ʹ�õĽṹ�뺯����param_config_t / menu_param_config / param_flash_read / param_flash_write
// ���ڲ���ֱ��������ȫ�� pid_param����˰�װΪ�����µ�ʵ�֡�
// ����ռλ�ṹ�����ɴ����� extern param_config_t params; �ɼ������ӣ��ڲ�����ʹ�ã���
typedef struct {
	float Kp_dir, Ki_dir, Kd_dir;      // ���� PID (��: Kp_dir, Kd_dir + kd_diff ��ϣ����ﱣ�����������չ)
	float Kp_speed, Ki_speed, Kd_speed;// �ٶ� PID
	float base_speed, target_speed;    // �ٶ��趨
} param_config_t;

// ����һ����̬ӳ���� pid_param �Ŀ��գ��޸��������Զ���д�������������²˵���
param_config_t * get_param_config(void);
int menu_param_config(void);          // ���ݾ��� -> ���� menu_pid_config
void param_flash_read(void);          // ���ݾ��� -> pid_param_flash_read
void param_flash_write(void);         // ���ݾ��� -> pid_param_flash_write

// PID �����־û����ٶ� PID + ���� PID + ��չ��base_speed,target_speed��
void pid_param_flash_read(void);
void pid_param_flash_write(void);
float * menu_get_base_speed_ptr(void);      // �����ٶ�����
float * menu_get_target_speed_ptr(void);    // Ŀ���ٶ�����

#endif // MENU_H