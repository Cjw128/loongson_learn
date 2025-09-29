// 仅保留函数实现，类型定义放在 control.h
#include "control.h"
#include "zf_common_headfile.h" // 若其它模块需要此公共头文件
#include "Header.h"             // 如果项目中有依赖（原文件包含，保留）
#include <math.h>

#define RAD_TO_DEG 57.2957795131f

void complementary_filter_init(complementary_filter_t *cf, float alpha, float dt)
{
    if(!cf) return;
    cf->alpha = alpha;
    cf->dt = dt;
    cf->roll = 0.0f;
    cf->pitch = 0.0f;
    cf->yaw = 0.0f;
}

void complementary_filter_update(complementary_filter_t *cf,
                                 float acc_x, float acc_y, float acc_z,
                                 float gyro_x, float gyro_y, float gyro_z)
{
    if(!cf) return;

    float acc_magnitude = sqrtf(acc_x * acc_x + acc_y * acc_y + acc_z * acc_z);
    if (acc_magnitude < 0.001f) {
        acc_magnitude = 0.001f; // 防止除零
    }
    float acc_x_norm = acc_x / acc_magnitude;
    float acc_y_norm = acc_y / acc_magnitude;
    float acc_z_norm = acc_z / acc_magnitude;

    float acc_roll  = atan2f(acc_y_norm, acc_z_norm) * RAD_TO_DEG;
    float acc_pitch = -asinf(acc_x_norm) * RAD_TO_DEG;

    // 陀螺仪积分
    float gyro_roll  = cf->roll  + gyro_x * cf->dt;
    float gyro_pitch = cf->pitch + gyro_y * cf->dt;
    float gyro_yaw   = cf->yaw   + gyro_z * cf->dt;

    // 互补融合
    float a = cf->alpha;
    cf->roll  = a * gyro_roll  + (1.0f - a) * acc_roll;
    cf->pitch = a * gyro_pitch + (1.0f - a) * acc_pitch;
    cf->yaw   = gyro_yaw; // 无加速度校正

    // 角度范围归一化到 [-180, 180]
    if (cf->roll  > 180.0f) cf->roll  -= 360.0f; else if (cf->roll  < -180.0f) cf->roll  += 360.0f;
    if (cf->pitch > 180.0f) cf->pitch -= 360.0f; else if (cf->pitch < -180.0f) cf->pitch += 360.0f;
    if (cf->yaw   > 180.0f) cf->yaw   -= 360.0f; else if (cf->yaw   < -180.0f) cf->yaw   += 360.0f;
}

void get_filtered_angles(complementary_filter_t *cf, float *roll, float *pitch, float *yaw)
{
    if(!cf) return;
    if(roll)  *roll  = cf->roll;
    if(pitch) *pitch = cf->pitch;
    if(yaw)   *yaw   = cf->yaw;
}

