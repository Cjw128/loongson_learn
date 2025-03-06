
#include "zf_common_headfile.h"

int main(int, char**) 
{
    ips200_init("/dev/fb0");

    if(uvc_camera_init("/dev/video0") < 0)
    {
        return -1;
    }

    while(1)
    {
        imu963ra_get_acc();
        imu963ra_get_gyro();
        imu963ra_get_mag();


        printf("imu963ra_acc_x  = %d\r\n", imu963ra_acc_x);
        printf("imu963ra_acc_y  = %d\r\n", imu963ra_acc_y);
        printf("imu963ra_acc_z  = %d\r\n", imu963ra_acc_z);

        printf("imu963ra_gyro_x = %d\r\n", imu963ra_gyro_x);
        printf("imu963ra_gyro_y = %d\r\n", imu963ra_gyro_y);
        printf("imu963ra_gyro_z = %d\r\n", imu963ra_gyro_z);

        printf("imu963ra_mag_x = %d\r\n", imu963ra_mag_x);
        printf("imu963ra_mag_y = %d\r\n", imu963ra_mag_y);
        printf("imu963ra_mag_z = %d\r\n", imu963ra_mag_z);

        int16 encoder_left = encoder_get_count("/dev/zf_encoder_left");
        int16 encoder_right = encoder_get_count("/dev/zf_encoder_right");

        printf("zf_encoder_left = %d\r\n", encoder_left);
        printf("zf_encoder_right = %d\r\n", encoder_right);

        
        wait_image_refresh();

        // ips200_show_uint(0,0,12345,5);
        // ips200_show_int(0,16,-12345,5);
        // gpio_set_level("/dev/zf_driver_gpio_beep", 0);
        ips200_show_gray_image(0, 0, rgay_image, 160, 120);

        pwm_set_duty("/dev/zf_device_pwm_servo", 2000);
        pwm_set_duty("/dev/zf_device_pwm_esc_left", 3000);
        pwm_set_duty("/dev/zf_device_pwm_esc_right", 4000);

        
        uint16 adc_value = adc_convert("/sys/bus/iio/devices/iio:device0/in_voltage7_raw");
        printf("adc_value = %d\r\n",adc_value);

        float scale = adc_get_scale("/sys/bus/iio/devices/iio:device0/in_voltage_scale");
        printf("scale = %f\r\n", scale);

        uint16 vol = adc_value * scale * 11;
        printf("vol = %d mv\r\n", vol);


        // for(uint32 i=0;i<120*160;i++)
        // {
        //     printf("%d\r\n",rgay_image[i]);
        // }
        // printf("123\r\n");
        // system_delay_ms(1000);
        // gpio_set_level("/dev/zf_driver_gpio_beep", 1);

        // printf("key_0 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_key_0"));
        // printf("key_1 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_key_1"));
        // printf("key_2 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_key_2"));
        // printf("key_3 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_key_3"));

        // printf("switch_0 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_switch_0"));
        // printf("switch_1 = %d\r\n", gpio_get_level("/dev/zf_driver_gpio_switch_1"));

        system_delay_ms(100);
    }
}