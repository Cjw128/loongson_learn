
// 重新整理：融合旧菜单模式 + 新 PID 菜单（兼容旧接口）
#include "zf_common_headfile.h"
#include "menu.h"
#include "PID.h"
#include "image.h"
#include <stdio.h>

// 分离菜单模式，不再使用统一配置函数

// 可选：串口重定向，如需要请取消注释并修改设备
// #include <cstdio>
// #include <unistd.h>
// #include <fcntl.h>
// freopen("/dev/ttyS0", "w", stdout);

void device_init(void)
{
    user_key_init(10);          // 按键扫描周期 10ms
    ips200_init("/dev/fb0");
    vis_camera_init("/dev/video0"); // 摄像头初始化
    motor_init();
    encoder_init();
}

static void pid_init_defaults(void)
{
    pid_reset(&motor_dir, 2.0f, 0.00f, 0.10f, 200.0f,  500.0f, -500.0f);
    pid_reset(&motor_l,   1.5f, 0.05f, 0.00f, 500.0f, 1000.0f, -1000.0f);
    pid_reset(&motor_r,   1.5f, 0.05f, 0.00f, 500.0f, 1000.0f, -1000.0f);
}

typedef enum {
    APP_MENU_MAIN = 1,
    APP_MENU_PID_CFG,
    APP_MENU_IMAGE_VIEW,
    APP_MENU_IMAGE_PARAM,
} app_menu_state_t;

int main(void)
{
    printf("\r\n==== System Boot ====\r\n");
    //printf("[OK ] LCD init.\r\n");
    //pid_init_defaults();
    pid_param_flash_read();    // 加载保存的 PID/base/target
    image_param_load();        // 加载图像显示/裁切参数
    app_menu_state_t state = APP_MENU_MAIN;
    int menu_ret = 0;

    while(1)
    {
        switch(state)
        {
            case APP_MENU_MAIN:
                // 主菜单：返回值 1..4
                menu_ret = menu_main();
                if(menu_ret == 1) { state = APP_MENU_MAIN; }          // demo 占位
                else if(menu_ret == 2) { state = APP_MENU_PID_CFG; }
                else if(menu_ret == 3) { state = APP_MENU_IMAGE_PARAM; }
                else if(menu_ret == 4) { state = APP_MENU_IMAGE_VIEW; }
                break;
            case APP_MENU_PID_CFG:
                menu_pid_config();
                state = APP_MENU_MAIN; break;
            case APP_MENU_IMAGE_VIEW:
                menu_image_view(); // 图像显示/处理循环，长按 KEY4 返回
                state = APP_MENU_MAIN; break;
            case APP_MENU_IMAGE_PARAM:
                menu_image_config();
                state = APP_MENU_MAIN; break;
            default:
                state = APP_MENU_MAIN;
                break;
        }
        system_delay_ms(20); // 状态机循环节拍
    }
    return 0;
}