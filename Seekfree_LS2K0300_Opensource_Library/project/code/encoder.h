#ifndef ENCODER_H
#define ENCODER_H

/********************************************编码器定义*********************************************/
extern int16 encoder_left;
extern int16 encoder_right;
#define ENCODER_1           "/dev/zf_encoder_1"
#define ENCODER_2           "/dev/zf_encoder_2" //定义来自设备树
/********************************************编码器定义*********************************************/

void encoder_init(void);
void encoder_update(void);


#endif