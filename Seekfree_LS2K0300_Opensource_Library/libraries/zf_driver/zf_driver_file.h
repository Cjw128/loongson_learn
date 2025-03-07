#ifndef _zf_driver_file_h
#define _zf_driver_file_h


#include "zf_common_typedef.h"



int8 file_write_dat(const char *path, uint8 value);
int8 file_write_dat(const char *path, uint16 value);
int8 file_write_dat(const char *path, uint32 value);

int8 file_read_dat(const char *path, int8 *ret_value);
int8 file_read_dat(const char *path, uint8 *ret_value);
int8 file_read_dat(const char *path, int16 *ret_value);

int8 file_read_string(const char *path, char *str);

#endif

