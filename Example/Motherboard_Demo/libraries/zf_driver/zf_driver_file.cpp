#include "zf_driver_file.h"

int8 file_write_uint8(const char *path, uint8 value)
{
    int fd;

    if (0 > (fd = open(path, O_WRONLY)))
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if (write(fd, &value, sizeof(value)) < 0) 
    {
        printf("%s write error\r\n", path);
        close(fd);
        return -1;
    }
    
    close(fd); //关闭文件

    return 0;
}


int8 file_write_uint16(const char *path, uint16 value)
{
    int fd;

    if (0 > (fd = open(path, O_WRONLY)))
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if (write(fd, &value, sizeof(value)) < 0) 
    {
        printf("%s write error\r\n", path);
        close(fd);
        return -1;
    }
    
    close(fd); //关闭文件

    return 0;
}

int8 file_write_uint32(const char *path, uint32 value)
{
    int fd;

    if (0 > (fd = open(path, O_WRONLY)))
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if (write(fd, &value, sizeof(value)) < 0) 
    {
        printf("%s write error\r\n", path);
        close(fd);
        return -1;
    }
    
    close(fd); //关闭文件

    return 0;
}




int8 file_read_int8(const char *path, int8 *ret_value)
{
    int fd; 

    int8 value;

    fd = open(path, O_RDWR);
    if (0 > fd)
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if(read(fd, &value, sizeof(value)) < 0)
    {
        printf("value error\r\n");
        return -1;
    }

    *ret_value = value;

    close(fd); //关闭文件

    return 0;
}


int8 file_read_uint8(const char *path, uint8 *ret_value)
{
    int fd; 

    uint8 value;
    fd = open(path, O_RDWR);
    if (0 > fd)
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if(read(fd, &value, sizeof(value)) < 0)
    {
        printf("value error\r\n");
        return -1;
    }

    *ret_value = value;

    close(fd); //关闭文件

    return 0;
}

int8 file_read_int16(const char *path, int16 *ret_value)
{
    int fd; 

    int16 value;

    fd = open(path, O_RDWR);
    if (0 > fd)
    {
        printf("%s open error\r\n", path);
        return -1;
    }

    if(read(fd, &value, sizeof(value)) < 0)
    {
        printf("value error\r\n");
        return -1;
    }

    *ret_value = value;

    close(fd); //关闭文件

    return 0;
}


int8 file_read_string(const char *path, char *str)
{
    int ret = 0;
	FILE *fp;

    fp = fopen(path, "r"); /* 只读打开 */
    if(fp == NULL) 
    {
		printf("can not open file %s\r\n", path);
		return -1;
	}

	ret = fscanf(fp, "%s", str);
    if(!ret)
    {
        printf("file read error!\r\n");
    } 
    else if(ret == EOF) 
    {
        fseek(fp, 0, SEEK_SET);  
    }

	fclose(fp);		
	return 0;
}
