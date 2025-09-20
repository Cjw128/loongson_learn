//本文件为用户编写的类似于硬件flash读取、写入的函数，实际是读写文件
#include "zf_common_headfile.h"
#include "flash.h"
#include "Header.h"

//------------------------------------------------------------------------------------------------
//函数名称    flash_write_u32
//参数说明    const char* filename：文件路径
//           uint32_t value：要写入的整数值
//返回参数    int：0成功，非0失败
//使用实例    flash_write_u32("/home/cjw/data/param.txt", value);    // 写入整数到文件
//------------------------------------------------------------------------------------------------
int flash_write_u32(const char* filename, uint32_t value)
{
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    fprintf(fp, "%u", value);
    fclose(fp);
    return 0;
}

//------------------------------------------------------------------------------------------------
//函数名称    flash_read_u32
//参数说明    const char* filename：文件路径
//           uint32_t* value：读取的整数值指针
//返回参数    int：0成功，非0失败
//使用实例    flash_read_u32("/mnt/sdcard/config.txt", &value);    // 读取整数从文件
//------------------------------------------------------------------------------------------------
int flash_read_u32(const char* filename, uint32_t* value)
{
    char buffer[32] = {0};
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    if (fgets(buffer, sizeof(buffer), fp)) {
        *value = (uint32_t)atoi(buffer);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return -2;
}

//------------------------------------------------------------------------------------------------
//函数名称    flash_write_float
//参数说明    const char* filename：文件路径
//           float value：要写入的浮点数值
//返回参数    int：0成功，非0失败
//使用实例    flash_write_float("/home/cjw/data/param.txt", value);    // 写入浮点数到文件
//------------------------------------------------------------------------------------------------
int flash_write_float(const char* filename, float value)
{
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    fprintf(fp, "%f", value);
    fclose(fp);
    return 0;
}

//------------------------------------------------------------------------------------------------
//函数名称    flash_read_float
//参数说明    const char* filename：文件路径
//           float* value：读取的浮点数值指针
//返回参数    int：0成功，非0失败
//使用实例    flash_read_float("/mnt/sdcard/config.txt", &value);    // 读取浮点数从文件
//------------------------------------------------------------------------------------------------
int flash_read_float(const char* filename, float* value)
{
    char buffer[32] = {0};
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    if (fgets(buffer, sizeof(buffer), fp)) {
        *value = atof(buffer);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return -2;
}
