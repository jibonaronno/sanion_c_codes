#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "src/i2c-dev.h"

// LCD TEST PIN
#define HMI_LCDPOWER     0x2000000000
#define HMI_BACKLIGHT     0x8000000000
#define HMI_BUZZER 		0x4000000000

unsigned long long int led_iedsts =0 ;


int e1=0;
int dev_btn=0, dev_led1 = 0, dev_led2 = 0, dev_iedled = 0;
unsigned int led1to32_status=0, led33to64_status=0;
unsigned int key=0xffffff, key_old=0xffffff;

void HMI_KeyLed_Init()
{
	int addr_chip0,addr_chip1, addr_chip2, addr_chip3, i2c_dev, i=0;

	char filename[30] = {0,};
	long funcs;

	int i2cbus = 3 ;	/* /dev/i2c-3 : For Sanion Board */

	//PCA9698DGG
	addr_chip0 = 0x20 ; /* For Sanion USER LED 0 ~15 #0 I2C Address */
	addr_chip1 = 0x21 ; /* For Sanion USER LED 16~31 #1 I2C Address */
	addr_chip2 = 0x22 ; /* For Sanion LED 10ea #2 I2C Address */
	addr_chip3 = 0x23 ; /* For Sanion BUTTON 0~17 #3 I2C Address */


	/* fprintf(stderr,"ADDR : %4X  DATA : %2X \n", daddress, value); */

	sprintf(filename,"/dev/i2c-%d",i2cbus);

	if ((dev_led1 = open(filename,O_RDWR)) < 0)
	{
		e1 = errno;
		if(e1 == ENOENT )
		{
			fprintf(stderr,"Error: Could not open file `%s', : %s\n",
					 filename,strerror(ENOENT));
		}
		if (e1 != ENOENT)
		{
			  fprintf(stderr,"Error: Could not open file `%s' : %s\n",
						 filename,strerror(e1));
			  if(e1 == EACCES)
				fprintf(stderr,"Run as root?\n");
		}
		return;	// Error Exit
	}
	if ((dev_led2 = open(filename,O_RDWR)) < 0)
	{
		e1 = errno;
		if(e1 == ENOENT )
		{
			fprintf(stderr,"Error: Could not open file `%s', : %s\n",
					 filename,strerror(ENOENT));
		}
		if (e1 != ENOENT)
		{
			  fprintf(stderr,"Error: Could not open file `%s' : %s\n",
						 filename,strerror(e1));
			  if(e1 == EACCES)
				fprintf(stderr,"Run as root?\n");
		}
		return;	// Error Exit
	}
	if ((dev_iedled = open(filename,O_RDWR)) < 0)
	{
		e1 = errno;
		if(e1 == ENOENT )
		{
			fprintf(stderr,"Error: Could not open file `%s', : %s\n",
					 filename,strerror(ENOENT));
		}
		if (e1 != ENOENT)
		{
			  fprintf(stderr,"Error: Could not open file `%s' : %s\n",
						 filename,strerror(e1));
			  if(e1 == EACCES)
				fprintf(stderr,"Run as root?\n");
		}
		return;	// Error Exit
	}
	if ((dev_btn = open(filename,O_RDWR)) < 0)
	{
		e1 = errno;
		if(e1 == ENOENT )
		{
			fprintf(stderr,"Error: Could not open file `%s', : %s\n",
					 filename,strerror(ENOENT));
		}
		if (e1 != ENOENT)
		{
			  fprintf(stderr,"Error: Could not open file `%s' : %s\n",
						 filename,strerror(e1));
			  if(e1 == EACCES)
				fprintf(stderr,"Run as root?\n");
		}
		return;	// Error Exit
	}


	/* check adapter functionality */
	if (ioctl(dev_led1,I2C_FUNCS,&funcs) < 0)
	{
		fprintf(stderr,
		        "Error: Could not get the adapter functionality matrix: %s\n",
		        strerror(errno));

		close(i2c_dev);
		return;
	}

	/* use FORCE so that we can write registers even when a driver is also running */

	if (ioctl(dev_led1,I2C_SLAVE_FORCE,addr_chip0) < 0)
	{
		fprintf(stderr,"Error: Could not set address to %d: %s\n",addr_chip0,
		        strerror(errno));

		close(i2c_dev);
		return;
	}

	if (ioctl(dev_led2,I2C_SLAVE_FORCE,addr_chip1) < 0)
	{
		fprintf(stderr,"Error: Could not set address to %d: %s\n",addr_chip1,
		        strerror(errno));

		close(i2c_dev);
		return;
	}

	if (ioctl(dev_iedled,I2C_SLAVE_FORCE,addr_chip2) < 0)
	{
		fprintf(stderr,"Error: Could not set address to %d: %s\n",addr_chip2,
		        strerror(errno));

		close(i2c_dev);
		return;
	}

	if (ioctl(dev_btn,I2C_SLAVE_FORCE,addr_chip3) < 0)
	{
		fprintf(stderr,"Error: Could not set address to %d: %s\n",addr_chip2,
		        strerror(errno));

		close(i2c_dev);
		return;
	}

	e1 = 0;

	/*
	* sootoo23 - Chip Configuration
	* chip bank 5
	* Register 18~1C : IO Configration (R or W)
	*/
	for(i=0; i<5; i++)
	{
		i2c_smbus_write_byte_data(dev_led1, 0x18+i, 0x0);
		i2c_smbus_write_byte_data(dev_led2, 0x18+i, 0x0);
		i2c_smbus_write_byte_data(dev_iedled, 0x18+i, 0x0);
	}

	//Buzzer & Backlight Enable
	//Reg 8: out register > 8+4: Bank 5 Data Write
	i2c_smbus_write_byte_data(dev_iedled, 0xC, 0xC0);
	led_iedsts |= HMI_BACKLIGHT;
}

void main()
{
	HMI_KeyLed_Init();
	
	int i=0 ;
	unsigned char buf[5] = {0,};

	while(1)
	{
		//test
		led_iedsts |= HMI_BUZZER;
		led_iedsts |= (HMI_LCDPOWER);

		memcpy(buf, &led_iedsts, sizeof(buf));
		for(i=0; i<5; i++)
		{
			//addr 8~C Bank: output register
			i2c_smbus_write_byte_data(dev_iedled, 0x8+i, buf[i]);
		}

		usleep(10); // 100us
		//test
		led_iedsts &= (~HMI_LCDPOWER);

		memcpy(buf, &led_iedsts, sizeof(buf));
		for(i=0; i<5; i++)
		{
			//addr 8~C Bank: output register
			i2c_smbus_write_byte_data(dev_iedled, 0x8+i, buf[i]);
		}

		usleep(1000*1000); // 1000ms
	}
}
