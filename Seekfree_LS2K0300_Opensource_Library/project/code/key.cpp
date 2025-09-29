// 按键驱动重写：消抖 + 一次性事件（短按只报告一次，长按只在阈值达到时报告一次）
#include "zf_common_headfile.h"
#include "key.h"
#include "Header.h"

// 设备节点（低电平表示按下）
#define KEY_4_PATH "/dev/zf_driver_gpio_key_0"
#define KEY_3_PATH "/dev/zf_driver_gpio_key_1"
#define KEY_2_PATH "/dev/zf_driver_gpio_key_2"
#define KEY_1_PATH "/dev/zf_driver_gpio_key_3"

#ifndef KEY_LONG_PRESS_MS
#define KEY_LONG_PRESS_MS 1000u
#endif
#ifndef KEY_DEBOUNCE_COUNT
#define KEY_DEBOUNCE_COUNT 3u     // 连续 N 次采样一致才认为稳定
#endif

static const char* key_dev_path[KEY_NUMBER] = {KEY_1_PATH, KEY_2_PATH, KEY_3_PATH, KEY_4_PATH};

static uint32_t scanner_period = 0;                 // ms
static uint32_t press_time[KEY_NUMBER] = {0};       // 当前按下累计时长
static uint8_t  stable_cnt[KEY_NUMBER]  = {0};      // 消抖计数
static uint8_t  last_raw[KEY_NUMBER]    = {1,1,1,1}; // 上一次原始电平（1=松开）
static uint8_t  debounced_pressed[KEY_NUMBER] = {0}; // 稳定按下状态
static uint8_t  long_reported[KEY_NUMBER] = {0};     // 是否已上报长按
static key_state_enum key_event[KEY_NUMBER] = {KEY_RELEASE}; // 事件：短按或长按（一次性）

void user_key_init(uint32_t period)
{
    scanner_period = period;
    for(int i=0;i<KEY_NUMBER;i++)
    {
        press_time[i] = 0;
        stable_cnt[i] = 0;
        last_raw[i] = 1;
        debounced_pressed[i] = 0;
        long_reported[i] = 0;
        key_event[i] = KEY_RELEASE;
    }
}

void key_scanner(void)
{
    for(int i=0;i<KEY_NUMBER;i++)
    {
        int level = gpio_get_level(key_dev_path[i]); // 读取电平 0=按下 1=松开（假设驱动如此）
        uint8_t raw_pressed = (level == 0) ? 1 : 0;

        if(raw_pressed == (last_raw[i]==0?1:0))
        {
            // 逻辑有点绕：last_raw 保存的是电平本身(0/1)。raw_pressed 是按下态(1)或松开(0)。
        }
        // 消抖：同一原始电平连续 KEY_DEBOUNCE_COUNT 次才确认
        if(level == last_raw[i])
        {
            if(stable_cnt[i] < 0xFF) stable_cnt[i]++;
        }
        else
        {
            last_raw[i] = level;
            stable_cnt[i] = 1;
        }
        if(stable_cnt[i] < KEY_DEBOUNCE_COUNT) continue; // 未稳定

        uint8_t new_pressed = raw_pressed; // 稳定逻辑
        if(new_pressed != debounced_pressed[i])
        {
            // 发生稳定状态翻转
            if(new_pressed) // 按下沿
            {
                press_time[i] = 0;
                long_reported[i] = 0;
            }
            else // 松开沿
            {
                if(!long_reported[i] && press_time[i] < KEY_LONG_PRESS_MS)
                {
                    key_event[i] = KEY_SHORT_PRESS; // 生成短按事件
                }
                // 如果已经 long_reported，则长按事件之前已经上报过，这里不再产生事件
                press_time[i] = 0;
                long_reported[i] = 0;
            }
            debounced_pressed[i] = new_pressed;
        }
        else if(new_pressed) // 按下保持中
        {
            if(press_time[i] < 0xFFFFFF00u) press_time[i] += scanner_period;
            if(!long_reported[i] && press_time[i] >= KEY_LONG_PRESS_MS)
            {
                key_event[i] = KEY_LONG_PRESS; // 只上报一次
                long_reported[i] = 1;
            }
        }
    }
}

// 一次性读取事件：返回 SHORT 或 LONG 后会自动清除
key_state_enum key_get_state(key_index_enum key_n)
{
    if(key_n >= KEY_NUMBER) return KEY_RELEASE;
    key_state_enum ev = key_event[key_n];
    if(ev == KEY_SHORT_PRESS || ev == KEY_LONG_PRESS)
    {
        key_event[key_n] = KEY_RELEASE; // 自动消费
    }
    return ev;
}

void key_clear_state(key_index_enum key_n)
{
    if(key_n < KEY_NUMBER) key_event[key_n] = KEY_RELEASE;
}

void key_clear_all_state(void)
{
    for(int i=0;i<KEY_NUMBER;i++) key_event[i] = KEY_RELEASE;
}

// 可选：查询当前是否处于按下（持续态，已消抖）
int key_is_pressed(key_index_enum key_n)
{
    if(key_n < KEY_NUMBER) return debounced_pressed[key_n];
    return 0;
}