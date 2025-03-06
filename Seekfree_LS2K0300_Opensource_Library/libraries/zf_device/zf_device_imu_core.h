#ifndef _zf_device_imu_core_h
#define _zf_device_imu_core_h


#include "zf_common_typedef.h"


enum path_index 
{
	ACC_X_RAW,
	ACC_Y_RAW,
	ACC_Z_RAW,
	
	GYRO_X_RAW,
	GYRO_Y_RAW,
	GYRO_Z_RAW,

	MAG_X_RAW,
	MAG_Y_RAW,
	MAG_Z_RAW,
};

extern const char *imu_file_path[];

int16 imu_get_raw(const char *path);

#endif
