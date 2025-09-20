#ifndef IMAGE_H
#define IMAGE_H


/**********************************图像相关参数**********************************/
extern uint8_t *rgay_image;                 // 原图指针
#define VIS_ORI_H           UVC_HEIGHT      // 原图高度
#define VIS_ORI_W           UVC_WIDTH       // 原图宽度
#define VIS_TOP_CUT         (0)             // 顶部裁剪像素数量
#define VIS_BOT_CUT         (0)             // 底部裁剪像素数量
#define VIS_LEFT_CUT        (0)             // 左侧裁剪像素数量
#define VIS_RIGHT_CUT       (0)             // 右侧裁剪像素数量
#define VIS_H              (VIS_ORI_H - VIS_TOP_CUT - VIS_BOT_CUT)      // 有效图高度
#define VIS_W              (VIS_ORI_W - VIS_LEFT_CUT - VIS_RIGHT_CUT)   // 有效图宽度
#define VIS_SIZE           (VIS_H * VIS_W)  // 有效图像素数量
#define VIS_BORDER_MIN    (0)             // 边界最小值
#define VIS_BORDER_MAX    (255)           // 边界最大值
/**********************************图像相关参数**********************************/

/**********************************路径相关参数**********************************/
typedef uint8 linelist[VIS_H];
extern linelist l_border;
extern linelist r_border;
extern linelist center_line;


/**********************************路径相关参数**********************************/
/**********************************函数部分**********************************/
int vis_camera_init(const char* device); 
void vis_otsu();
uint8 vis_get_start_point(uint8 start_row);
void vis_search_l_r(uint16 break_flag, uint8(*image)[VIS_W], uint16 *l_stastic, uint16 *r_stastic, uint8 l_start_x, uint8 l_start_y, uint8 r_start_x, uint8 r_start_y, uint8*hightest);
void vis_get_left(uint16 total_L);
void vis_get_right(uint16 total_R);
void vis_image_filter(uint8(*bin_image)[VIS_W]);
void vis_image_draw_rectan(uint8(*image)[VIS_W]);
void vis_image_process(void);
void vis_draw_line(void);
/**********************************函数部分**********************************/




#endif