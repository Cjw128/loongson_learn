#ifndef MOTOR_H
#define MOTOR_H

/********************************************电机定义*********************************************/
#define MOTOR1_DIR   "/dev/zf_driver_gpio_motor_1"
#define MOTOR1_PWM   "/dev/zf_device_pwm_motor_1"

#define MOTOR2_DIR   "/dev/zf_driver_gpio_motor_2"
#define MOTOR2_PWM   "/dev/zf_device_pwm_motor_2"   //定义来自设备树

#define MAX_DUTY        (30 )   // 最大 MAX_DUTY% 占空比

#define MOTOR1_PWM_DUTY_MAX    (10000)       // 在设备树中，设置的10000。如果要修改，需要与设备树对应。
#define MOTOR2_PWM_DUTY_MAX    (10000)       // 在设备树中，设置的10000。如果要修改，需要与设备树对应。

extern struct pwm_info motor_1_pwm_info;
extern struct pwm_info motor_2_pwm_info;            //pwm信息结构体
/********************************************电机定义*********************************************/

/********************************************函数定义*********************************************/
void motor_init(void);
void motor_stop(void);
int motor_dir_set(bool dir);
void motor_set_left_speed(uint32_t duty);
void motor_set_right_speed(uint32_t duty);
int motor_get_left_speed(void);
int motor_get_right_speed(void);

/********************************************函数定义*********************************************/
#endif