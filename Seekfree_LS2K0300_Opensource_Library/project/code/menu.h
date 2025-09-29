#ifndef MENU_H
#define MENU_H

#include <stdint.h>

// 菜单接口
int menu_main(void);                 // 主功能菜单示例
int menu_image_view(void);               // 图像处理/运行演示菜单
int menu2_2(void);               // 简单确认/返回菜单
int menu_pid_config(void);       // 直接编辑 PID 参数的菜单
int menu_image_config(void);      // 图像参数配置菜单

// ---- 兼容旧版本接口（迁移期保留） ----
// 旧代码中使用的结构与函数：param_config_t / menu_param_config / param_flash_read / param_flash_write
// 现在参数直接作用于全局 pid_param，因此包装为调用新的实现。
// 轻量占位结构：若旧代码有 extern param_config_t params; 可继续链接（内部不再使用）。
typedef struct {
	float Kp_dir, Ki_dir, Kd_dir;      // 方向 PID (旧: Kp_dir, Kd_dir + kd_diff 组合，这里保留三项便于扩展)
	float Kp_speed, Ki_speed, Kd_speed;// 速度 PID
	float base_speed, target_speed;    // 速度设定
} param_config_t;

// 返回一个静态映射自 pid_param 的快照（修改它不再自动回写——鼓励调用新菜单）
param_config_t * get_param_config(void);
int menu_param_config(void);          // 兼容旧名 -> 调用 menu_pid_config
void param_flash_read(void);          // 兼容旧名 -> pid_param_flash_read
void param_flash_write(void);         // 兼容旧名 -> pid_param_flash_write

// PID 参数持久化（速度 PID + 方向 PID + 扩展：base_speed,target_speed）
void pid_param_flash_read(void);
void pid_param_flash_write(void);
float * menu_get_base_speed_ptr(void);      // 基础速度引用
float * menu_get_target_speed_ptr(void);    // 目标速度引用

#endif // MENU_H