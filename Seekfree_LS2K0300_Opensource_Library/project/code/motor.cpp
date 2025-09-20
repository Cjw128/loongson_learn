//本文件为用户自己编写的针对龙芯2k0300逐飞开源库及开发板母板的电机控制文件,参考型号为drv8701
#include "zf_common_headfile.h"
#include "motor.h"
#include "Header.h"



/********************************************用户代码*********************************************/

 struct pwm_info motor_1_pwm_info;
 struct pwm_info motor_2_pwm_info;
//------------------------------------------------------------------------------------------------
//函数名称    motor_init
//参数说明    void
//返回参数    void
//使用实例    motor_init();    // 初始化电机
//------------------------------------------------------------------------------------------------
void motor_init(void)
{
    // 获取PWM设备信息
    pwm_get_dev_info(MOTOR1_PWM, &motor_1_pwm_info);
    pwm_get_dev_info(MOTOR2_PWM, &motor_2_pwm_info);
}

//------------------------------------------------------------------------------------------------
//函数名称    motor_stop
//参数说明    void
//返回参数    void
//使用实例    motor_stop();    // 停止电机
//------------------------------------------------------------------------------------------------
void motor_stop(void)
{
    pwm_set_duty(MOTOR1_PWM, 0);   
    pwm_set_duty(MOTOR2_PWM, 0);    
}

//------------------------------------------------------------------------------------------------
//函数名称    motor_dir_set
//参数说明    dir            方向 true正转 false反转
//返回参数    int            0成功 -1失败
//使用实例    motor_dir_set(true);    // 设置电机正转
//------------------------------------------------------------------------------------------------
int motor_dir_set(bool dir)
{
    if(dir) // 正转
    {
        gpio_set_level(MOTOR1_DIR, 1); // DIR输出高电平
        gpio_set_level(MOTOR2_DIR, 1); // DIR输出高电平
    }
    else    // 反转
    {
        gpio_set_level(MOTOR1_DIR, 0); // DIR输出低电平
        gpio_set_level(MOTOR2_DIR, 0); // DIR输出低电平
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
//函数名称    motor_set_speed
//参数说明    duty            占空比 0~MAX_DUTY
//返回参数    void
//使用实例    motor_set_speed(20);    // 设置电机占空比为2%
//备注信息    该函数仅设置占空比，方向需要通过motor_dir_set函数设置
//备注信息    为了追求更高的精度,我把占空比缩小了十分之一,所以现在是100的MAX_DUTY是用0~1000的duty来控制
//------------------------------------------------------------------------------------------------
void motor_set_left_speed(int duty)
{
    if (duty >= MAX_DUTY)
    {
        duty = MAX_DUTY;
    }
    pwm_set_duty(MOTOR1_PWM, duty* (MOTOR1_PWM_DUTY_MAX / 1000));
}
void motor_set_right_speed(int duty)
{
    if (duty >= MAX_DUTY)
    {
        duty = MAX_DUTY;
    }
    pwm_set_duty(MOTOR2_PWM, duty* (MOTOR2_PWM_DUTY_MAX / 1000));
}

//------------------------------------------------------------------------------------------------
//函数名称    motor_get_speed
//参数说明    void
//返回参数    int            电机编码器计数值
//使用实例    int left_speed = motor_get_left_speed();    // 获取左电机编码器计数值
//------------------------------------------------------------------------------------------------
int motor_get_left_speed(void)
{
    encoder_update();
    return encoder_left;
}
int motor_get_right_speed(void)
{
    encoder_update();
    return encoder_right;
}