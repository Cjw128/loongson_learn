#include "zf_driver_gpio.h"
#include "zf_driver_file.h"

void gpio_set_level(const char *path, uint8 dat)
{
    file_write_uint8(path, dat + 0x30);

}



uint8 gpio_get_level(const char *path)
{
    uint8 dat;

    file_read_uint8(path, &dat);

    return dat;
}