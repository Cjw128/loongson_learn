#ifndef PID_H
#define PID_H

typedef struct 
{
    float kp;           // 比例系数
    float ki;           // 积分系数
    float kd;           // 微分系数
    float integral;     // 积分值
    float err;         // 当前误差值
    float last_error;   // 上一次误差值
    float previous_error; // 上上次误差值
    float output;       // 输出值
    float integral_limit; // 积分限幅
    float omax;        // 输出限幅最大值
    float omin;        // 输出限幅最小值
    float target;      // 目标值
    float actual;      // 实际值
} pid_param;

// 全局PID实例（在 PID.cpp 中定义）
extern pid_param motor_l, motor_r;     // 速度或位置 PID
extern pid_param motor_dir, motor_gyro;// 方向 / 陀螺 PID

// 初始化单个 PID 结构 (清零积分与误差，设置限幅)
void pid_reset(pid_param *p, float kp, float ki, float kd,
               float integral_limit, float omax, float omin);

// 将菜单参数绑定到相关 PID（由实现决定映射关系）
void pid_apply_menu_params(void);


#endif