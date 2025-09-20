#include <opencv2/opencv.hpp>
#include "zf_common_headfile.h"
#include "Header.h"

using namespace cv;

extern Mat frame_rgay;      // 灰度图像（由摄像头采集）
extern VideoCapture cap;    // 摄像头对象

// 定义红色的颜色值（根据ips200的颜色格式调整）
#define RED 0xF800

int main()
{
   
        ips200_init("/dev/fb0");
        

        while (true) {
            // 采集一帧灰度图像
            if (wait_image_refresh() != 0) {
                printf("采集图像失败！\n");
                continue;
            }
            if (frame_rgay.empty()) {
                printf("frame_rgay is empty!\n");
                continue;
            }

            vis_image_process(); // 图像处理，得到边线数据
        }

        cap.release();
        return 0;
}
