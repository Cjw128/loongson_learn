#include <linux/spi/spi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/unaligned/be_byteshift.h>

#include "zf_device_imu963ra.h"

#define IMU963RA_NAME	"zf_device_imu963ra"
float imu963ra_transition_factor[2] = {0};

struct imu963ra_dev_struct 
{

	struct spi_device *spi;				// spi设备
	struct regmap *regmap;				// 寄存器访问接口
	struct regmap_config reg_cfg;		// 寄存器访问接口
    struct mutex lock;
};

//-------------------------------------------------------------------------------------------------------------------
// 枚举简介     IMU963RA扫描元素枚举
// 成员说明     INV_IMU_CHAN_ACC_X    加速度计X轴扫描元素
// 成员说明     INV_IMU_CHAN_ACC_Y    加速度计Y轴扫描元素
// 成员说明     INV_IMU_CHAN_ACC_Z    加速度计Z轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_X   陀螺仪X轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_Y   陀螺仪Y轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_Z   陀螺仪Z轴扫描元素
// 成员说明     INV_IMU_CHAN_MAG_X    磁力计Z轴扫描元素
// 成员说明     INV_IMU_CHAN_MAG_Y    磁力计Z轴扫描元素
// 成员说明     INV_IMU_CHAN_MAG_Z    磁力计Z轴扫描元素
// 使用示例     enum inv_icm20608_scan my_scan_element = INV_IMU_CHAN_ACCL_X;
// 备注信息     用于标识IMU963RA设备不同轴的扫描元素
//-------------------------------------------------------------------------------------------------------------------
enum inv_imu_chan
{
    INV_IMU_CHAN_ACC_X,
    INV_IMU_CHAN_ACC_Y,
    INV_IMU_CHAN_ACC_Z,
    INV_IMU_CHAN_GYRO_X,
    INV_IMU_CHAN_GYRO_Y,
    INV_IMU_CHAN_GYRO_Z,
    INV_IMU_CHAN_MAG_X,
    INV_IMU_CHAN_MAG_Y,
    INV_IMU_CHAN_MAG_Z,

};

//-------------------------------------------------------------------------------------------------------------------
// 宏简介     IMU963RA通道配置宏
// 参数说明     _type       通道类型，如IIO_ACCEL（加速度计）、IIO_ANGL_VEL（陀螺仪）等
// 参数说明     _channel2   通道修饰符，如IIO_MOD_X、IIO_MOD_Y、IIO_MOD_Z等
// 参数说明     _index      扫描索引，用于确定通道在扫描序列中的位置
// 使用示例     IMU963RA_CHAN(IIO_ACCEL, IIO_MOD_X, INV_IMU_CHAN_ACCL_X);
// 备注信息     用于快速配置IMU963RA设备的通道信息
//-------------------------------------------------------------------------------------------------------------------
#define IMU963RA_CHAN(_type, _channel2, _index)                    \
    {                                                             \
        .type = _type,                                        \
        .modified = 1,                                        \
        .channel2 = _channel2,                                \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	      \
        .scan_index = _index,                                 \
        .scan_type = {                                        \
                .sign = 's',                          \
                .realbits = 16,                       \
                .storagebits = 16,                    \
                .shift = 0,                           \
                .endianness = IIO_BE,                 \
                 },                                       \
    }

// 定义IMU963RA的通道配置数组
static const struct iio_chan_spec imu963ra_channels[] = {
    // 加速度计X轴通道配置
    IMU963RA_CHAN(IIO_ACCEL, IIO_MOD_X, INV_IMU_CHAN_ACC_X),	    
    // 加速度计Y轴通道配置
    IMU963RA_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_IMU_CHAN_ACC_Y),	    
    // 加速度计Z轴通道配置
    IMU963RA_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_IMU_CHAN_ACC_Z),	

    // 陀螺仪X轴通道配置
    IMU963RA_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_IMU_CHAN_GYRO_X),	
    // 陀螺仪Y轴通道配置
    IMU963RA_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_IMU_CHAN_GYRO_Y),	
    // 陀螺仪Z轴通道配置
    IMU963RA_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_IMU_CHAN_GYRO_Z),	

    // 磁力计X轴通道配置
    IMU963RA_CHAN(IIO_MAGN, IIO_MOD_X, INV_IMU_CHAN_MAG_X),	
    // 磁力计Y轴通道配置
    IMU963RA_CHAN(IIO_MAGN, IIO_MOD_Y, INV_IMU_CHAN_MAG_Y),	
    // 磁力计Z轴通道配置
    IMU963RA_CHAN(IIO_MAGN, IIO_MOD_Z, INV_IMU_CHAN_MAG_Z),

};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 写寄存器
// 参数说明     reg             寄存器地址
// 参数说明     data            数据
// 返回参数     void
// 使用示例     imu963ra_write_acc_gyro_register(IMU963RA_SLV0_CONFIG, 0x00);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_write_acc_gyro_register(reg, data)     (regmap_write(dev->regmap, reg, data))

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 读寄存器
// 参数说明     reg             寄存器地址
// 返回参数     uint8           数据
// 使用示例     imu963ra_read_acc_gyro_register(IMU963RA_STATUS_MASTER);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_read_acc_gyro_register(reg)\
({\
    uint8 ret = 0; \
    uint32 data = 0; \
    ret = regmap_read(dev->regmap, reg, &data); \
    data; \
})

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 读数据 内部调用
// 参数说明     reg             寄存器地址
// 参数说明     data            数据缓冲区
// 参数说明     len             数据长度
// 返回参数     void
// 使用示例     imu963ra_read_acc_gyro_registers(IMU963RA_OUTX_L_A, dat, 6);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_read_acc_gyro_registers(reg, data, len)     (regmap_bulk_read(dev->regmap, reg, data, len))



//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 作为 IIC 主机向磁力计写数据
// 参数说明     addr            目标地址
// 参数说明     reg             目标寄存器
// 参数说明     data            数据
// 返回参数     uint8           1-失败 0-成功
// 使用示例     imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CONTROL2, 0x80);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_write_mag_register(addr, reg, data)\
({\
    uint8 return_state = 0;\
    uint16 timeout_count = 0;\
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_CONFIG, 0x00);\
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_ADD, addr << 1 | 0);\
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_SUBADD, reg);\
    imu963ra_write_acc_gyro_register(IMU963RA_DATAWRITE_SLV0, data);\
    imu963ra_write_acc_gyro_register(IMU963RA_MASTER_CONFIG, 0x4C);\
    while(0 == (0x80 & imu963ra_read_acc_gyro_register(IMU963RA_STATUS_MASTER)))\
    {\
        if(IMU963RA_TIMEOUT_COUNT < timeout_count ++)\
        {\
            return_state = 1;\
            break;\
        }\
        mdelay(2);\
    }\
    return_state;\
})



//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 作为 IIC 主机向磁力计读数据
// 参数说明     addr            目标地址
// 参数说明     reg             目标寄存器
// 返回参数     uint8           读取的数据
// 使用示例     imu963ra_read_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CHIP_ID);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_read_mag_register(addr, reg) \
({\
    uint16 timeout_count = 0;\
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_ADD, (addr << 1) | 1); \
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_SUBADD, reg); \
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_CONFIG, 0x01); \
    imu963ra_write_acc_gyro_register(IMU963RA_MASTER_CONFIG, 0x4C); \
    while(0 == (0x01 & imu963ra_read_acc_gyro_register(IMU963RA_STATUS_MASTER))) \
    { \
        if(IMU963RA_TIMEOUT_COUNT < timeout_count ++) \
        { \
            break; \
        } \
        mdelay(2); \
    } \
    (imu963ra_read_acc_gyro_register(IMU963RA_SENSOR_HUB_1)); \
})

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 作为 IIC 主机向磁力计自动写数据
// 参数说明     addr            目标地址
// 参数说明     reg             目标寄存器
// 返回参数     void
// 使用示例     imu963ra_connect_mag(IMU963RA_MAG_ADDR, IMU963RA_MAG_OUTX_L);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_connect_mag(addr, reg) \
({ \
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_ADD, (addr << 1) | 1); \
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_SUBADD, reg); \
    imu963ra_write_acc_gyro_register(IMU963RA_SLV0_CONFIG, 0x06); \
    imu963ra_write_acc_gyro_register(IMU963RA_MASTER_CONFIG, 0x6C); \
})

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 IMU963RA 加速度计数据
// 参数说明     dev             IMU963RA设备结构体指针，指向要获取数据的设备实例
// 参数说明     axis            轴编号，指定要获取哪个轴的加速度计数据
// 参数说明     val             数据指针，用于存储获取到的加速度计数据
// 返回参数     int             数据类型标识，这里返回IIO_VAL_INT表示返回整数类型数据
// 使用示例     imu963ra_get_acc(dev, IIO_MOD_X, &val);
// 备注信息     从设备读取指定轴的加速度计原始数据
//-------------------------------------------------------------------------------------------------------------------
int imu963ra_get_acc (struct imu963ra_dev_struct *dev, int axis, int *val)
{
    uint8_t dat[2];

    // axis_x = 1
    // axis_y = 2
    // axis_z = 3

    // 从指定寄存器地址读取加速度计数据
    imu963ra_read_acc_gyro_registers(IMU963RA_OUTX_L_A + (axis - IIO_MOD_X) * 2, dat, 2);
    // 将读取到的两个字节数据合并为一个16位整数
    *val = (int16_t)(((uint16_t)dat[1] << 8 | dat[0]));

    return IIO_VAL_INT;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取IMU963RA陀螺仪数据
// 参数说明     dev             IMU963RA设备结构体指针，指向要获取数据的设备实例
// 参数说明     axis            轴编号，指定要获取哪个轴的陀螺仪数据
// 参数说明     val             数据指针，用于存储获取到的陀螺仪数据
// 返回参数     int             数据类型标识，这里返回IIO_VAL_INT表示返回整数类型数据
// 使用示例     imu963ra_get_gyro(dev, IIO_MOD_X, &val);
// 备注信息     从设备读取指定轴的陀螺仪原始数据
//-------------------------------------------------------------------------------------------------------------------
int imu963ra_get_gyro (struct imu963ra_dev_struct *dev, int axis, int *val)
{
    uint8_t dat[2];

    // axis_x = 1
    // axis_y = 2
    // axis_z = 3

    // 从指定寄存器地址读取陀螺仪数据
    imu963ra_read_acc_gyro_registers(IMU963RA_OUTX_L_G + (axis - IIO_MOD_X) * 2, dat, 2);
    // 将读取到的两个字节数据合并为一个16位整数
    *val = (int16_t)(((uint16_t)dat[1] << 8 | dat[0]));

    return IIO_VAL_INT;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取IMU963RA磁力计
// 参数说明     dev             IMU963RA设备结构体指针，指向要获取数据的设备实例
// 参数说明     axis            轴编号，指定要获取哪个轴的磁力计数据
// 参数说明     val             数据指针，用于存储获取到的磁力计数据
// 返回参数     int             数据类型标识，这里返回IIO_VAL_INT表示返回整数类型数据
// 使用示例     imu963ra_get_gyro(dev, IIO_MOD_X, &val);
// 备注信息     从设备读取指定轴的磁力计原始数据
//-------------------------------------------------------------------------------------------------------------------
int imu963ra_get_mag (struct imu963ra_dev_struct *dev, int axis, int *val)
{
    static int16 mag_dat[3] = {0};
    uint8 temp_status;
    uint8 dat[6];

    imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x40);
    temp_status = imu963ra_read_acc_gyro_register(IMU963RA_STATUS_MASTER);
    if(0x01 & temp_status)
    {
        imu963ra_read_acc_gyro_registers(IMU963RA_SENSOR_HUB_1, dat, 6);
        mag_dat[0] = (int16)(((uint16)dat[1] << 8 | dat[0]));
        mag_dat[1] = (int16)(((uint16)dat[3] << 8 | dat[2]));
        mag_dat[2] = (int16)(((uint16)dat[5] << 8 | dat[4]));
    }
    imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x00);

    *val = mag_dat[axis - IIO_MOD_X];

    return IIO_VAL_INT;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 六轴自检 内部调用
// 参数说明     void
// 返回参数     uint8           1-自检失败 0-自检成功
// 使用示例     imu963ra_acc_gyro_self_check();
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static uint8 imu963ra_acc_gyro_self_check (struct imu963ra_dev_struct *dev)
{
    uint8 return_state = 0;
    uint8 dat = 0;
    uint16 timeout_count = 0;

    while(0x6B != dat)                                                          // 判断 ID 是否正确
    {
        if(IMU963RA_TIMEOUT_COUNT < timeout_count ++)
        {
            return_state = 1;
            break;
        }
        dat = imu963ra_read_acc_gyro_register(IMU963RA_WHO_AM_I);

        dev_info(&dev->spi->dev, "imu963ra_acc_gyro error dat = %d\r\n", dat);  

        mdelay(10);
    }
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU963RA 磁力计自检 内部调用
// 参数说明     void
// 返回参数     uint8           1-自检失败 0-自检成功
// 使用示例     imu963ra_mag_self_check();
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static uint8 imu963ra_mag_self_check (struct imu963ra_dev_struct *dev)
{
    uint8 return_state = 0;
    uint8 dat = 0;
    uint16 timeout_count = 0;

    while(0xff != dat)                                                          // 判断 ID 是否正确
    {
        if(IMU963RA_TIMEOUT_COUNT < timeout_count ++)
        {
            return_state = 1;
            break;
        }
        dat = imu963ra_read_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CHIP_ID);

        dev_info(&dev->spi->dev, "imu963ra_mag_self error dat = %d\r\n", dat);  

        mdelay(10);
    }
    return return_state;
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     初始化 IMU963RA
// 参数说明     dev             IMU963RA设备结构体指针，指向要初始化的设备实例
// 返回参数     uint8           1-初始化失败 0-初始化成功
// 使用示例     imu963ra_init(dev);
// 备注信息     对IMU963RA设备进行初始化操作，包括自检、配置寄存器
//-------------------------------------------------------------------------------------------------------------------
uint8 imu963ra_init(struct imu963ra_dev_struct *dev)
{
    uint8 return_state = 0;

	do
    {
        imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x00);       // 关闭HUB寄存器访问
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL3_C, 0x01);               // 复位设备
        mdelay(2);                             
        imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x00);       // 关闭HUB寄存器访问
        if(imu963ra_acc_gyro_self_check(dev))                   
        {                   
            dev_warn(&dev->spi->dev, "IMU963RA acc and gyro self check error.");                    
            return_state = 1;
            break;            
        }                   
                            
        imu963ra_write_acc_gyro_register(IMU963RA_INT1_CTRL, 0x03);             // 开启陀螺仪 加速度数据就绪中断

        // IMU963RA_CTRL1_XL 寄存器
        // 设置为 0x30 加速度量程为 ±2  G    获取到的加速度计数据除以 16393  可以转化为带物理单位的数据 单位 g(m/s^2)
        // 设置为 0x38 加速度量程为 ±4  G    获取到的加速度计数据除以 8197   可以转化为带物理单位的数据 单位 g(m/s^2)
        // 设置为 0x3C 加速度量程为 ±8  G    获取到的加速度计数据除以 4098   可以转化为带物理单位的数据 单位 g(m/s^2)
        // 设置为 0x34 加速度量程为 ±16 G    获取到的加速度计数据除以 2049   可以转化为带物理单位的数据 单位 g(m/s^2)
        switch(IMU963RA_ACC_SAMPLE_DEFAULT)
        {
            default:
            {
                dev_warn(&dev->spi->dev, "IMU963RA_ACC_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case IMU963RA_ACC_SAMPLE_SGN_2G:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL1_XL, 0x30);
                imu963ra_transition_factor[0] = 16393;
            }break;
            case IMU963RA_ACC_SAMPLE_SGN_4G:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL1_XL, 0x38);
                imu963ra_transition_factor[0] = 8197;
            }break;
            case IMU963RA_ACC_SAMPLE_SGN_8G:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL1_XL, 0x3C);
                imu963ra_transition_factor[0] = 4098;
            }break;
            case IMU963RA_ACC_SAMPLE_SGN_16G:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL1_XL, 0x34);
                imu963ra_transition_factor[0] = 2049;
            }break;
        }
        if(1 == return_state)
        {
            break;
        }

        // IMU963RA_CTRL2_G 寄存器
        // 设置为 0x52 陀螺仪量程为 ±125  dps    获取到的陀螺仪数据除以 228.6   可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x50 陀螺仪量程为 ±250  dps    获取到的陀螺仪数据除以 114.3   可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x54 陀螺仪量程为 ±500  dps    获取到的陀螺仪数据除以 57.1    可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x58 陀螺仪量程为 ±1000 dps    获取到的陀螺仪数据除以 28.6    可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x5C 陀螺仪量程为 ±2000 dps    获取到的陀螺仪数据除以 14.3    可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x51 陀螺仪量程为 ±4000 dps    获取到的陀螺仪数据除以 7.1     可以转化为带物理单位的数据 单位为 °/s
        switch(IMU963RA_GYRO_SAMPLE_DEFAULT)
        {
            default:
            {
                dev_warn(&dev->spi->dev, "IMU963RA_GYRO_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_125DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x52);
                imu963ra_transition_factor[1] = 228.6;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_250DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x50);
                imu963ra_transition_factor[1] = 114.3;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_500DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x54);
                imu963ra_transition_factor[1] = 57.1;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_1000DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x58);
                imu963ra_transition_factor[1] = 28.6;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_2000DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x5C);
                imu963ra_transition_factor[1] = 14.3;
            }break;
            case IMU963RA_GYRO_SAMPLE_SGN_4000DPS:
            {
                imu963ra_write_acc_gyro_register(IMU963RA_CTRL2_G, 0x51);
                imu963ra_transition_factor[1] = 7.1;
            }break;
        }
        if(1 == return_state)
        {
            break;
        }

        imu963ra_write_acc_gyro_register(IMU963RA_CTRL3_C, 0x44);               // 使能陀螺仪数字低通滤波器
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL4_C, 0x02);               // 使能数字低通滤波器
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL5_C, 0x00);               // 加速度计与陀螺仪四舍五入
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL6_C, 0x00);               // 开启加速度计高性能模式 陀螺仪低通滤波 133hz
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL7_G, 0x00);               // 开启陀螺仪高性能模式 关闭高通滤波
        imu963ra_write_acc_gyro_register(IMU963RA_CTRL9_XL, 0x01);              // 关闭I3C接口

        imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x40);       // 开启HUB寄存器访问 用于配置地磁计
        imu963ra_write_acc_gyro_register(IMU963RA_MASTER_CONFIG, 0x80);         // 复位I2C主机
        mdelay(2);                             
        imu963ra_write_acc_gyro_register(IMU963RA_MASTER_CONFIG, 0x00);         // 清除复位标志
        mdelay(2);
        
        imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CONTROL2, 0x80);// 复位连接的外设
        mdelay(2);
        imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CONTROL2, 0x00);
        mdelay(2);

        if(imu963ra_mag_self_check(dev))
        {
            dev_warn(&dev->spi->dev, "IMU963RA mag self check error.");
            return_state = 1;
            break;            
        }

        // IMU963RA_MAG_ADDR 寄存器
        // 设置为 0x09 磁力计量程为 2G   获取到的磁力计数据除以 12000   可以转化为带物理单位的数据 单位 G(高斯)
        // 设置为 0x19 磁力计量程为 8G   获取到的磁力计数据除以 3000    可以转化为带物理单位的数据 单位 G(高斯)
        switch(IMU963RA_MAG_SAMPLE_DEFAULT)
        {
            default:
            {
                dev_warn(&dev->spi->dev, "IMU963RA_MAG_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case IMU963RA_MAG_SAMPLE_2G:
            {
                imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CONTROL1, 0x09);
                imu963ra_transition_factor[2] = 12000;
            }break;
            case IMU963RA_MAG_SAMPLE_8G:
            {
                imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_CONTROL1, 0x19);
                imu963ra_transition_factor[2] = 3000;
            }break;
        }
        if(1 == return_state)
        {
            break;
        }

        imu963ra_write_mag_register(IMU963RA_MAG_ADDR, IMU963RA_MAG_FBR, 0x01);
        imu963ra_connect_mag(IMU963RA_MAG_ADDR, IMU963RA_MAG_OUTX_L);
        
        imu963ra_write_acc_gyro_register(IMU963RA_FUNC_CFG_ACCESS, 0x00);       // 关闭HUB寄存器访问

        mdelay(20);                                                    // 等待磁力计获取数据
    }while(0);

    return return_state;
}

static int imu963ra_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	struct imu963ra_dev_struct *dev = iio_priv(indio_dev);
	int ret = 0;

	switch (mask) 
    {
        case IIO_CHAN_INFO_RAW:
            mutex_lock(&dev->lock);
            switch (chan->type) 
            {
                case IIO_ACCEL:		// 加速度计原始数据
                    ret = imu963ra_get_acc(dev, chan->channel2, val);
                    break;
                case IIO_ANGL_VEL:	// 陀螺仪原始数据
                    ret = imu963ra_get_gyro(dev, chan->channel2, val);
                    break;
                case IIO_MAGN:	// 陀螺仪原始数据
                    ret = imu963ra_get_mag(dev, chan->channel2, val);
                    break;
                default:
                    ret = -EINVAL;
                    break;
            }
            mutex_unlock(&dev->lock);

            break;
        default:
            ret = -EINVAL;
            break;
	}

    return ret;
}	

// iio_info结构体变量
static const struct iio_info imu963ra_info = {
	.read_raw		= imu963ra_read_raw,
	// .write_raw		= imu963ra_write_raw,
	// .write_raw_get_fmt = &imu963ra_write_raw_get_fmt,	/* 用户空间写数据格式 */




};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动初始化
// 参数说明     spi  SPI设备结构体指针
// 返回参数     int  0 - 初始化成功; 负数 - 初始化失败
// 使用示例     由Linux自动调用
// 备注信息     该函数用于初始化IMU963RA设备驱动，包括分配资源、注册字符设备等操作
//-------------------------------------------------------------------------------------------------------------------
static int imu963ra_probe(struct spi_device *spi)
{
    int ret;
    struct imu963ra_dev_struct *dev;
    struct iio_dev *indio_dev;

    // 打印调试信息，表明进入驱动初始化流程
    printk("imu963ra_probe\r\n");

    // 1、申请 iio_dev 内存
    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
    if (!indio_dev) 
    {
        // 若内存分配失败，使用 dev_err 打印错误信息并返回内存不足错误码
        dev_err(&spi->dev, "Failed to allocate iio_dev memory for %s\n", IMU963RA_NAME);
        return -ENOMEM;
    }

    // 2、获取 imu963ra_dev 结构体地址
    dev = iio_priv(indio_dev); 
    dev->spi = spi;
    // 将 indio_dev 设置为 spi->dev 的 driver_data，方便后续通过 spi 设备获取 iio_dev
    spi_set_drvdata(spi, indio_dev);
    // 初始化互斥锁，用于保护对设备的并发访问
    mutex_init(&dev->lock);

    // 3、设置 iio_dev 的其他成员变量
    // 设置 iio_dev 的父设备为当前 spi 设备
    indio_dev->dev.parent = &spi->dev;
    // 关联 iio_dev 的操作函数信息
    indio_dev->info = &imu963ra_info;
    // 设置 iio_dev 的名称
    indio_dev->name = IMU963RA_NAME;
    // 设置工作模式为直接模式，提供 sysfs 接口
    indio_dev->modes = INDIO_DIRECT_MODE;
    // 关联 iio_dev 的通道配置
    indio_dev->channels = imu963ra_channels;
    // 设置 iio_dev 的通道数量
    indio_dev->num_channels = ARRAY_SIZE(imu963ra_channels);

    // 4、注册 iio_dev
    ret = iio_device_register(indio_dev);
    if (ret < 0) 
    {
        // 若注册失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to register iio device for %s\n", IMU963RA_NAME);
        goto err_iio_register;
    }

    // 初始化 regmap_config 设置
    // 寄存器长度为 8 位
    dev->reg_cfg.reg_bits = 8;
    // 值长度为 8 位
    dev->reg_cfg.val_bits = 8;
    // 读掩码设置为 0X80，imu963ra 使用 SPI 接口读的时候寄存器最高位应该为 1
    dev->reg_cfg.read_flag_mask = 0x80;

    // 初始化 spi 接口的 regmap
    dev->regmap = regmap_init_spi(spi, &dev->reg_cfg);
    if (IS_ERR(dev->regmap)) 
    {
        // 若 regmap 初始化失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to initialize regmap for %s\n", IMU963RA_NAME);
        ret = PTR_ERR(dev->regmap);
        goto err_regmap_init;
    }

    // 保存 spi 设备指针
    dev->spi = spi;

    // 初始化 spi_device，设置模式为 0
    spi->mode = SPI_MODE_0;
    // 应用 SPI 设备的设置
    ret = spi_setup(spi);
    if (ret < 0) 
    {
        // 若 spi 设置失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to setup SPI device for %s\n", IMU963RA_NAME);
        goto err_imu963ra_init;
    }

    // 使用寄存器初始化 imu963ra
    if (imu963ra_init(dev)) 
    {
        // 若初始化失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to initialize imu963ra device %s\n", IMU963RA_NAME);
        ret = -EIO;
        goto err_imu963ra_init;
    } 
    else 
    {
        // 若初始化成功，使用 dev_info 打印成功信息
        dev_info(&spi->dev, "imu963ra_init ok for %s.\r\n", IMU963RA_NAME);
    }

    // 初始化成功，返回 0
    return 0;

err_imu963ra_init:
    // 若 imu963ra 初始化失败，退出 regmap
    if (dev->regmap) 
    {
        regmap_exit(dev->regmap);
    }
err_regmap_init:
    // 若 regmap 初始化失败，注销 iio_dev
    if (indio_dev)
    {
        iio_device_unregister(indio_dev);
    }
err_iio_register:
    // 返回错误码
    return ret;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动移除
// 参数说明     spi  SPI设备结构体指针，用于获取设备驱动数据
// 返回参数     int  通常返回0表示移除成功
// 使用示例     由Linux内核自动调用，当设备移除时触发
// 备注信息     该函数负责释放驱动初始化时分配的各种资源，包括字符设备、设备号、设备类、设备实例和寄存器映射
//-------------------------------------------------------------------------------------------------------------------
static int imu963ra_remove(struct spi_device *spi)
{
    // 从SPI设备中获取驱动数据
	struct iio_dev *indio_dev = spi_get_drvdata(spi);
	struct imu963ra_dev_struct *dev = iio_priv(indio_dev);
	
	/* 1、删除regmap */ 
	regmap_exit(dev->regmap);

	/* 2、注销IIO */
	iio_device_unregister(indio_dev);

    // 打印移除成功信息
    pr_info("imu963ra_remove: Successfully removed driver for device %s.\r\n", IMU963RA_NAME);

    return 0;
}

// 设备树匹配表
static const struct of_device_id imu963ra_of_match[] = {
	{ .compatible = "seekfree,imu963ra" },
	{ /* Sentinel */ }
};

// IMU963RA驱动结构体
static struct spi_driver imu963ra_driver = {
	.probe = imu963ra_probe,
	.remove = imu963ra_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "imu963ra",
		   	.of_match_table = imu963ra_of_match,
		   },
};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动入口函数
// 参数说明     void
// 返回参数     int          
// 使用示例     由linux自动调用
// 备注信息     
//-------------------------------------------------------------------------------------------------------------------
static int __init imu963ra_driver_init(void)
{
	return spi_register_driver(&imu963ra_driver);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动出口函数
// 参数说明     void
// 返回参数     int          
// 使用示例     由linux自动调用
// 备注信息     
//-------------------------------------------------------------------------------------------------------------------
static void __exit imu963ra_driver_exit(void)
{
	spi_unregister_driver(&imu963ra_driver);
}

module_init(imu963ra_driver_init);
module_exit(imu963ra_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("seekfree_bigW");
