//���ļ�Ϊ�û���д��������Ӳ��flash��ȡ��д��ĺ�����ʵ���Ƕ�д�ļ�
#include "zf_common_headfile.h"
#include "flash.h"
#include "Header.h"

//------------------------------------------------------------------------------------------------
//��������    flash_write_u32
//����˵��    const char* filename���ļ�·��
//           uint32_t value��Ҫд�������ֵ
//���ز���    int��0�ɹ�����0ʧ��
//ʹ��ʵ��    flash_write_u32("/home/cjw/data/param.txt", value);    // д���������ļ�
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
//��������    flash_read_u32
//����˵��    const char* filename���ļ�·��
//           uint32_t* value����ȡ������ֵָ��
//���ز���    int��0�ɹ�����0ʧ��
//ʹ��ʵ��    flash_read_u32("/mnt/sdcard/config.txt", &value);    // ��ȡ�������ļ�
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
//��������    flash_write_float
//����˵��    const char* filename���ļ�·��
//           float value��Ҫд��ĸ�����ֵ
//���ز���    int��0�ɹ�����0ʧ��
//ʹ��ʵ��    flash_write_float("/home/cjw/data/param.txt", value);    // д�븡�������ļ�
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
//��������    flash_read_float
//����˵��    const char* filename���ļ�·��
//           float* value����ȡ�ĸ�����ֵָ��
//���ز���    int��0�ɹ�����0ʧ��
//ʹ��ʵ��    flash_read_float("/mnt/sdcard/config.txt", &value);    // ��ȡ���������ļ�
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
