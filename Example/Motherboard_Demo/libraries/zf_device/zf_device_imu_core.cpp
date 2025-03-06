#include "zf_device_imu_core.h"
#include "zf_driver_file.h"

// iio框架对应的文件路径
const char *imu_file_path[] = 
{
	"/sys/bus/iio/devices/iio:device1/in_accel_x_raw",
	"/sys/bus/iio/devices/iio:device1/in_accel_y_raw",
	"/sys/bus/iio/devices/iio:device1/in_accel_z_raw",

	"/sys/bus/iio/devices/iio:device1/in_anglvel_x_raw",
	"/sys/bus/iio/devices/iio:device1/in_anglvel_y_raw",
	"/sys/bus/iio/devices/iio:device1/in_anglvel_z_raw",

	"/sys/bus/iio/devices/iio:device1/in_magn_x_raw",
	"/sys/bus/iio/devices/iio:device1/in_magn_y_raw",
	"/sys/bus/iio/devices/iio:device1/in_magn_z_raw",

};


int16 imu_get_raw(const char *path)
{
	char str[20] = {0};
	file_read_string(path, str);
	return atoi(str);
}


