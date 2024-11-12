/******************************************************************************
 **                     Copyright(C) 2016 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : wdt_ds1832.c
    AUTHOR      : sootoo23
    DATE        : 2016-11-21
    REVISION    : V1.00
    DESCRIPTION : Watch Dog Timer Driver
 ******************************************************************************
    HISTORY     :
    2016-11-21 sootoo23 - 초안작성
 ******************************************************************************/

#include <stdio.h>
#include <unistd.h>

#include "ur_gpio.h"
#include "global.h"
#include "wdt_ds1832.h"


int is_WdtStart = 0, WdtSts = 0;

void WDT_Init(void)
{	
	usleep(10*1000); // 10msec
		
	//Export & Direction Set
	gpio_export(GPIO_APP_START);
	gpio_set_dir(GPIO_APP_START, GPIO_OUTPUT, 1);
	
	gpio_export(GPIO_WDT_SIGNAL);
	gpio_set_dir(GPIO_WDT_SIGNAL, GPIO_OUTPUT, 0);

	WDT_AppStart();

	WDT_InputSignal();
	//WDT_ThreadExit = 0;
}

void WDT_DeInit(void)
{
	//sootoo23: App Stop -> direction input setting
	gpio_ioctl(GPIO_READ, GPIO_WDT_SIGNAL, 0);

	WDT_AppStop();

	//Un Export
	gpio_unexport(GPIO_APP_START);
	gpio_unexport(GPIO_WDT_SIGNAL);
}

void WDT_Reset(void)
{
	g_u8WDTCtrlFlag = 0;
}

void WDT_AppStart(void)
{
	g_u8WDTCtrlFlag = 1;

	gpio_set_val(GPIO_APP_START, 1);	
}

void WDT_AppStop(void)
{
	gpio_set_val(GPIO_APP_START, 0);

	g_u8WDTCtrlFlag = 0;
}

void WDT_InputSignal(void)
{
	if(g_u8WDTCtrlFlag)
	{	
		if(g_u8WDTSignalFlag)
		{
			gpio_set_val(GPIO_WDT_SIGNAL, 0);
			g_u8WDTSignalFlag = 0;
		}
		else
		{
			gpio_set_val(GPIO_WDT_SIGNAL, 1);
			g_u8WDTSignalFlag = 1;
		}
	}
}

