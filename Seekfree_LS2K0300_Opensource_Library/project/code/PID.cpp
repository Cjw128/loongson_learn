#include "zf_common_headfile.h"
#include "PID.h"
#include "Header.h"

pid_param motor_l, motor_r;
pid_param motor_dir , motor_gyro ;

void pid_reset(pid_param *p, float kp, float ki, float kd,
               float integral_limit, float omax, float omin)
{
    if(!p) return;
    p->kp = kp; p->ki = ki; p->kd = kd;
    p->integral = 0.0f;
    p->err = p->last_error = p->previous_error = 0.0f;
    p->output = 0.0f;
    p->integral_limit = integral_limit;
    p->omax = omax; p->omin = omin;
    p->target = 0.0f; p->actual = 0.0f;
}

// 不再依赖菜单参数统一映射，由外部（菜单模块）直接操作 pid_param 成员。
void pid_apply_menu_params(void) { /* 保留空实现兼容旧调用 */ }

int16 constrain_data(int amt,int low,int high)
{
    return (amt < low) ? low : ((amt > high) ? high : amt);
}

//------------------------------------------------------------------------------------------------
//函数名称    increasing_PID_control
//参数说明    pid_param* PID：PID参数结构体指针
//返回参数    float：PID输出值
//使用实例    float output = increasing_PID_control(&motor_l);    // 计算电机左轮的PID输出值
//------------------------------------------------------------------------------------------------
float increasing_PID_control(pid_param* PID)
{
    PID->err = PID->target - PID->actual;
    PID->output = PID->kp*(PID->err - PID->last_error) + PID->ki*PID->err + PID->kd*(PID->err - 2 * PID->last_error + PID->previous_error);
    PID->output = constrain_data(PID->output, PID->omin, PID->omax);
    PID->previous_error = PID->last_error;
    PID->last_error = PID->err;
    return PID->output;
}

//------------------------------------------------------------------------------------------------
//函数名称    position_PID_control
//参数说明    pid_param* PID：PID参数结构体指针
//返回参数    float：PID输出值
//使用实例    float output = position_PID_control(&motor_r);    // 计算电机右轮的PID输出值
//------------------------------------------------------------------------------------------------
float position_PID_control(pid_param* PID)
{
    PID->err = PID->target - PID->actual;
    PID->integral += PID->err;
    if (PID->integral > PID->integral_limit) PID->integral = PID->integral_limit;
    else if (PID->integral < -PID->integral_limit) PID->integral = -PID->integral_limit;
    PID->output = PID->kp * PID->err + PID->ki * PID->integral + PID->kd * (PID->err - PID->last_error);
    PID->output = constrain_data(PID->output, PID->omin, PID->omax);
    PID->last_error = PID->err;
    return PID->output;
}