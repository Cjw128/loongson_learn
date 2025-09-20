//本文件为用户自己编写的中断服务函数
//用户要进行中断，直接在==start_isr==函数中传入自己的中断服务函数即可
#include "zf_common_headfile.h"
#include "isr.h"
#include "Header.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <functional>
/********************************************用户代码*********************************************/

//------------------------------------------------------------------------------------------------
//函数名称    timer_callback_thread
//参数说明    arg            回调函数指针
//          time           中断周期，单位微秒
//返回参数    void*          线程退出状态
//使用实例    pthread_t tid;
//          pthread_create(&tid, NULL, timer_callback_thread, (void*)user_function, 10000);    // 创建一个周期为10ms的线程
//------------------------------------------------------------------------------------------------
void* timer_callback_thread(void* arg)
{
    void (*callback)(void) = (void (*)(void))arg;
    while (1)
    {
        usleep(1000000); // 用户自行设置中断时长
        callback();
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
//函数名称    start_isr
//参数说明    isr_func        中断服务函数指针
//返回参数    void
//使用实例    void user_isr(void) { // 用户中断服务函数 }
//          start_isr(user_isr);    // 启动中断服务
//------------------------------------------------------------------------------------------------
void start_isr(void (*isr_func)(void))
{
    pthread_t tid;
    pthread_create(&tid, NULL, timer_callback_thread, (void*)isr_func);
}

//以下是一个调用示例
/*
void my_isr(void){
    printf("This is my custom ISR function.\n");
}

int main() 
{   
    // 启动中断服务，传入用户自定义的中断服务函数
    start_isr(my_isr);
    while(1)
    {   
        // 主循环代码
        usleep(100000); // 每次循环休眠100ms，降低CPU占用
    }
}
*/