//本文件为用户自己编写的针对龙芯开发环境，usb摄像头的图像处理程序。
#include "zf_common_headfile.h"
#include "Header.h"
#include "image.h"

// 已移除对 OpenCV 的直接依赖（保留在底层摄像头驱动文件中）。

#include <iostream> // for std::cerr
#include <fstream>  // for std::ofstream
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>


/***************************图像定义***************************/
extern uint8_t *rgay_image;                 // 原图指针
// 运行期图像参数变量定义
volatile int g_img_display_mode = IMG_MODE_GRAY;
volatile int g_crop_top = 0;
volatile int g_crop_bottom = 0;
volatile int g_crop_left = 0;
volatile int g_crop_right = 0;

#ifndef IMAGE_PARAM_STORE_PATH
#define IMAGE_PARAM_STORE_PATH "/home/root/image_cfg.txt"
#endif
/***************************图像定义***************************/

//------------------------------------------------------------------------------------------------
//函数名称    image_param_load
//参数说明    void
//返回参数    void
//使用实例    image_param_load();    // 加载图像显示模式与裁切参数
//备注信息    若配置文件不存在或格式错误则使用默认值(灰度模式+零裁切)
//------------------------------------------------------------------------------------------------
void image_param_load(void)
{
	FILE *fp = fopen(IMAGE_PARAM_STORE_PATH, "r");
	if(!fp){
		printf("[IMG] no image param file, using defaults\n");
		return;
	}
	int mode, t,b,l,r;
	if(5==fscanf(fp, "%d %d %d %d %d", &mode,&t,&b,&l,&r)){
		g_img_display_mode = (mode==IMG_MODE_TRACK)?IMG_MODE_TRACK:IMG_MODE_GRAY;
		g_crop_top=t; g_crop_bottom=b; g_crop_left=l; g_crop_right=r;
	}else{
		printf("[IMG] image param file format error\n");
	}
	fclose(fp);
}

//------------------------------------------------------------------------------------------------
//函数名称    image_param_save
//参数说明    void
//返回参数    void
//使用实例    image_param_save();    // 保存当前图像显示模式与裁切参数
//备注信息    写入 IMAGE_PARAM_STORE_PATH，失败会打印错误
//------------------------------------------------------------------------------------------------
void image_param_save(void)
{
		FILE *fp = fopen(IMAGE_PARAM_STORE_PATH, "w");
		if(!fp){ printf("[IMG] save failed %s\n", IMAGE_PARAM_STORE_PATH); return; }
		fprintf(fp,
			"IMG\n"
			"Mode %d\n"
			"Crop_Top %d\n"
			"Crop_Bottom %d\n"
			"Crop_Left %d\n"
			"Crop_Right %d\n",
			g_img_display_mode,
			g_crop_top,
			g_crop_bottom,
			g_crop_left,
			g_crop_right
		);
		fclose(fp);
		printf("[IMG] saved to %s\n", IMAGE_PARAM_STORE_PATH);
}

//------------------------------------------------------------------------------------------------
//函数名称    image_apply_crops
//参数说明    int *x：返回 ROI 起始 x
//           int *y：返回 ROI 起始 y
//           int *w：返回 ROI 宽度
//           int *h：返回 ROI 高度
//返回参数    void
//使用实例    image_apply_crops(&rx,&ry,&rw,&rh);    // 依据当前裁切变量计算合法 ROI
//备注信息    自动裁剪并防止越界；若裁切过度会自动压缩使宽高保持最小安全范围
//------------------------------------------------------------------------------------------------
// 根据运行期裁切变量计算 ROI（确保合法）
void image_apply_crops(int *x,int *y,int *w,int *h)
{
	int top = g_crop_top; if(top < 0) top = 0; if(top > VIS_ORI_H-10) top = VIS_ORI_H-10;
	int bottom = g_crop_bottom; if(bottom < 0) bottom = 0; if(bottom > VIS_ORI_H-10) bottom = VIS_ORI_H-10;
	int left = g_crop_left; if(left < 0) left = 0; if(left > VIS_ORI_W-10) left = VIS_ORI_W-10;
	int right = g_crop_right; if(right < 0) right = 0; if(right > VIS_ORI_W-10) right = VIS_ORI_W-10;
	if(top + bottom > VIS_ORI_H-5){ bottom = (VIS_ORI_H-5) - top; if(bottom<0) bottom=0; }
	if(left + right > VIS_ORI_W-5){ right = (VIS_ORI_W-5) - left; if(right<0) right=0; }
	*x = left; *y = top; *w = VIS_ORI_W - left - right; *h = VIS_ORI_H - top - bottom;
}



//------------------------------------------------------------------------------------------------
//函数名称    my_abs
//参数说明    int value：输入值
//返回参数    int：绝对值
//使用实例    int abs_value = my_abs(value);    // 计算value的绝对值
//------------------------------------------------------------------------------------------------
int my_abs(int value){
    if (value>=0) return value;
    else return -value;
}

//------------------------------------------------------------------------------------------------
//函数名称    limit_a_b
//参数说明    int x：输入值
//           int a：下限
//           int b：上限
//返回参数    int16：限制在a到b之间的值
//使用实例    int16 limited_value = limit_a_b(x, a, b);    // 将x限制在a和b之间
//------------------------------------------------------------------------------------------------
int16 limit_a_b(int16 x,int a,int b){
    if (x<a) x = a;
    if (x>b) x = b;
    return x;
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_camera_init
//参数说明    const char* device：V4L2 设备节点 (如 /dev/video0)
//返回参数    int：0-成功  非0-失败
//使用实例    if(vis_camera_init("/dev/video0")){ /* 失败处理 */ }
//备注信息    封装底层 uvc_camera_init，失败打印错误信息
//------------------------------------------------------------------------------------------------
int vis_camera_init(const char* device)
{
	int ret = uvc_camera_init(device);
	if (ret != 0) {
		printf("摄像头初始化失败: %s\n", device);
	}
	return ret;
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_otsu
//参数说明    void
//返回参数    void
//使用实例    vis_otsu();    // 大津法二值化（纯C实现）
//备注信息    基于运行期裁切参数计算 ROI，在该区域内执行 Otsu 阈值并就地写回。
//------------------------------------------------------------------------------------------------
void vis_otsu(){
	if(!rgay_image) return;
	int rx, ry, rw, rh; image_apply_crops(&rx,&ry,&rw,&rh);
	if(rw <= 0 || rh <= 0) return;
	unsigned long hist[256];
	memset(hist,0,sizeof(hist));
	// 统计直方图
	for(int j=0;j<rh;j++){
		const uint8_t *row = rgay_image + (ry + j) * VIS_ORI_W + rx;
		for(int i=0;i<rw;i++) hist[row[i]]++;
	}
	unsigned long total = (unsigned long)rw * rh;
	if(total == 0) return;
	unsigned long sum_all = 0; for(int i=0;i<256;i++) sum_all += i * hist[i];
	unsigned long wB = 0; unsigned long sumB = 0; double max_between = -1.0; int best_th = 0;
	for(int t=0;t<256;t++){
		wB += hist[t];
		if(wB == 0) continue;
		unsigned long wF = total - wB; if(wF == 0) break;
		sumB += t * hist[t];
		double mB = (double)sumB / wB;
		double mF = (double)(sum_all - sumB) / wF;
		double diff = mB - mF; double between = (double)wB * (double)wF * diff * diff;
		if(between > max_between){ max_between = between; best_th = t; }
	}
	// 应用阈值
	for(int j=0;j<rh;j++){
		uint8_t *row = rgay_image + (ry + j) * VIS_ORI_W + rx;
		for(int i=0;i<rw;i++) row[i] = (row[i] > best_th) ? 255 : 0;
	}
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_get_start_point
//参数说明    uint8 start_row：起始行
//返回参数    uint8：1-找到起始点 0-未找到起始点
//使用实例    uint8 found = get_start_point(start_row);    // 获取起始点
//备注信息    起始点为图像中间向左右扫描，找到的第一个黑色点
//------------------------------------------------------------------------------------------------
uint8 start_point_l[2] = {0};
uint8 start_point_r[2] = {0};
uint8 vis_get_start_point(uint8 start_row)
{
	uint8 i = 0,l_found = 0,r_found = 0;
	//清零
	start_point_l[0] = 0;//x
	start_point_l[1] = 0;//y

	start_point_r[0] = 0;//x
	start_point_r[1] = 0;//y

	for (i = VIS_W / 2; i > VIS_BORDER_MIN; i--)
	{
		start_point_l[0] = i;//x
		start_point_l[1] = start_row;//y
		if (rgay_image[start_row * VIS_ORI_W + i] == 255 && rgay_image[start_row * VIS_ORI_W + i - 1] == 0)
		{
			l_found = 1;
			break;
		}
	}

	for (i = VIS_W / 2; i < VIS_BORDER_MAX ;i++)
	{
		start_point_r[0] = i;//x
		start_point_r[1] = start_row;//y
		if (rgay_image[start_row * VIS_ORI_W + i] == 255 && rgay_image[start_row * VIS_ORI_W + i + 1] == 0)
		{
			r_found = 1;
			break;
		}
	}

	if(l_found&&r_found)return 1;
	else {
		return 0;
	} 
}


//------------------------------------------------------------------------------------------------
//函数名称    vis_search_l_r
//参数说明    uint16 break_flag：最大循环次数
//           uint8(*image)[image_w]：二值化图像数组
//           uint16 *l_stastic：左边点统计数量指针  
//           uint16 *r_stastic：右边点统计数量指针
//           uint8 l_start_x：左边起始点x坐标
//           uint8 l_start_y：左边起始点y坐标
//           uint8 r_start_x：右边起始点x坐标
//           uint8 r_start_y：右边起始点y坐标
//           uint8 *hightest：返回最高点
//返回参数    void
//使用实例    search_l_r(break_flag, image, &l_stastic, &r_stastic, l_start_x, l_start_y, r_start_x, r_start_y, &hightest);    // 搜索左右边线点
//备注信息    该函数使用8邻域法搜索边线点，
//           break_flag：最大循环次数，防止死循环
//           image：二值化图像数组
//           *l_stastic：左边点统计数量指针
//           *r_stastic：右边点统计数量指针
//           l_start_x：左边起始点x坐标
//           l_start_y：左边起始点y坐标
//           r_start_x：右边起始点x坐标
//           r_start_y：右边起始点y坐标
//           hightest：返回最高点
//------------------------------------------------------------------------------------------------
#define USE_num	VIS_H*3	//定义找点的数组成员个数按理说300个点能放下，但是有些特殊情况确实难顶，多定义了一点

 //存放点的x，y坐标
uint16 points_l[(uint16)USE_num][2] = { {  0 } };//左线
uint16 points_r[(uint16)USE_num][2] = { {  0 } };//右线
uint16 dir_r[(uint16)USE_num] = { 0 };//用来存储右边生长方向
uint16 dir_l[(uint16)USE_num] = { 0 };//用来存储左边生长方向
uint16 data_stastics_l = 0;
uint16 data_stastics_r = 0;
uint8 hightest = 0; 
uint8_t l_dir_sum[9]={0};
uint8_t r_dir_sum[9]={0};
void vis_search_l_r(uint16 break_flag, uint8(*image)[VIS_W], uint16 *l_stastic, uint16 *r_stastic, uint8 l_start_x, uint8 l_start_y, uint8 r_start_x, uint8 r_start_y, uint8*hightest)
{

	uint8 i = 0, j = 0;

	//左边变量
	uint8 search_filds_l[8][2] = { {  0 } };
	uint8 index_l = 0;
	uint8 temp_l[8][2] = { {  0 } };
	uint8 center_point_l[2] = {  0 };
	uint16 l_data_statics;
	static int8 seeds_l[8][2] = { {0,  1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,  0},{1, 1}, };
	uint8 search_filds_r[8][2] = { {  0 } };
	uint8 center_point_r[2] = { 0 };
	uint8 index_r = 0;
	uint8 temp_r[8][2] = { {  0 } };
	uint16 r_data_statics;
	static int8 seeds_r[8][2] = { {0,  1},{1,1},{1,0}, {1,-1},{0,-1},{-1,-1}, {-1,  0},{-1, 1}, };

	l_data_statics = *l_stastic;
	r_data_statics = *r_stastic;

	center_point_l[0] = l_start_x;
	center_point_l[1] = l_start_y;
	center_point_r[0] = r_start_x;
	center_point_r[1] = r_start_y;

	while (break_flag--)
	{

		for (i = 0; i < 8; i++)//传递8F坐标
		{
			search_filds_l[i][0] = center_point_l[0] + seeds_l[i][0];//x
			search_filds_l[i][1] = center_point_l[1] + seeds_l[i][1];//y
		}
		//中心坐标点填充到已经找到的点内
		points_l[l_data_statics][0] = center_point_l[0];//x
		points_l[l_data_statics][1] = center_point_l[1];//y
		l_data_statics++;//索引加一

		//右边
		for (i = 0; i < 8; i++)//传递8F坐标
		{
			search_filds_r[i][0] = center_point_r[0] + seeds_r[i][0];//x
			search_filds_r[i][1] = center_point_r[1] + seeds_r[i][1];//y
		}
		//中心坐标点填充到已经找到的点内
		points_r[r_data_statics][0] = center_point_r[0];//x
		points_r[r_data_statics][1] = center_point_r[1];//y

		index_l = 0;//先清零，后使用
		for (i = 0; i < 8; i++)
		{
			temp_l[i][0] = 0;//先清零，后使用
			temp_l[i][1] = 0;//先清零，后使用
		}

		//左边判断
		for (i = 0; i < 8; i++)
		{
			if (image[search_filds_l[i][1]][search_filds_l[i][0]] == 0
				&& image[search_filds_l[(i + 1) & 7][1]][search_filds_l[(i + 1) & 7][0]] == 255)
			{
				temp_l[index_l][0] = search_filds_l[(i)][0];
				temp_l[index_l][1] = search_filds_l[(i)][1];
				index_l++;
				if(center_point_l[1] < 5)
                        dir_l[l_data_statics-1] = -1;
                    else {
                        dir_l[l_data_statics - 1]= (i);/////////////////////
                        l_dir_sum[i]++;
                    }
			}

			if (index_l)
			{
				//更新坐标点
				center_point_l[0] = temp_l[0][0];//x
				center_point_l[1] = temp_l[0][1];//y
				for (j = 0; j < index_l; j++)
				{
					if (center_point_l[1] > temp_l[j][1])
					{
						center_point_l[0] = temp_l[j][0];//x
						center_point_l[1] = temp_l[j][1];//y
					}
				}
			}

		}
		if ((points_r[r_data_statics][0]== points_r[r_data_statics-1][0]&& points_r[r_data_statics][0] == points_r[r_data_statics - 2][0]
			&& points_r[r_data_statics][1] == points_r[r_data_statics - 1][1] && points_r[r_data_statics][1] == points_r[r_data_statics - 2][1])
			||(points_l[l_data_statics-1][0] == points_l[l_data_statics - 2][0] && points_l[l_data_statics-1][0] == points_l[l_data_statics - 3][0]
				&& points_l[l_data_statics-1][1] == points_l[l_data_statics - 2][1] && points_l[l_data_statics-1][1] == points_l[l_data_statics - 3][1]))
		{
			break;
		}
		if (my_abs(points_r[r_data_statics][0] - points_l[l_data_statics - 1][0]) < 2
			&& my_abs(points_r[r_data_statics][1] - points_l[l_data_statics - 1][1] < 2)
			)
		{
			*hightest = (points_r[r_data_statics][1] + points_l[l_data_statics - 1][1]) >> 1;
			break;
		}
		if ((points_r[r_data_statics][1] < points_l[l_data_statics - 1][1]))
		{
			continue;
		}
		if (dir_l[l_data_statics - 1] == 7
			&& (points_r[r_data_statics][1] > points_l[l_data_statics - 1][1]))
		{
			center_point_l[0] = points_l[l_data_statics - 1][0];//x
			center_point_l[1] = points_l[l_data_statics - 1][1];//y
			l_data_statics--;
		}
		r_data_statics++;

		index_r = 0;
		for (i = 0; i < 8; i++)
		{
			temp_r[i][0] = 0;
			temp_r[i][1] = 0;
		}

		//右边判断
		for (i = 0; i < 8; i++)
		{
			if (image[search_filds_r[i][1]][search_filds_r[i][0]] == 0
				&& image[search_filds_r[(i + 1) & 7][1]][search_filds_r[(i + 1) & 7][0]] == 255)
			{
				temp_r[index_r][0] = search_filds_r[(i)][0];
				temp_r[index_r][1] = search_filds_r[(i)][1];
				index_r++;
				if(center_point_r[1] < 5)//////////////////////////
                            dir_r[r_data_statics - 1] = -1;//记录生长方向
                        else {
                            dir_r[r_data_statics - 1] = (i);//记录生长方向
                        r_dir_sum[i]++;
                        }
			}
			if (index_r)
			{

				//更新坐标点
				center_point_r[0] = temp_r[0][0];//x
				center_point_r[1] = temp_r[0][1];//y
				for (j = 0; j < index_r; j++)
				{
					if (center_point_r[1] > temp_r[j][1])
					{
						center_point_r[0] = temp_r[j][0];//x
						center_point_r[1] = temp_r[j][1];//y
					}
				}

			}
		}


	}


	//取出循环次数
	*l_stastic = l_data_statics;
	*r_stastic = r_data_statics;

}


//------------------------------------------------------------------------------------------------
//函数名称    vis_get_left
//参数说明    uint16 total_L：左边点统计数量
//返回参数    void
//使用实例    vis_get_left(total_L);    // 获取左边线数组
//备注信息    根据搜索到的左边点，得到左边线数组	
//------------------------------------------------------------------------------------------------
 linelist l_border;
 linelist r_border;
 linelist center_line;
void vis_get_left(uint16 total_L)
{
	uint8 i = 0;
	uint16 j = 0;
	uint8 h = 0;
	//初始化
	for (i = 0;i<VIS_H;i++)
	{
		l_border[i] = VIS_BORDER_MIN;
	}
	h = VIS_H - 2;
	//左边
	for (j = 0; j < total_L; j++)
	{
		//printf("%d\n", j);
		if (points_l[j][1] == h)
		{
			l_border[h] = points_l[j][0]+1;
		}
		else continue; //每行只取一个点，没到下一行就不记录
		h--;
		if (h == 0) 
		{
			break;//到最后一行退出
		}
	}
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_get_right
//参数说明    uint16 total_R：右边点统计数量
//返回参数    void
//使用实例    vis_get_right(total_R);    // 获取右边线数组
//备注信息    根据搜索到的右边点，未命中行保持默认最右值以维持中心线稳定
//------------------------------------------------------------------------------------------------
void vis_get_right(uint16 total_R)
{
	uint8 i = 0;
	uint16 j = 0;
	uint8 h = 0;
	for (i = 0; i < VIS_H; i++)
	{
		r_border[i] = VIS_BORDER_MAX;//右边线初始化放到最右边，左边线放到最左边，这样八邻域闭合区域外的中线就会在中间，不会干扰得到的数据
	}
	h = VIS_H - 2;
	//右边
	for (j = 0; j < total_R; j++)
	{
		if (points_r[j][1] == h)
		{
			r_border[h] = points_r[j][0] - 1;
		}
		else continue;//每行只取一个点，没到下一行就不记录
		h--;
		if (h == 0)break;//到最后一行退出
	}
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_image_filter
//参数说明    uint8(*bin_image)[image_w]：二值化图像数组
//返回参数    void
//使用实例    vis_image_filter(bin_image);    // 形态学滤波
//备注信息    形态学滤波，简单来说就是膨胀和腐蚀的思想
//------------------------------------------------------------------------------------------------
//定义膨胀和腐蚀的阈值区间
#define threshold_max	255*5//此参数可根据自己的需求调节
#define threshold_min	255*2//此参数可根据自己的需求调节
void vis_image_filter(uint8(*bin_image)[VIS_W])//形态学滤波，简单来说就是膨胀和腐蚀的思想
{
	uint16 i, j;
	uint32 num = 0;


	for (i = 1; i < VIS_H - 1; i++)
	{
		for (j = 1; j < (VIS_W - 1); j++)
		{
			//统计八个方向的像素值
			num =
				bin_image[i - 1][j - 1] + bin_image[i - 1][j] + bin_image[i - 1][j + 1]
				+ bin_image[i][j - 1] + bin_image[i][j + 1]
				+ bin_image[i + 1][j - 1] + bin_image[i + 1][j] + bin_image[i + 1][j + 1];


			if (num >= threshold_max && bin_image[i][j] == 0)
			{

				bin_image[i][j] = 255;//白  可以搞成宏定义，方便更改

			}
			if (num <= threshold_min && bin_image[i][j] == 255)
			{

				bin_image[i][j] = 0;//黑

			}

		}
	}

}

//------------------------------------------------------------------------------------------------
//函数名称    vis_image_draw_rectan
//参数说明    uint8(*image)[image_w]：二值化图像数组
//返回参数    void
//使用实例    vis_image_draw_rectan(image);    // 给图像画一个黑框
//备注信息    给图像画一个黑框，防止边线点搜索时，搜索到图像边缘的噪声点
//------------------------------------------------------------------------------------------------
void vis_image_draw_rectan(uint8(*image)[VIS_W])
{

	uint8 i = 0;
	for (i = 0; i < VIS_H; i++)
	{
		image[i][0] = 0;
		image[i][1] = 0;
		image[i][VIS_W - 1] = 0;
		image[i][VIS_W - 2] = 0;

	}
	for (i = 0; i < VIS_W; i++)
	{
		image[0][i] = 0;
		image[1][i] = 0;
		//image[image_h-1][i] = 0;

	}
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_cal_centerline
//参数说明    void
//返回参数    void
//使用实例    vis_cal_centerline();    // 计算中线
//备注信息    计算中线
//------------------------------------------------------------------------------------------------
void vis_cal_centerline(void){
	uint8 i = 0;
	for (i = 0; i < VIS_H; i++)
	{
		center_line[i] = (l_border[i] + r_border[i]) >> 1;
	}
}	

//------------------------------------------------------------------------------------------------
//函数名称    vis_draw_line
//参数说明    void
//返回参数    void
//使用实例    vis_draw_line();    // 画出边线和中线
//备注信息    画出边线和中线
//------------------------------------------------------------------------------------------------
void vis_draw_line() {
	for (int i = hightest; i < VIS_H - 1; i++)
    {
        int cl = center_line[i];
    int lb = l_border[i];
    int rb = r_border[i];
    // 限制坐标范围
    cl = (cl < 0) ? 0 : (cl >= VIS_W ? VIS_W - 1 : cl);
    lb = (lb < 0) ? 0 : (lb >= VIS_W ? VIS_W - 1 : lb);
    rb = (rb < 0) ? 0 : (rb >= VIS_W ? VIS_W - 1 : rb);
        ips200_draw_point(cl, i, RGB565_BLUE);
        ips200_draw_point(lb, i, RGB565_GREEN);
        ips200_draw_point(rb, i, RGB565_GREEN);
    }
	system_delay_ms(20);
	ips200_clear();
}

//------------------------------------------------------------------------------------------------
//函数名称    vis_cal_err
//参数说明    void
//返回参数    float：偏差值
//使用实例    float err = vis_cal_err();    // 计算偏差值
//备注信息    计算偏差值
//------------------------------------------------------------------------------------------------
float err = 0.0;
void vis_cal_err(void){

	// 取20到50行，每隔5行取一个点，计算与图像中心的偏差均值
	int sum_err = 0;
	int count = 0;
	int center_x = VIS_W / 2;
	int i;
	for (i = 20; i <= 50; i += 5) {
		int cl = center_line[i];
		sum_err += (cl - center_x);
		count++;
	}
	err = (float)sum_err / count;
	// printf("track err = %f\n", err);
}


//------------------------------------------------------------------------------------------------
//函数名称    vis_image_process
//参数说明    void
//返回参数    void
//使用实例    vis_image_process();   
//备注信息    图像处理主函数
//------------------------------------------------------------------------------------------------
void vis_image_process(void)
{
	if(!rgay_image) return;
	// 显示模式: 灰度直显 (尊重裁切) 或 赛道算法
	if(g_img_display_mode == IMG_MODE_GRAY){
		int rx,ry,rw,rh; image_apply_crops(&rx,&ry,&rw,&rh);
		if(rw<=0||rh<=0) return;
		static uint8_t roi_buf[VIS_ORI_W*VIS_ORI_H]; // 临时缓冲 (可优化为动态或静态最小尺寸)
		for(int j=0;j<rh;j++) memcpy(roi_buf + j*rw, rgay_image + (ry+j)*VIS_ORI_W + rx, rw);
		ips200_show_gray_image(0,0, roi_buf, rw, rh);
		return;
	}

	// 赛道模式: 需无裁切保证算法固定尺寸
	if(g_crop_top||g_crop_bottom||g_crop_left||g_crop_right){
		ips200_show_string(0,0,"CLR CROPS FOR TRACK");
		return;
	}
	uint8 bin_image[VIS_H][VIS_W] = { 0 };
	vis_otsu();
	memcpy(bin_image, rgay_image, VIS_SIZE);
	vis_image_filter(bin_image);
	vis_image_draw_rectan(bin_image);
	data_stastics_l = 0; data_stastics_r = 0; hightest = 0;
	if (vis_get_start_point(VIS_H - 2)){
		vis_search_l_r((uint16)USE_num, bin_image, &data_stastics_l, &data_stastics_r, start_point_l[0], start_point_l[1], start_point_r[0], start_point_r[1], &hightest);
		vis_get_left(data_stastics_l); vis_get_right(data_stastics_r);
	}
	vis_cal_centerline();
	vis_cal_err();
	ips200_show_gray_image(0,0,(const uint8 *)rgay_image, VIS_W, VIS_H);
}



