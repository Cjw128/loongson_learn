#include "zf_device_uvc.h"


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

cv::Mat frame_rgb;      // 构建opencv对象 彩色
cv::Mat frame_rgay;     // 构建opencv对象 灰度

uint8_t *rgay_image;    // 灰度图像数组指针

VideoCapture cap;

int8 uvc_camera_init(const char *path)
{
    cap.open(path);

    if(!cap.isOpened())
    {
        printf("find uvc camera error.\r\n");
        return -1;
    } 
    else 
    {
        printf("find uvc camera Successfully.\r\n");
    }


    cap.set(CAP_PROP_FRAME_WIDTH, UVC_WIDTH);     // 设置摄像头宽度
    cap.set(CAP_PROP_FRAME_HEIGHT, UVC_HEIGHT);    // 设置摄像头高度
    cap.set(CAP_PROP_FPS, UVC_FPS);              // 显示屏幕帧率

    return 0;
}


void wait_image_refresh()
{
    // 阻塞式等待图像刷新
    cap >> frame_rgb;
    // cap.read(frame_rgb);

    // rgb转灰度
    cv::cvtColor(frame_rgb, frame_rgay, cv::COLOR_BGR2GRAY);

    // cv对象转指针
    rgay_image = reinterpret_cast<uint8_t *>(frame_rgay.ptr(0));
}



