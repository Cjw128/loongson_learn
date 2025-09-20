//本代码为用户自己编写的针对龙芯2k0300逐飞开源库及开发板母板的编码器控制文件
#include "zf_common_headfile.h"
#include "encoder.h"
#include "Header.h"
/********************************************用户代码*********************************************/
int16 encoder_left;
int16 encoder_right;

extern struct pwm_info motor_1_pwm_info;
extern struct pwm_info motor_2_pwm_info;
//------------------------------------------------------------------------------------------------
//函数名称    encoder_init
//参数说明    void
//返回参数    void
//使用实例    encoder_init();    // 初始化编码器
//------------------------------------------------------------------------------------------------
void encoder_init(void)
{
    encoder_left = 0;
    encoder_right = 0;
}

//------------------------------------------------------------------------------------------------
//函数名称    encoder_update
//参数说明    void
//返回参数    void
//使用实例    encoder_update();
//备注信息    需要周期调用该函数，更新编码器数值,否则会累计
//备注信息    编码器的值放在 encoder_left 和 encoder_right 变量中    
//------------------------------------------------------------------------------------------------
void encoder_update(void)
{
    encoder_left = encoder_get_count(ENCODER_1);
    encoder_right = encoder_get_count(ENCODER_2);
}
