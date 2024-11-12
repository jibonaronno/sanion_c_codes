/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED MPU
 ******************************************************************************
    FILE NAME   : ur_gpio.h
    AUTHOR      : sootoo23
    DATE        : 2017-03-27
    REVISION    : V1.00
    DESCRIPTION : UR-IED GPIO Driver
 ******************************************************************************
    HISTORY     :
    2017-03-27 �ʾ��ۼ�
 ******************************************************************************/

typedef enum
{
	GPIO_READ,
	GPIO_WRITE
}GPIO_IOCTLTYPE;

//sootoo23 - GPIO PIN
//EX) GPIO 6_7 => (GPIO 32pin * (6EA - 1) + Num7 Pin)
#define GPIO_HMI_RESET  ((6*16)+6)  //HMI I2c Chip Reset
#define GPIO_ETH_RESET  ((6*16)+7)  //Ethernet Reset
#define GPIO_APP_START  (((2-1)*32)+29) //App Start Signal (WDT) GPIO 2_29
#define GPIO_WDT_SIGNAL (((5-1)*32)+4)  //Watch Dog Signal (Strobe Input)
#define GPIO_PHYCHIP_RESET (((5-1)*32)+7) 
#define GPIO_FPGA_N_RESET (203)		//FPGA nReset GPIO 7_11

#define GPIO_LED  216
#define GPIO_OUTPUT 0
#define GPIO_INPUT 1
#define GPIO_HIGH  1
#define GPIO_LOW  0
 
#define GPIO_NONE  "none"
#define GPIO_FALLING "falling"
#define GPIO_RISING "rising"
#define GPIO_BOTH  "both"

int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_get_dir(unsigned int gpio, unsigned int *dir);
int gpio_set_dir(unsigned int gpio, unsigned int dir, unsigned int val);
int gpio_get_val(unsigned int gpio, unsigned int *val);
int gpio_set_val(unsigned int gpio, unsigned int val);
int gpio_open(unsigned int gpio);
int gpio_close(int fd);
int gpio_read(int fd, unsigned int *val);

void gpio_ioctl(GPIO_IOCTLTYPE type, unsigned int gpio, unsigned int val);


