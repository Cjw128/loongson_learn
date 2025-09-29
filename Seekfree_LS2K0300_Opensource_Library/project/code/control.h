#ifndef CONTROL_COMPLEMENTARY_H
#define CONTROL_COMPLEMENTARY_H

// �����˲��������ṹ�壨��һ���壬���������ø�ͷ�ļ���Դ�ļ�ʹ�ã�
typedef struct {
    float alpha;   // �����˲���ϵ�� (0-1)��ͨ��ȡ0.98-0.99
    float dt;      // ����ʱ���� (��)
    float roll;    // �˲���ĺ���� (��)
    float pitch;   // �˲���ĸ����� (��)
    float yaw;     // �˲����ƫ���� (��)
} complementary_filter_t;

// ����C/C++�ӿ�
#ifdef __cplusplus
extern "C" {
#endif

// ��ʼ�������˲���
void complementary_filter_init(complementary_filter_t *filter, float alpha, float dt);

// ���»����˲������뵱ǰһ�β����ļ��ٶȼ���������ԭʼ���ݣ�
void complementary_filter_update(complementary_filter_t *filter,
                                 float acc_x, float acc_y, float acc_z,
                                 float gyro_x, float gyro_y, float gyro_z);

// ��ȡ��ǰ�˲������̬��
void get_filtered_angles(complementary_filter_t *filter,
                         float *roll, float *pitch, float *yaw);

#ifdef __cplusplus
}
#endif

#endif // CONTROL_COMPLEMENTARY_H