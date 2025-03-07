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

#include "zf_device_imu660ra.h"
#include "zf_device_config.h"

#define IMU660RA_NAME	"zf_device_imu660ra"
float imu660ra_transition_factor[2] = {0};

struct imu660ra_dev_struct 
{
	int16 gyro_x;						// 陀螺仪X轴原始数据 	
	int16 gyro_y;						// 陀螺仪Y轴原始数据
	int16 gyro_z;						// 陀螺仪Z轴原始数据 
	int16 accel_x;					    // 加速度计X轴原始数据
	int16 accel_y;					    // 加速度计Y轴原始数据
	int16 accel_z;					    // 加速度计Z轴原始数据

	struct spi_device *spi;				// spi设备
	struct regmap *regmap;				// 寄存器访问接口
	struct regmap_config reg_cfg;		// 寄存器访问接口
    struct mutex lock;
};

//-------------------------------------------------------------------------------------------------------------------
// 枚举简介     IMU660RA扫描元素枚举
// 成员说明     INV_IMU_CHAN_ACCL_X    加速度计X轴扫描元素
// 成员说明     INV_IMU_CHAN_ACCL_Y    加速度计Y轴扫描元素
// 成员说明     INV_IMU_CHAN_ACCL_Z    加速度计Z轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_X    陀螺仪X轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_Y    陀螺仪Y轴扫描元素
// 成员说明     INV_IMU_CHAN_GYRO_Z    陀螺仪Z轴扫描元素
// 使用示例     enum inv_icm20608_scan my_scan_element = INV_IMU_CHAN_ACCL_X;
// 备注信息     用于标识IMU660RA设备不同轴的扫描元素
//-------------------------------------------------------------------------------------------------------------------
enum inv_imu_chan 
{
    INV_IMU_CHAN_ACC_X,
    INV_IMU_CHAN_ACC_Y,
    INV_IMU_CHAN_ACC_Z,
    INV_IMU_CHAN_GYRO_X,
    INV_IMU_CHAN_GYRO_Y,
    INV_IMU_CHAN_GYRO_Z,
};

//-------------------------------------------------------------------------------------------------------------------
// 宏简介     IMU660RA通道配置宏
// 参数说明     _type       通道类型，如IIO_ACCEL（加速度计）、IIO_ANGL_VEL（陀螺仪）等
// 参数说明     _channel2   通道修饰符，如IIO_MOD_X、IIO_MOD_Y、IIO_MOD_Z等
// 参数说明     _index      扫描索引，用于确定通道在扫描序列中的位置
// 使用示例     IMU660RA_CHAN(IIO_ACCEL, IIO_MOD_X, INV_IMU_CHAN_ACCL_X);
// 备注信息     用于快速配置IMU660RA设备的通道信息
//-------------------------------------------------------------------------------------------------------------------
#define IMU660RA_CHAN(_type, _channel2, _index)                    \
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

// 定义IMU660RA的通道配置数组
static const struct iio_chan_spec imu660ra_channels[] = {
    // 加速度计X轴通道配置
    IMU660RA_CHAN(IIO_ACCEL, IIO_MOD_X, INV_IMU_CHAN_ACC_X),	    
    // 加速度计Y轴通道配置
    IMU660RA_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_IMU_CHAN_ACC_Y),	    
    // 加速度计Z轴通道配置
    IMU660RA_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_IMU_CHAN_ACC_Z),	    
    // 陀螺仪X轴通道配置
    IMU660RA_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_IMU_CHAN_GYRO_X),	
    // 陀螺仪Y轴通道配置
    IMU660RA_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_IMU_CHAN_GYRO_Y),	
    // 陀螺仪Z轴通道配置
    IMU660RA_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_IMU_CHAN_GYRO_Z),	
};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU660RA 读寄存器
// 参数说明     dev             IMU660RA设备结构体指针，指向要操作的设备实例
// 参数说明     reg             寄存器地址，指定要读取的寄存器位置
// 返回参数     uint8           从指定寄存器读取到的数据
// 使用示例     imu660ra_read_register(dev, IMU660RA_WHO_AM_I);
// 备注信息     内部调用，用于驱动内部对设备寄存器的读取操作
//-------------------------------------------------------------------------------------------------------------------
static uint8_t imu660ra_read_register(struct imu660ra_dev_struct *dev, uint8_t reg)
{
    uint8_t ret;
    uint8_t data[2];

    // 使用寄存器映射接口批量读取寄存器数据
    ret = regmap_bulk_read(dev->regmap, reg, data, 2);

    return data[1]; 
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU660RA 写寄存器
// 参数说明     dev             IMU660RA设备结构体指针，指向要操作的设备实例
// 参数说明     reg             寄存器地址，指定要写入的寄存器位置
// 参数说明     buff            数据缓冲区指针，包含要写入寄存器的数据
// 参数说明     len             数据长度，指定要写入的数据字节数
// 返回参数     void
// 使用示例     imu660ra_write_registers(dev, IMU660RA_PWR_MGMT_1, buff, len);
// 备注信息     内部调用，用于驱动内部对设备寄存器的批量写入操作
//-------------------------------------------------------------------------------------------------------------------
static void imu660ra_write_registers(struct imu660ra_dev_struct *dev, uint8_t reg, uint8_t *buff, uint32_t len)
{
    // 使用寄存器映射接口批量写入寄存器数据
    regmap_bulk_write(dev->regmap,  reg, buff, len);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU660RA 写寄存器
// 参数说明     dev             IMU660RA设备结构体指针，指向要操作的设备实例
// 参数说明     reg             寄存器地址，指定要写入的寄存器位置
// 参数说明     value           数据，要写入指定寄存器的值
// 返回参数     void
// 使用示例     imu660ra_write_register(dev, IMU660RA_PWR_MGMT_1, 0x80);
// 备注信息     内部调用，用于驱动内部对设备单个寄存器的写入操作
//-------------------------------------------------------------------------------------------------------------------
static void imu660ra_write_register(struct imu660ra_dev_struct *dev, uint8_t reg, uint8_t value)
{
    // 使用寄存器映射接口写入单个寄存器数据
    regmap_write(dev->regmap, reg, value);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU660RA 读数据
// 参数说明     dev             IMU660RA设备结构体指针，指向要操作的设备实例
// 参数说明     reg             寄存器地址，指定要读取数据的起始寄存器位置
// 参数说明     data            数据缓冲区指针，用于存储读取到的数据
// 参数说明     len             数据长度，指定要读取的数据字节数
// 返回参数     void
// 使用示例     imu660ra_read_registers(dev, IMU660RA_ACCEL_XOUT_H, dat, 6);
// 备注信息     内部调用，用于驱动内部从设备连续寄存器读取数据
//-------------------------------------------------------------------------------------------------------------------
static void imu660ra_read_registers(struct imu660ra_dev_struct *dev, uint8_t reg, uint8_t *data, uint32_t len)
{
    // 使用寄存器映射接口批量读取连续寄存器数据
    regmap_bulk_read(dev->regmap, reg, data, len);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     IMU660RA 自检
// 参数说明     dev             IMU660RA设备结构体指针，指向要进行自检的设备实例
// 返回参数     uint8           1-自检失败 0-自检成功
// 使用示例     imu660ra_self_check(dev);
// 备注信息     内部调用，用于在设备初始化时检查设备是否正常工作
//-------------------------------------------------------------------------------------------------------------------
static uint8_t imu660ra_self_check (struct imu660ra_dev_struct *dev)
{
    uint8_t dat = 0, return_state = 0;
    uint16_t timeout_count = 0;

    // 循环读取设备ID寄存器，直到超时或读取到正确的设备ID
    do
    {
        if(IMU660RA_TIMEOUT_COUNT < timeout_count ++)
        {
            return_state =  1;
            break;
        }
        dat = imu660ra_read_register(dev, IMU660RA_CHIP_ID);
        mdelay(10);
    }while(0x24 != dat);                                                        // 读取设备ID是否等于0X24，如果不是0X24则认为没检测到设备

    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 IMU660RA 加速度计数据
// 参数说明     dev             IMU660RA设备结构体指针，指向要获取数据的设备实例
// 参数说明     axis            轴编号，指定要获取哪个轴的加速度计数据
// 参数说明     val             数据指针，用于存储获取到的加速度计数据
// 返回参数     int             数据类型标识，这里返回IIO_VAL_INT表示返回整数类型数据
// 使用示例     imu660ra_get_acc(dev, IIO_MOD_X, &val);
// 备注信息     从设备读取指定轴的加速度计原始数据
//-------------------------------------------------------------------------------------------------------------------
int imu660ra_get_acc (struct imu660ra_dev_struct *dev, int axis, int *val)
{
    // SPI读取第一个地址为空
    uint8_t dat[3];

    // axis_x = 1
    // axis_y = 2
    // axis_z = 3

    // 从指定寄存器地址读取加速度计数据
    imu660ra_read_registers(dev, IMU660RA_ACC_ADDRESS + (axis - IIO_MOD_X) * 2, dat, 3);

    // 将读取到的两个字节数据合并为一个16位整数
    *val = (int16_t)(((uint16_t)dat[2] << 8 | dat[1]));

    return IIO_VAL_INT;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取IMU660RA陀螺仪数据
// 参数说明     dev             IMU660RA设备结构体指针，指向要获取数据的设备实例
// 参数说明     axis            轴编号，指定要获取哪个轴的陀螺仪数据
// 参数说明     val             数据指针，用于存储获取到的陀螺仪数据
// 返回参数     int             数据类型标识，这里返回IIO_VAL_INT表示返回整数类型数据
// 使用示例     imu660ra_get_gyro(dev, IIO_MOD_X, &val);
// 备注信息     从设备读取指定轴的陀螺仪原始数据
//-------------------------------------------------------------------------------------------------------------------
int imu660ra_get_gyro (struct imu660ra_dev_struct *dev, int axis, int *val)
{
    uint8_t dat[3];

    // axis_x = 1
    // axis_y = 2
    // axis_z = 3

    // 从指定寄存器地址读取陀螺仪数据
    imu660ra_read_registers(dev, IMU660RA_GYRO_ADDRESS + (axis - IIO_MOD_X) * 2, dat, 3);

    // 将读取到的两个字节数据合并为一个16位整数
    *val = (int16_t)(((uint16_t)dat[2] << 8 | dat[1]));

    return IIO_VAL_INT;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     初始化 IMU660RA
// 参数说明     dev             IMU660RA设备结构体指针，指向要初始化的设备实例
// 返回参数     uint8           1-初始化失败 0-初始化成功
// 使用示例     imu660ra_init(dev);
// 备注信息     对IMU660RA设备进行初始化操作，包括自检、配置寄存器
//-------------------------------------------------------------------------------------------------------------------
uint8 imu660ra_init(struct imu660ra_dev_struct *dev)
{
    uint8 return_state = 0;

	do
    {

		imu660ra_read_register(dev, IMU660RA_CHIP_ID);                                   // 读取一下设备ID 将设备设置为SPI模式
        
		if(imu660ra_self_check(dev))
        {
            // 如果程序在输出了断言信息 并且提示出错位置在这里
            // 那么就是 IMU660RA 自检出错并超时退出了
            // 检查一下接线有没有问题 如果没问题可能就是坏了
			dev_err(&dev->spi->dev, "imu660ra self check error.\r\n");
            dev_err(&dev->spi->dev, "imu660ra self check error.\r\n");
            dev_err(&dev->spi->dev, "imu660ra self check error.\r\n");

            return_state = 1;
            break;
        }

		imu660ra_write_register(dev, IMU660RA_PWR_CONF, 0x00);                       // 关闭高级省电模式
        mdelay(1);
        imu660ra_write_register(dev, IMU660RA_INIT_CTRL, 0x00);                      // 开始对模块进行初始化配置
        imu660ra_write_registers(dev, IMU660RA_INIT_DATA, (uint8 *)imu660ra_config_file, sizeof(imu660ra_config_file));   // 输出配置文件
        imu660ra_write_register(dev, IMU660RA_INIT_CTRL, 0x01);                      // 初始化配置结束
        mdelay(20);
        if(1 != imu660ra_read_register(dev, IMU660RA_INT_STA))                       // 检查是否配置完成
        {
            // 如果程序在输出了断言信息 并且提示出错位置在这里
            // 那么就是 IMU660RA 配置初始化文件出错了
            // 检查一下接线有没有问题 如果没问题可能就是坏了
            dev_err(&dev->spi->dev, "imu660ra init error.");
            dev_err(&dev->spi->dev, "imu660ra init error.");
            dev_err(&dev->spi->dev, "imu660ra init error.");

            return_state = 1;
            break;
        }
        imu660ra_write_register(dev, IMU660RA_PWR_CTRL, 0x0E);                       // 开启性能模式  使能陀螺仪、加速度、温度传感器
        imu660ra_write_register(dev, IMU660RA_ACC_CONF, 0xA7);                       // 加速度采集配置 性能模式 正常采集 50Hz  采样频率
        imu660ra_write_register(dev, IMU660RA_GYR_CONF, 0xA9);                       // 陀螺仪采集配置 性能模式 正常采集 200Hz 采样频率

        // IMU660RA_ACC_SAMPLE 寄存器
        // 设置为 0x00 加速度计量程为 ±2  g   获取到的加速度计数据除以 16384  可以转化为带物理单位的数据 (g 代表重力加速度 物理学名词 一般情况下 g 取 9.8 m/s^2 为标准值)
        // 设置为 0x01 加速度计量程为 ±4  g   获取到的加速度计数据除以 8192   可以转化为带物理单位的数据 (g 代表重力加速度 物理学名词 一般情况下 g 取 9.8 m/s^2 为标准值)
        // 设置为 0x02 加速度计量程为 ±8  g   获取到的加速度计数据除以 4096   可以转化为带物理单位的数据 (g 代表重力加速度 物理学名词 一般情况下 g 取 9.8 m/s^2 为标准值)
        // 设置为 0x03 加速度计量程为 ±16 g   获取到的加速度计数据除以 2048   可以转化为带物理单位的数据 (g 代表重力加速度 物理学名词 一般情况下 g 取 9.8 m/s^2 为标准值)
        switch(IMU660RA_ACC_SAMPLE_DEFAULT)
        {
            default:
            {
                dev_err(&dev->spi->dev, "IMU660RA_ACC_SAMPLE_DEFAULT set error.");
                dev_err(&dev->spi->dev, "IMU660RA_ACC_SAMPLE_DEFAULT set error.");
                dev_err(&dev->spi->dev, "IMU660RA_ACC_SAMPLE_DEFAULT set error.");

                return_state = 1;
            }break;
            case IMU660RA_ACC_SAMPLE_SGN_2G:
            {
                imu660ra_write_register(dev, IMU660RA_ACC_RANGE, 0x00);
                imu660ra_transition_factor[0] = 16384;
            }break;
            case IMU660RA_ACC_SAMPLE_SGN_4G:
            {
                imu660ra_write_register(dev, IMU660RA_ACC_RANGE, 0x01);
                imu660ra_transition_factor[0] = 8192;
            }break;
            case IMU660RA_ACC_SAMPLE_SGN_8G:
            {
                imu660ra_write_register(dev, IMU660RA_ACC_RANGE, 0x02);
                imu660ra_transition_factor[0] = 4096;
            }break;
            case IMU660RA_ACC_SAMPLE_SGN_16G:
            {
                imu660ra_write_register(dev, IMU660RA_ACC_RANGE, 0x03);
                imu660ra_transition_factor[0] = 2048;
            }break;
        }
        if(1 == return_state)
        {
            break;
        }

        // IMU660RA_GYR_RANGE 寄存器
        // 设置为 0x04 陀螺仪量程为 ±125  dps    获取到的陀螺仪数据除以 262.4   可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x03 陀螺仪量程为 ±250  dps    获取到的陀螺仪数据除以 131.2   可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x02 陀螺仪量程为 ±500  dps    获取到的陀螺仪数据除以 65.6    可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x01 陀螺仪量程为 ±1000 dps    获取到的陀螺仪数据除以 32.8    可以转化为带物理单位的数据 单位为 °/s
        // 设置为 0x00 陀螺仪量程为 ±2000 dps    获取到的陀螺仪数据除以 16.4    可以转化为带物理单位的数据 单位为 °/s
        switch(IMU660RA_GYRO_SAMPLE_DEFAULT)
        {
            default:
            {
                dev_err(&dev->spi->dev, "IMU660RA_GYRO_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case IMU660RA_GYRO_SAMPLE_SGN_125DPS:
            {
                imu660ra_write_register(dev, IMU660RA_GYR_RANGE, 0x04);
                imu660ra_transition_factor[1] = 262.4;
            }break;
            case IMU660RA_GYRO_SAMPLE_SGN_250DPS:
            {
                imu660ra_write_register(dev, IMU660RA_GYR_RANGE, 0x03);
                imu660ra_transition_factor[1] = 131.2;
            }break;
            case IMU660RA_GYRO_SAMPLE_SGN_500DPS:
            {
                imu660ra_write_register(dev, IMU660RA_GYR_RANGE, 0x02);
                imu660ra_transition_factor[1] = 65.6;
            }break;
            case IMU660RA_GYRO_SAMPLE_SGN_1000DPS:
            {
                imu660ra_write_register(dev, IMU660RA_GYR_RANGE, 0x01);
                imu660ra_transition_factor[1] = 32.8;
            }break;
            case IMU660RA_GYRO_SAMPLE_SGN_2000DPS:
            {
                imu660ra_write_register(dev, IMU660RA_GYR_RANGE, 0x00);
                imu660ra_transition_factor[1] = 16.4;
            }break;
        }
        if(1 == return_state)
        {
            break;
        }
    }while(0);
    return return_state;
}

static int imu660ra_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	struct imu660ra_dev_struct *dev = iio_priv(indio_dev);
	int ret = 0;

	switch (mask) 
    {
        case IIO_CHAN_INFO_RAW:

            mutex_lock(&dev->lock);
            switch (chan->type) 
            {
                case IIO_ACCEL:		// 加速度计原始数据
                    ret = imu660ra_get_acc(dev, chan->channel2, val);
                    break;
                case IIO_ANGL_VEL:	// 陀螺仪原始数据
                    ret = imu660ra_get_gyro(dev, chan->channel2, val);
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
static const struct iio_info imu660ra_info = {
	.read_raw		= imu660ra_read_raw,
	// .write_raw		= imu660ra_write_raw,
	// .write_raw_get_fmt = &imu660ra_write_raw_get_fmt,	/* 用户空间写数据格式 */
};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动初始化
// 参数说明     spi  SPI设备结构体指针
// 返回参数     int  0 - 初始化成功; 负数 - 初始化失败
// 使用示例     由Linux自动调用
// 备注信息     该函数用于初始化IMU660RA设备驱动，包括分配资源、注册字符设备等操作
//-------------------------------------------------------------------------------------------------------------------
static int imu660ra_probe(struct spi_device *spi)
{
    int ret;
    struct imu660ra_dev_struct *dev;
    struct iio_dev *indio_dev;

    // 打印调试信息，表明进入驱动初始化流程
    dev_info(&dev->spi->dev,"imu660ra_probe\r\n");

    // 1、申请 iio_dev 内存
    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
    if (!indio_dev) 
    {
        // 若内存分配失败，使用 dev_err 打印错误信息并返回内存不足错误码
        dev_err(&spi->dev, "Failed to allocate iio_dev memory for %s\n", IMU660RA_NAME);
        return -ENOMEM;
    }

    // 2、获取 imu660ra_dev 结构体地址
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
    indio_dev->info = &imu660ra_info;
    // 设置 iio_dev 的名称
    indio_dev->name = IMU660RA_NAME;
    // 设置工作模式为直接模式，提供 sysfs 接口
    indio_dev->modes = INDIO_DIRECT_MODE;
    // 关联 iio_dev 的通道配置
    indio_dev->channels = imu660ra_channels;
    // 设置 iio_dev 的通道数量
    indio_dev->num_channels = ARRAY_SIZE(imu660ra_channels);

    // 4、注册 iio_dev
    ret = iio_device_register(indio_dev);
    if (ret < 0) 
    {
        // 若注册失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to register iio device for %s\n", IMU660RA_NAME);
        goto err_iio_register;
    }

    // 初始化 regmap_config 设置
    // 寄存器长度为 8 位
    dev->reg_cfg.reg_bits = 8;
    // 值长度为 8 位
    dev->reg_cfg.val_bits = 8;
    // 读掩码设置为 0X80，imu660ra 使用 SPI 接口读的时候寄存器最高位应该为 1
    dev->reg_cfg.read_flag_mask = 0x80;

    // 初始化 spi 接口的 regmap
    dev->regmap = regmap_init_spi(spi, &dev->reg_cfg);
    if (IS_ERR(dev->regmap)) 
    {
        // 若 regmap 初始化失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to initialize regmap for %s\n", IMU660RA_NAME);
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
        dev_err(&spi->dev, "Failed to setup SPI device for %s\n", IMU660RA_NAME);
        goto err_imu660ra_init;
    }

    // 使用寄存器初始化 imu660ra
    if (imu660ra_init(dev)) 
    {
        // 若初始化失败，使用 dev_err 打印错误信息并跳转到错误处理标签
        dev_err(&spi->dev, "Failed to initialize imu660ra device %s\n", IMU660RA_NAME);
        ret = -EIO;
        goto err_imu660ra_init;
    } 
    else 
    {
        // 若初始化成功，使用 dev_info 打印成功信息
        dev_info(&spi->dev, "imu660ra_init ok for %s.\r\n", IMU660RA_NAME);
    }

    // 初始化成功，返回 0
    return 0;

err_imu660ra_init:
    // 若 imu660ra 初始化失败，退出 regmap
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
static int imu660ra_remove(struct spi_device *spi)
{
    // 从SPI设备中获取驱动数据
	struct iio_dev *indio_dev = spi_get_drvdata(spi);
	struct imu660ra_dev_struct *dev = iio_priv(indio_dev);
	
	/* 1、删除regmap */ 
	regmap_exit(dev->regmap);

	/* 2、注销IIO */
	iio_device_unregister(indio_dev);

    // 打印移除成功信息
    pr_info("imu660ra_remove: Successfully removed driver for device %s.\r\n", IMU660RA_NAME);

    return 0;
}

// 设备树匹配表
static const struct of_device_id imu660ra_of_match[] = {
	{ .compatible = "seekfree,imu660ra" },
	{ /* Sentinel */ }
};

// IMU660RA驱动结构体
static struct spi_driver imu660ra_driver = {
	.probe = imu660ra_probe,
	.remove = imu660ra_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "imu660ra",
		   	.of_match_table = imu660ra_of_match,
		   },
};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动入口函数
// 参数说明     void
// 返回参数     int          
// 使用示例     由linux自动调用
// 备注信息     
//-------------------------------------------------------------------------------------------------------------------
static int __init imu660ra_driver_init(void)
{
	return spi_register_driver(&imu660ra_driver);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     驱动出口函数
// 参数说明     void
// 返回参数     int          
// 使用示例     由linux自动调用
// 备注信息     
//-------------------------------------------------------------------------------------------------------------------
static void __exit imu660ra_driver_exit(void)
{
	spi_unregister_driver(&imu660ra_driver);
}

module_init(imu660ra_driver_init);
module_exit(imu660ra_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("seekfree_bigW");
