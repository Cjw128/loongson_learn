#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
int flash_write_u32(const char* filename, uint32_t value);
int flash_read_u32(const char* filename, uint32_t* value);  
int flash_write_float(const char* filename, float value);
int flash_read_float(const char* filename, float* value);
#endif