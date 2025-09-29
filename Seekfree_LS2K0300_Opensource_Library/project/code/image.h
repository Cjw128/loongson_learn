#ifndef IMAGE_H
#define IMAGE_H


/**********************************图像相关参数**********************************/
extern uint8_t *rgay_image;                 // 原图指针 (由 uvc 模块提供)
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
/**********************************运行期可调图像参数**********************************/
// 显示模式：0 灰度原图  1 赛道处理（显示处理结果/边线）
typedef enum { IMG_MODE_GRAY = 0, IMG_MODE_TRACK = 1 } img_display_mode_t;
extern volatile int g_img_display_mode;
// 运行期裁切参数（替代静态宏的可调值，单位像素）
extern volatile int g_crop_top;    // 0..VIS_ORI_H-10
extern volatile int g_crop_bottom; // 0..VIS_ORI_H-10
extern volatile int g_crop_left;   // 0..VIS_ORI_W-10
extern volatile int g_crop_right;  // 0..VIS_ORI_W-10

void image_param_load(void);   // 读取持久化参数
void image_param_save(void);   // 保存持久化参数
void image_apply_crops(int *x,int *y,int *w,int *h); // 计算当前有效 ROI
/**********************************运行期可调图像参数**********************************/
/**********************************图像相关参数**********************************/

/**********************************路径相关参数**********************************/
typedef uint8 linelist[VIS_H];
extern linelist l_border;
extern linelist r_border;
extern linelist center_line;

extern float err;

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
void vis_cal_centerline(void);
void vis_image_process(void);
void vis_draw_line(void);
void vis_cal_err(void);
/**********************************函数部分**********************************/




#endif