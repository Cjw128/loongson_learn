#ifndef _KEY_H_
#define _KEY_H_

#include <stdint.h>

typedef enum {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_NUMBER
} key_index_enum;

typedef enum {
    KEY_RELEASE = 0,
    KEY_SHORT_PRESS,
    KEY_LONG_PRESS
} key_state_enum;

void user_key_init(uint32_t period);
void key_scanner(void);
key_state_enum key_get_state(key_index_enum key_n);
void key_clear_state(key_index_enum key_n);
void key_clear_all_state(void);
// 查询当前是否按下（经过消抖的稳定态），用于长按加速逻辑
int key_is_pressed(key_index_enum key_n);

#endif