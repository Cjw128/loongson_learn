#ifndef PID_H
#define PID_H

typedef struct 
{
    float kp;           // ����ϵ��
    float ki;           // ����ϵ��
    float kd;           // ΢��ϵ��
    float integral;     // ����ֵ
    float err;         // ��ǰ���ֵ
    float last_error;   // ��һ�����ֵ
    float previous_error; // ���ϴ����ֵ
    float output;       // ���ֵ
    float integral_limit; // �����޷�
    float omax;        // ����޷����ֵ
    float omin;        // ����޷���Сֵ
    float target;      // Ŀ��ֵ
    float actual;      // ʵ��ֵ
} pid_param;

// ȫ��PIDʵ������ PID.cpp �ж��壩
extern pid_param motor_l, motor_r;     // �ٶȻ�λ�� PID
extern pid_param motor_dir, motor_gyro;// ���� / ���� PID

// ��ʼ������ PID �ṹ (����������������޷�)
void pid_reset(pid_param *p, float kp, float ki, float kd,
               float integral_limit, float omax, float omin);

// ���˵������󶨵���� PID����ʵ�־���ӳ���ϵ��
void pid_apply_menu_params(void);


#endif