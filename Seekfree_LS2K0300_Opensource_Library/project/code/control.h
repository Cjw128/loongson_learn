#ifndef CONTROL_COMPLEMENTARY_H
#define CONTROL_COMPLEMENTARY_H

// 互补滤波器参数结构体（单一定义，供所有引用该头文件的源文件使用）
typedef struct {
    float alpha;   // 互补滤波器系数 (0-1)，通常取0.98-0.99
    float dt;      // 采样时间间隔 (秒)
    float roll;    // 滤波后的横滚角 (度)
    float pitch;   // 滤波后的俯仰角 (度)
    float yaw;     // 滤波后的偏航角 (度)
} complementary_filter_t;

// 兼容C/C++接口
#ifdef __cplusplus
extern "C" {
#endif

// 初始化互补滤波器
void complementary_filter_init(complementary_filter_t *filter, float alpha, float dt);

// 更新互补滤波（输入当前一次采样的加速度计与陀螺仪原始数据）
void complementary_filter_update(complementary_filter_t *filter,
                                 float acc_x, float acc_y, float acc_z,
                                 float gyro_x, float gyro_y, float gyro_z);

// 获取当前滤波后的姿态角
void get_filtered_angles(complementary_filter_t *filter,
                         float *roll, float *pitch, float *yaw);

#ifdef __cplusplus
}
#endif

#endif // CONTROL_COMPLEMENTARY_H