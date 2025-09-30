#include "zf_common_headfile.h"
#include "Header.h"
#include "menu.h"
#include "flash.h"
#include "key.h"
#include "image.h"
#include "PID.h"

/******************************************用户代码******************************************/

// Flash 模拟存储路径（根据需要修改为实际可写路径）
#ifndef PARAM_STORE_PATH
#define PARAM_STORE_PATH "/home/root/param_cfg.txt"
#endif

// ---------------- 直接使用 PID 全局结构 + 额外基础速度、目标速度 + 扩展 PID 限幅参数 ----------------
static float g_base_speed   = 43.0f;
static float g_target_speed = 50.0f;

// 若首次无文件，提供默认 PID 参数（原来 scattered 在 main）
static void pid_set_defaults(void)
{
    // 方向 PID
    motor_dir.kp = 2.0f; motor_dir.ki = 0.00f; motor_dir.kd = 0.10f;
    motor_dir.integral_limit = 200.0f; motor_dir.omax = 500.0f; motor_dir.omin = -500.0f;
    motor_dir.integral = 0; motor_dir.err = motor_dir.last_error = motor_dir.previous_error = 0; motor_dir.output = 0; motor_dir.target = 0; motor_dir.actual = 0;
    // 速度 PID（左右共用参数，右侧稍后同步）
    motor_l.kp = 1.5f; motor_l.ki = 0.05f; motor_l.kd = 0.00f;
    motor_l.integral_limit = 500.0f; motor_l.omax = 1000.0f; motor_l.omin = -1000.0f;
    motor_l.integral = 0; motor_l.err = motor_l.last_error = motor_l.previous_error = 0; motor_l.output = 0; motor_l.target = 0; motor_l.actual = 0;
    motor_r = motor_l; // 同步
}

float * menu_get_base_speed_ptr(void)   { return &g_base_speed; }
float * menu_get_target_speed_ptr(void) { return &g_target_speed; }

// 兼容旧 param_config_t 占位与包装
static param_config_t g_param_snapshot; // 仅用于旧接口兼容显示，不作为真实数据源
param_config_t * get_param_config(void)
{
    g_param_snapshot.Kp_dir = motor_dir.kp;
    g_param_snapshot.Ki_dir = motor_dir.ki;
    g_param_snapshot.Kd_dir = motor_dir.kd;
    g_param_snapshot.Kp_speed = motor_l.kp;
    g_param_snapshot.Ki_speed = motor_l.ki;
    g_param_snapshot.Kd_speed = motor_l.kd;
    g_param_snapshot.base_speed = g_base_speed;
    g_param_snapshot.target_speed = g_target_speed;
    return &g_param_snapshot;
}

// 持久化格式：
// 版本P2:  "P2" 后接 14 个浮点 (dir: kp ki kd i_lim omax omin) (spd: kp ki kd i_lim omax omin) base target
// 旧版本(无P2) 8 个浮点：dir(kp ki kd) spd(kp ki kd) base target   -> 读取后补默认限幅
void pid_param_flash_write(void)
{
    FILE *fp = fopen(PARAM_STORE_PATH, "w");
    if(!fp) { printf("[PID] save failed(open) %s\r\n", PARAM_STORE_PATH); return; }
    fprintf(fp,
        "P2\n"
        "Dir_Kp %.6f\nDir_Ki %.6f\nDir_Kd %.6f\nDir_ILim %.6f\nDir_OMax %.6f\nDir_OMin %.6f\n"
        "Spd_Kp %.6f\nSpd_Ki %.6f\nSpd_Kd %.6f\nSpd_ILim %.6f\nSpd_OMax %.6f\nSpd_OMin %.6f\n"
        "Base %.6f\nTarget %.6f\n",
        motor_dir.kp, motor_dir.ki, motor_dir.kd, motor_dir.integral_limit, motor_dir.omax, motor_dir.omin,
        motor_l.kp,   motor_l.ki,   motor_l.kd,   motor_l.integral_limit,   motor_l.omax,   motor_l.omin,
        g_base_speed, g_target_speed
    );
    fclose(fp);
    printf("[PID] saved to %s\r\n", PARAM_STORE_PATH);
}

void pid_param_flash_read(void)
{
    FILE *fp = fopen(PARAM_STORE_PATH, "r");
    if(!fp) {
        printf("[PID] no existing file, load defaults\r\n");
        pid_set_defaults();
        return;
    }
    char head[4] = {0};
    if(1 == fscanf(fp, "%3s", head) && 0 == strcmp(head, "P2"))
    {
        int n = fscanf(fp, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                       &motor_dir.kp, &motor_dir.ki, &motor_dir.kd, &motor_dir.integral_limit, &motor_dir.omax, &motor_dir.omin,
                       &motor_l.kp,   &motor_l.ki,   &motor_l.kd,   &motor_l.integral_limit,   &motor_l.omax,   &motor_l.omin,
                       &g_base_speed, &g_target_speed);
        if(n != 14) {
            printf("[PID] P2 format mismatch(%d) -> defaults\r\n", n);
            pid_set_defaults();
        }
    }
    else
    {
        // 回退旧格式：回到文件开头重新解析 8 参数
        fseek(fp, 0, SEEK_SET);
        int n = fscanf(fp, "%f %f %f %f %f %f %f %f",
                       &motor_dir.kp, &motor_dir.ki, &motor_dir.kd,
                       &motor_l.kp,   &motor_l.ki,   &motor_l.kd,
                       &g_base_speed, &g_target_speed);
        pid_set_defaults(); // 先默认，后覆盖基础值
        if(n == 8)
        {
            printf("[PID] old format detected -> upgrading\r\n");
            // 覆盖 kp ki kd base target，其它 (i_lim / omax / omin) 保留默认
        }
        else
        {
            printf("[PID] read fail -> defaults\r\n");
        }
    }
    fclose(fp);
    motor_r.kp = motor_l.kp; motor_r.ki = motor_l.ki; motor_r.kd = motor_l.kd;
    motor_r.integral_limit = motor_l.integral_limit; motor_r.omax = motor_l.omax; motor_r.omin = motor_l.omin;
}

// ---------------- 按键读取小工具 ----------------
static inline uint8 key_short(key_index_enum key)
{
	key_state_enum st = key_get_state(key);
	return (st == KEY_SHORT_PRESS) ? 1u : 0u;
}

//------------------------------------------------------------------------------------------------
//函数名称    menu_main
//参数说明    void
//返回参数    int：用户选择项编号
//使用实例    int sel = menu_main();    // 1=test1 2=PID_Config 3=Image_test 4=Image_Param
//备注信息    上下(K1/K2)循环选择，KEY_3 确认；界面会用 'x' 标记当前位置
//------------------------------------------------------------------------------------------------
int menu_main(void)
{
    int sel = 1; // 1..4
    int last_sel = -1;
    ips200_clear();
    ips200_show_string(0, 100, "Menu 1: test1");
    ips200_show_string(0, 120, "PID_Config");
    ips200_show_string(0, 140, "Image_Param");
    ips200_show_string(0, 160, "Image_test");

    // 长按加速计时（与子菜单一致参数）
    uint32_t hold_dec = 0, hold_inc = 0; // KEY_1=dec KEY_2=inc
    const uint32_t first_delay=400, repeat_gap=120, accel_after=1500, fast=60;

    while(1)
    {
        key_scanner();
        // 短按一次
        if (key_short(KEY_2)) { if(++sel > 4) sel = 1; }
        if (key_short(KEY_1)) { if(--sel < 1) sel = 4; }

        // 长按加速：模拟连续触发
        int rep_dir = 0; // -1 上, +1 下
        if(key_is_pressed(KEY_1)) {
            hold_dec += 10;
            if(hold_dec > first_delay){ uint32_t g=(hold_dec>accel_after)?fast:repeat_gap; if(((hold_dec-first_delay)%g)<10) rep_dir = -1; }
        } else hold_dec = 0;
        if(key_is_pressed(KEY_2)) {
            hold_inc += 10;
            if(hold_inc > first_delay){ uint32_t g=(hold_inc>accel_after)?fast:repeat_gap; if(((hold_inc-first_delay)%g)<10) rep_dir = (rep_dir==-1)?-1:1; }
        } else hold_inc = 0;
        if(rep_dir == 1){ if(++sel > 4) sel = 1; }
        else if(rep_dir == -1){ if(--sel < 1) sel = 4; }

        // 确认
        if (key_short(KEY_3)) {
            while(key_get_state(KEY_3) != KEY_RELEASE) { key_scanner(); system_delay_ms(10); }
            return sel; // 1=test1 2=PID 3=Image_test 4=Image_Param
        }

        // 仅在变化时刷新光标，减少闪烁
        if(last_sel != sel){
            // 清除全部标记
            ips200_show_string(120,100, " ");
            ips200_show_string(120,120, " ");
            ips200_show_string(120,140, " ");
            ips200_show_string(120,160, " ");
            ips200_show_string(120,100 + (sel - 1) * 20, "<");
            last_sel = sel;
        }
        system_delay_ms(10); // 更快响应
    }
}

//------------------------------------------------------------------------------------------------
//函数名称    menu_image_view
//参数说明    void
//返回参数    int：固定 0（预留扩展）
//使用实例    menu_image_view();    // 进入实时图像显示/处理循环
//备注信息    内部阻塞抓帧并调用 vis_image_process；长按 KEY_4 退出
//------------------------------------------------------------------------------------------------
int menu_image_view(void)
{
    ips200_clear();
    ips200_show_string(0,0,"UVC VIEW");
    // 阻塞抓帧 + 显示，长按 KEY4 退出
    while(1)
    {
        if(wait_image_refresh() < 0)
        {
            ips200_show_string(0,20,"CAP FAIL");
            system_delay_ms(100);
            continue;
        }
        if(rgay_image)
        {
            //ips200_show_gray_image(0, 0, rgay_image, UVC_WIDTH, UVC_HEIGHT);
            vis_image_process();
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "ERR: %.2f", err);
        ips200_show_string(0, 120, buf);
        // 按键退出
        key_scanner();
        if(key_get_state(KEY_4) == KEY_LONG_PRESS)
        {
            while(key_get_state(KEY_4) != KEY_RELEASE){ key_scanner(); system_delay_ms(10);} 
            break;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
//函数名称    menu2_2
//参数说明    void
//返回参数    int：1=确认 0=返回
//使用实例    if(menu2_2()){ /* 确认逻辑 */ }
//备注信息    当前仅作为占位示例，可根据业务替换
//------------------------------------------------------------------------------------------------
int menu2_2(void)
{
    ips200_clear();
    ips200_show_string(0, 240, "Press KEY_3 confirm");
    ips200_show_string(0, 260, "Press KEY_4 back");
    while(1)
    {
        key_scanner();
        if(key_short(KEY_3)) {
            while(key_get_state(KEY_3) != KEY_RELEASE) { key_scanner(); system_delay_ms(10);} 
            return 1;
        }
        if(key_short(KEY_4)) {
            while(key_get_state(KEY_4) != KEY_RELEASE) { key_scanner(); system_delay_ms(10);} 
            return 0;
        }
        system_delay_ms(50);
    }
}

// ================= 分离版 PID 参数菜单 =================
//------------------------------------------------------------------------------------------------
//函数名称    menu_pid_config
//参数说明    void
//返回参数    int：0=正常返回（保存或退出）
//使用实例    menu_pid_config();    // 进入 PID 参数调整界面
//备注信息    K2 换行；K1/K3 调整；支持长按加速；K4 保存并返回；右轮 PID 自动同步左轮
//------------------------------------------------------------------------------------------------
typedef enum {
    PID_DIR_KP = 0,
    PID_DIR_KI,
    PID_DIR_KD,
    PID_DIR_ILIM,
    PID_DIR_OMAX,
    PID_DIR_OMIN,
    PID_SPD_KP,
    PID_SPD_KI,
    PID_SPD_KD,
    PID_SPD_ILIM,
    PID_SPD_OMAX,
    PID_SPD_OMIN,
    PID_BASE_SPEED,
    PID_TARGET_SPEED,
    PID_ITEM_TOTAL
} pid_item_t;

static const float s_pid_step[PID_ITEM_TOTAL] = {
    0.1f, 0.01f, 0.05f,   // dir kp ki kd
    10.0f, 10.0f, 10.0f,  // dir i_lim omax omin
    0.1f, 0.01f, 0.05f,   // spd kp ki kd
    20.0f, 20.0f, 20.0f,  // spd i_lim omax omin
    5.0f, 1.0f            // base target
};

#define PID_VISIBLE_LINES 8
#define PID_LINE_HEIGHT   20
#define PID_START_Y       50

typedef struct {
    const char *label; float *value; uint8_t width; uint8_t frac;
} pid_line_desc_t;
static pid_line_desc_t pid_lines[PID_ITEM_TOTAL];

// 绑定每一行的显示与变量指针
static void pid_lines_bind(void)
{
    pid_lines[PID_DIR_KP]      = (pid_line_desc_t){"Dir Kp :", &motor_dir.kp, 5,2};
    pid_lines[PID_DIR_KI]      = (pid_line_desc_t){"Dir Ki :", &motor_dir.ki, 5,2};
    pid_lines[PID_DIR_KD]      = (pid_line_desc_t){"Dir Kd :", &motor_dir.kd, 5,2};
    pid_lines[PID_DIR_ILIM]    = (pid_line_desc_t){"Dir IL :", &motor_dir.integral_limit, 6,1};
    pid_lines[PID_DIR_OMAX]    = (pid_line_desc_t){"Dir OMax:", &motor_dir.omax, 6,1};
    pid_lines[PID_DIR_OMIN]    = (pid_line_desc_t){"Dir OMin:", &motor_dir.omin, 6,1};
    pid_lines[PID_SPD_KP]      = (pid_line_desc_t){"Spd Kp :", &motor_l.kp, 5,2};
    pid_lines[PID_SPD_KI]      = (pid_line_desc_t){"Spd Ki :", &motor_l.ki, 5,2};
    pid_lines[PID_SPD_KD]      = (pid_line_desc_t){"Spd Kd :", &motor_l.kd, 5,2};
    pid_lines[PID_SPD_ILIM]    = (pid_line_desc_t){"Spd IL :", &motor_l.integral_limit, 6,1};
    pid_lines[PID_SPD_OMAX]    = (pid_line_desc_t){"Spd OMax:", &motor_l.omax, 6,1};
    pid_lines[PID_SPD_OMIN]    = (pid_line_desc_t){"Spd OMin:", &motor_l.omin, 6,1};
    pid_lines[PID_BASE_SPEED]  = (pid_line_desc_t){"Base    :", &g_base_speed, 5,1};
    pid_lines[PID_TARGET_SPEED]= (pid_line_desc_t){"Target  :", &g_target_speed, 5,1};
}

// 数值合法化（防止出现负限幅等）
static void pid_value_sanitize(int index)
{
    switch(index){
        case PID_DIR_ILIM: if(motor_dir.integral_limit < 0) motor_dir.integral_limit = 0; break;
        case PID_DIR_OMAX: if(motor_dir.omax < 0) motor_dir.omax = 0; break;
        case PID_DIR_OMIN: if(motor_dir.omin > 0) motor_dir.omin = 0; break;
        case PID_SPD_ILIM: if(motor_l.integral_limit < 0) motor_l.integral_limit = 0; break;
        case PID_SPD_OMAX: if(motor_l.omax < 0) motor_l.omax = 0; break;
        case PID_SPD_OMIN: if(motor_l.omin > 0) motor_l.omin = 0; break;
        default: break;
    }
}

// 绘制当前页内容
static void pid_draw(int sel, int top)
{
    ips200_clear();
    ips200_show_string(0, 25, "PID CONFIG");
    int total_pages = (PID_ITEM_TOTAL + PID_VISIBLE_LINES - 1)/PID_VISIBLE_LINES;
    int cur_page = top / PID_VISIBLE_LINES + 1; char info[24];
    snprintf(info,sizeof(info),"Pg %d/%d",cur_page,total_pages);
    ips200_show_string(150, 25, info);
    for(int line=0; line<PID_VISIBLE_LINES; ++line){
        int idx = top + line; if(idx >= PID_ITEM_TOTAL) break; int y = PID_START_Y + line*PID_LINE_HEIGHT;
        ips200_show_string(0,y,pid_lines[idx].label);
        ips200_show_float(100,y,*pid_lines[idx].value,pid_lines[idx].width,pid_lines[idx].frac);
        ips200_show_string(180,y,(idx==sel)?"<":" ");
    }
    ips200_show_string(0, PID_START_Y + PID_VISIBLE_LINES*PID_LINE_HEIGHT + 2, "K1-/K3+ K2=Next K4=Save");
}

// 长按加速处理（返回 -1 / 1 / 0）
static int pid_key_repeat(uint32_t *hold_dec, uint32_t *hold_inc)
{
    const uint32_t first_delay=400, repeat_gap=120, accel_after=1500, fast=60; int act=0;
    if(key_is_pressed(KEY_1)){ *hold_dec+=10; if(*hold_dec>first_delay){ uint32_t g=(*hold_dec>accel_after)?fast:repeat_gap; if(((*hold_dec-first_delay)%g)<10) act=-1;} } else *hold_dec=0;
    if(key_is_pressed(KEY_3)){ *hold_inc+=10; if(*hold_inc>first_delay){ uint32_t g=(*hold_inc>accel_after)?fast:repeat_gap; if(((*hold_inc-first_delay)%g)<10) act=(act==-1)?-1:1;} } else *hold_inc=0;
    return act;
}

int menu_pid_config(void)
{
    pid_param_flash_read();
    pid_lines_bind();
    int sel=0, top=0, last_sel=-1,last_top=-1; uint32_t hold_dec=0,hold_inc=0;
    pid_draw(sel, top);
    while(1){
        key_scanner();
        uint8 k2=key_short(KEY_2), k4=key_short(KEY_4), k1=key_short(KEY_1), k3=key_short(KEY_3);
        if(k2){ if(++sel >= PID_ITEM_TOTAL) sel=0; }
        if(sel < top) top = (sel/PID_VISIBLE_LINES)*PID_VISIBLE_LINES; else if(sel >= top+PID_VISIBLE_LINES) top = (sel/PID_VISIBLE_LINES)*PID_VISIBLE_LINES;
        int rep = pid_key_repeat(&hold_dec,&hold_inc); float *pv = pid_lines[sel].value; int changed=0;
        if(pv){ if(k1||rep==-1){ *pv -= s_pid_step[sel]; changed=1; } if(k3||rep==1){ *pv += s_pid_step[sel]; changed=1; } if(changed) pid_value_sanitize(sel); }
        // 同步右轮 PID
        motor_r.kp = motor_l.kp; motor_r.ki = motor_l.ki; motor_r.kd = motor_l.kd;
        motor_r.integral_limit = motor_l.integral_limit; motor_r.omax = motor_l.omax; motor_r.omin = motor_l.omin;
        if(k4){ pid_param_flash_write(); ips200_show_string(0,240,"Saved"); system_delay_ms(300); return 0; }
        if(last_sel!=sel || last_top!=top || rep || k1 || k3){ pid_draw(sel, top); last_sel=sel; last_top=top; }
        system_delay_ms(10);
    }
}

// ================= 分离版 图像参数菜单 =================
//------------------------------------------------------------------------------------------------
//函数名称    menu_image_param
//参数说明    void
//返回参数    int：0=返回主菜单
//使用实例    menu_image_param();    // 调整图像显示模式与裁切
//备注信息    MODE：Gray/Track ；裁切上下左右；K4 在 Save 条目时保存；Back 返回
//------------------------------------------------------------------------------------------------
typedef enum {
    IMG_ITEM_MODE = 0,
    IMG_ITEM_CROP_TOP,
    IMG_ITEM_CROP_BOTTOM,
    IMG_ITEM_CROP_LEFT,
    IMG_ITEM_CROP_RIGHT,
    IMG_ITEM_SAVE,
    IMG_ITEM_BACK,
    IMG_ITEM_TOTAL
} img_item_t;

#define IMG_VISIBLE_LINES 7
#define IMG_LINE_HEIGHT   20
#define IMG_START_Y       40

// 裁切参数合法化（防止越界/反向交叉）
static void img_crop_sanitize(void)
{
    if(g_crop_top   < 0) g_crop_top = 0; if(g_crop_top   > VIS_ORI_H-10) g_crop_top = VIS_ORI_H-10;
    if(g_crop_bottom< 0) g_crop_bottom = 0; if(g_crop_bottom> VIS_ORI_H-10) g_crop_bottom = VIS_ORI_H-10;
    if(g_crop_left  < 0) g_crop_left = 0; if(g_crop_left  > VIS_ORI_W-10) g_crop_left = VIS_ORI_W-10;
    if(g_crop_right < 0) g_crop_right= 0; if(g_crop_right > VIS_ORI_W-10) g_crop_right= VIS_ORI_W-10;
    if(g_crop_top + g_crop_bottom > VIS_ORI_H-5){ g_crop_bottom = (VIS_ORI_H-5) - g_crop_top; if(g_crop_bottom<0) g_crop_bottom=0; }
    if(g_crop_left + g_crop_right > VIS_ORI_W-5){ g_crop_right = (VIS_ORI_W-5) - g_crop_left; if(g_crop_right<0) g_crop_right=0; }
}

// 绘制图像参数菜单
static void img_draw(int sel, int top)
{
    ips200_clear();
    ips200_show_string(0,15,"IMAGE PARAM");
    int total_pages = (IMG_ITEM_TOTAL + IMG_VISIBLE_LINES -1)/IMG_VISIBLE_LINES; int cur_page = top/IMG_VISIBLE_LINES + 1; char info[24];
    snprintf(info,sizeof(info),"Pg %d/%d",cur_page,total_pages); ips200_show_string(140,15,info);
    for(int i=0;i<IMG_VISIBLE_LINES;i++){
        int idx = top + i; if(idx >= IMG_ITEM_TOTAL) break; int y = IMG_START_Y + i*IMG_LINE_HEIGHT;
        switch(idx){
            case IMG_ITEM_MODE: ips200_show_string(0,y,"Mode      :"); ips200_show_string(100,y,(g_img_display_mode==IMG_MODE_TRACK)?"Track":"Gray"); break;
            case IMG_ITEM_CROP_TOP: ips200_show_string(0,y,"Crop Top  :"); ips200_show_int(100,y,g_crop_top,4); break;
            case IMG_ITEM_CROP_BOTTOM: ips200_show_string(0,y,"Crop Bottom:"); ips200_show_int(100,y,g_crop_bottom,4); break;
            case IMG_ITEM_CROP_LEFT: ips200_show_string(0,y,"Crop Left :"); ips200_show_int(100,y,g_crop_left,4); break;
            case IMG_ITEM_CROP_RIGHT: ips200_show_string(0,y,"Crop Right:"); ips200_show_int(100,y,g_crop_right,4); break;
            case IMG_ITEM_SAVE: ips200_show_string(0,y,"Save & Apply"); break;
            case IMG_ITEM_BACK: ips200_show_string(0,y,"Back"); break;
            default: break;
        }
        ips200_show_string(180,y,(idx==sel)?"<":" ");
    }
    ips200_show_string(0, IMG_START_Y + IMG_VISIBLE_LINES*IMG_LINE_HEIGHT + 2, "K1-/K3+ K2=Next K4=OK");
}

// 图像参数菜单长按加速
static int img_key_repeat(uint32_t *hold_dec, uint32_t *hold_inc)
{
    const uint32_t first_delay=400, repeat_gap=120, accel_after=1500, fast=60; int act=0;
    if(key_is_pressed(KEY_1)){ *hold_dec+=10; if(*hold_dec>first_delay){ uint32_t g=(*hold_dec>accel_after)?fast:repeat_gap; if(((*hold_dec-first_delay)%g)<10) act=-1;} } else *hold_dec=0;
    if(key_is_pressed(KEY_3)){ *hold_inc+=10; if(*hold_inc>first_delay){ uint32_t g=(*hold_inc>accel_after)?fast:repeat_gap; if(((*hold_inc-first_delay)%g)<10) act=(act==-1)?-1:1;} } else *hold_inc=0;
    return act;
}

int menu_image_config(void)
{
    int sel=0, top=0, last_sel=-1,last_top=-1; uint32_t hold_dec=0,hold_inc=0; img_draw(sel, top);
    while(1){
        key_scanner();
        uint8 k2=key_short(KEY_2), k4=key_short(KEY_4), k1=key_short(KEY_1), k3=key_short(KEY_3);
        if(k2){ if(++sel >= IMG_ITEM_TOTAL) sel=0; }
        if(sel < top) top = (sel/IMG_VISIBLE_LINES)*IMG_VISIBLE_LINES; else if(sel >= top+IMG_VISIBLE_LINES) top = (sel/IMG_VISIBLE_LINES)*IMG_VISIBLE_LINES;
        int rep = img_key_repeat(&hold_dec,&hold_inc); int changed=0;
        switch(sel){
            case IMG_ITEM_MODE: if(k1||k3||rep){ g_img_display_mode = (g_img_display_mode==IMG_MODE_GRAY)?IMG_MODE_TRACK:IMG_MODE_GRAY; changed=1; } break;
            case IMG_ITEM_CROP_TOP: if(k1||rep==-1){ g_crop_top--; changed=1;} if(k3||rep==1){ g_crop_top++; changed=1;} break;
            case IMG_ITEM_CROP_BOTTOM: if(k1||rep==-1){ g_crop_bottom--; changed=1;} if(k3||rep==1){ g_crop_bottom++; changed=1;} break;
            case IMG_ITEM_CROP_LEFT: if(k1||rep==-1){ g_crop_left--; changed=1;} if(k3||rep==1){ g_crop_left++; changed=1;} break;
            case IMG_ITEM_CROP_RIGHT: if(k1||rep==-1){ g_crop_right--; changed=1;} if(k3||rep==1){ g_crop_right++; changed=1;} break;
            case IMG_ITEM_SAVE: if(k4){ image_param_save(); ips200_show_string(0,220,"Saved"); system_delay_ms(250);} break;
            case IMG_ITEM_BACK: if(k4){ return 0; } break;
            default: break;
        }
        if(changed){ img_crop_sanitize(); }
        if(last_sel!=sel || last_top!=top || changed || rep || k1 || k3){ img_draw(sel, top); last_sel=sel; last_top=top; }
        system_delay_ms(10);
    }
}


// 旧接口保留
int menu_param_config(void){ return menu_pid_config(); }
void param_flash_read(void){ pid_param_flash_read(); }
void param_flash_write(void){ pid_param_flash_write(); }

