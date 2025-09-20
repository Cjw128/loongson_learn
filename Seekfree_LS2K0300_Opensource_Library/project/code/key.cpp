//本文件为用户自己编写的针对龙芯2k0300逐飞开源库及开发板母板的按键操作文件
#include "zf_common_headfile.h"
#include "key.h"
#include "Header.h"

/********************************************按键定义*********************************************/
#define KEY_4       "/dev/zf_driver_gpio_key_0"
#define KEY_3       "/dev/zf_driver_gpio_key_1"
#define KEY_2       "/dev/zf_driver_gpio_key_2"
#define KEY_1       "/dev/zf_driver_gpio_key_3"


static uint32_t scanner_period = 0;
static uint32_t key_press_time[KEY_NUMBER] = {0};
static key_state_enum key_state[KEY_NUMBER] = {KEY_RELEASE};

static const char* key_dev_path[KEY_NUMBER] = {KEY_1, KEY_2, KEY_3, KEY_4};
/********************************************按键定义*********************************************/

/********************************************用户代码*********************************************/
//------------------------------------------------------------------------------------------------
//函数名称    user_key_init
//参数说明    period            扫描周期 单位 ms
//返回参数    void
//使用实例    key_init(10);    // 初始化按键，扫描周期10ms
//------------------------------------------------------------------------------------------------
void user_key_init(uint32_t period)
{
    int i;
    scanner_period = period;
    for(i=0; i<KEY_NUMBER; i++)
    {
        // 设备节点已由驱动和设备树初始化，无需手动设置方向和上拉
        key_press_time[i] = 0;
        key_state[i] = KEY_RELEASE;
    }
}

//------------------------------------------------------------------------------------------------
//函数名称    key_scanner
//参数说明    void
//返回参数    void
//使用实例    key_scanner();    // 按键扫描函数，需放在定时器中断或主循环中周期调用
//------------------------------------------------------------------------------------------------
void key_scanner(void)
{
    int i;
    for(i=0; i<KEY_NUMBER; i++)
    {
        int level = gpio_get_level(key_dev_path[i]); // 直接用设备节点路径
        if(level == 0)      // 按键按下
        {
            key_press_time[i] += scanner_period;
            if((key_press_time[i] >= 1000) && (key_state[i] == KEY_RELEASE))   // 长按
            {
                key_state[i] = KEY_LONG_PRESS;
            }
        }
        else                                        // 按键松开
        {
            if((key_press_time[i] < 1000) && (key_press_time[i] > 0) && (key_state[i] == KEY_RELEASE)) // 短按
            {
                key_state[i] = KEY_SHORT_PRESS;
            }
            key_press_time[i] = 0;
            if(key_state[i] == KEY_LONG_PRESS)       // 长按松开
            {
                key_state[i] = KEY_RELEASE;
            }
        }
    }
}
//------------------------------------------------------------------------------------------------
//函数名称    key_get_state
//参数说明    key_n            按键编号 KEY_1~KEY_4
//返回参数    key_init_state_enum    按键状态 KEY_RELEASE/KEY_SHORT_PRESS   KEY_LONG_PRESS
//使用实例    key_get_state(KEY_1);    // 获取按键1状态
//------------------------------------------------------------------------------------------------
key_state_enum key_get_state(key_index_enum key_n)
{
    if(key_n < KEY_NUMBER)
    {
        return key_state[key_n];
    }
    return KEY_RELEASE;
}

//------------------------------------------------------------------------------------------------
//函数名称    key_clear_state
//参数说明    key_n            按键编号 KEY_1~KEY_4
//返回参数    void
//使用实例    key_clear_state(KEY_1);    // 清除按键1状态
//------------------------------------------------------------------------------------------------
void key_clear_state(key_index_enum key_n)
{
    if(key_n < KEY_NUMBER)
    {
        key_state[key_n] = KEY_RELEASE;
    }
}

//------------------------------------------------------------------------------------------------
//函数名称    key_clear_all_state
//参数说明    void
//返回参数    void
//使用实例    key_clear_all_state();    // 清除所有按键状态
//------------------------------------------------------------------------------------------------
void key_clear_all_state(void)
{
    int i;
    for(i=0; i<KEY_NUMBER; i++)
    {
        key_state[i] = KEY_RELEASE;
    }
}

/********************************************用户代码*********************************************/