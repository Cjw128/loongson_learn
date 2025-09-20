//本文件为用户自己编写的针对龙芯开发环境，usb摄像头的图像处理程序。
#include "zf_common_headfile.h"
#include "Header.h"
#include "image.h"

#include <opencv2/imgproc/imgproc.hpp>  // for cv::cvtColor
#include <opencv2/highgui/highgui.hpp> // for cv::VideoCapture
#include <opencv2/opencv.hpp>

#include <iostream> // for std::cerr
#include <fstream>  // for std::ofstream
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <atomic>

using namespace cv;


/***************************图像定义***************************/
extern uint8_t *rgay_image;                 // 原图指针

/***************************图像定义***************************/

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
//函数名称    camera_init
//参数说明    void
//返回参数    void
//使用实例    camera_init();    // 初始化摄像头
//------------------------------------------------------------------------------------------------
// 摄像头初始化，返回0成功，非0失败
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
//使用实例    vis_otsu();    // 大津法二值化
//备注信息    加入了图像裁切功能，得到的大津法阈值、二值化效果针对的是有效图区域。
//------------------------------------------------------------------------------------------------
void vis_otsu(){
    Mat src(VIS_ORI_H, VIS_ORI_W, CV_8UC1, rgay_image);
    Rect roi(VIS_LEFT_CUT, VIS_TOP_CUT, VIS_W, VIS_H);
    Mat cropped = src(roi); // 裁剪后区域
    Mat binary;
    threshold(cropped, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);
    memcpy(rgay_image + VIS_TOP_CUT * VIS_ORI_W + VIS_LEFT_CUT, binary.data, VIS_SIZE);
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
extern linelist l_border;
extern linelist r_border;
extern linelist center_line;
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
//函数名称    vis_image_process
//参数说明    void
//返回参数    void
//使用实例    vis_image_process();    // 图像处理主函数
//备注信息    图像处理主函数
//------------------------------------------------------------------------------------------------
void vis_image_process(void)
{
    //uint16 i = 0;
    uint8 bin_image[VIS_H][VIS_W] = { 0 };//二值化图像
    vis_otsu(); //大津法二值化
    memcpy(bin_image, rgay_image, VIS_SIZE);//复制到二维数组
    vis_image_filter(bin_image);//形态学滤波
    vis_image_draw_rectan(bin_image);//给图像画一个黑框
    data_stastics_l = 0;
    data_stastics_r = 0;
    hightest = 0;
    if (vis_get_start_point(VIS_H - 2))//获取起始点
    {   
        vis_search_l_r((uint16)USE_num, bin_image, &data_stastics_l, &data_stastics_r, start_point_l[0], start_point_l[1], start_point_r[0], start_point_r[1], &hightest);//搜索左右边线点  
        //获取左边界和右边界
        vis_get_left(data_stastics_l);
        vis_get_right(data_stastics_r);
    }
    vis_draw_line();	//可选边线显示

    //ips200_show_gray_image(0,0,(const uint8 *)rgay_image, VIS_W, VIS_H);
}




