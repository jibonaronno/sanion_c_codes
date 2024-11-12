/******************************************************************************
 **                     Copyright(C) 2017 SANION Co. Ltd.                    **
 **                  --------------------------------------                  **
 **                              WWW.SANION.COM                              **
 ******************************************************************************
    PROJECT     : UR-IED
 ******************************************************************************
    FILE NAME   : MSManger.c
    AUTHOR      : sootoo23
    DATE        : 2017.04.05
    REVISION    : V1.00
    DESCRIPTION : Measurement Manager & Calibration..
 ******************************************************************************
    HISTORY     :
        2017-04-05 Create
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "VT100.h"
#include "Common.h"
#include "SharedMemMgr.h"
#include "DBManager.h"
#include "UR_SDK.h"
#include "MSManager.h"

#define HELP_RESET_FACTOR "\
USAGE: \n\
RESET FACTOR \n"

#define HELP_FACTOR_DISPLAY "\
USAGE: FACTOR [PT/OCT/BCT] [CH] [Target]\n\
Example: \n\
	FACTOR PT : Display all factor of PT \n\
	FACTOR OCT 3 1.3043 : Set Ch 3 of CT factor to 1.3043\n"

#define HELP_TDFACTOR_SET "\
Description: Setting the factor of TD using 2 points calibration. 4mA and 20mA\n\
USAGE: \n\
TDF [SET/RESET] [CH] \n\
Example:\n\
    TDF SET 12\n\
    TDF RESET 3\n\
    TDF RESET ALL\n"

void* MSM_Factor(int argc, void* argv[])
{
	U16 i=0, u16CPTType=0, u16MaxCount = 32;
	U16 u16SelCH;
	F32 f32FactorData[3][32] = {0.0,};
	F32 fTarget;
	
	if( argc == 0 || argc == 2)
	{
		printf("Invalid argument count\n");
		printf(HELP_FACTOR_DISPLAY);
		return 0;
	}
	else if( argc == 1)
	{
		if(strncasecmp(argv[0], "PT", sizeof("PT")) == 0)
		{
			u16CPTType = 2;
			u16MaxCount = MAX_OCT_COUNT;
			
			i =  u16MaxCount * sizeof(F32);
			memcpy( &(f32FactorData[0][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32PhaseFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[1][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32GainFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[2][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32OffsetFactor[u16CPTType]), i);	
		}
		else if(strncasecmp(argv[0], "OCT", sizeof("OCT")) == 0)
		{
			u16CPTType = 0;
			u16MaxCount = MAX_OCT_COUNT;
			
			i =  u16MaxCount * sizeof(F32);
			memcpy( &(f32FactorData[0][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32PhaseFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[1][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32GainFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[2][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32OffsetFactor[u16CPTType]), i);	
		}
		else if(strncasecmp(argv[0], "BCT", sizeof("BCT")) == 0)
		{
			u16CPTType = 1;
			u16MaxCount = MAX_BCT_COUNT;
			
			i =  u16MaxCount * sizeof(F32);
			memcpy( &(f32FactorData[0][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32PhaseFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[1][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32GainFactor[u16CPTType]), i);
			memcpy( &(f32FactorData[2][0]), (F32*)(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32OffsetFactor[u16CPTType]), i);	
		}
		else
		{
			printf(HELP_FACTOR_DISPLAY);
			return 0;
		}

		printf("---------------------------------------------------\n");
		printf("             %s Factor\n", (char*)argv[0]);
		printf("       |   PHASE |  GAIN  | OFFSET\n");
		printf("---------------------------------------------------");

		for(i=0; i<u16MaxCount; i++)
			printf("\n CH %02d |  %5.4f   %5.4f  %6.1fuV", i+1, f32FactorData[0][i], f32FactorData[1][i], f32FactorData[2][i]);
		printf("\n==================================================\n\n");
	}
	else if( argc == 3)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;

		if(strncasecmp(argv[0], "PT", sizeof("PT")) == 0)
		{
			u16CPTType = 2;
			u16SelCH = atoi((char*)argv[1]);

			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_PT;
		}
		else if(strncasecmp(argv[0], "OCT", sizeof("OCT")) == 0)
		{
			u16CPTType = 0;
			u16SelCH = atoi((char*)argv[1]);

			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_OCT;
		}
		else if(strncasecmp(argv[0], "BCT", sizeof("BCT")) == 0)
		{
			u16CPTType = 1;
			u16SelCH = atoi((char*)argv[1]);

			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_BCT;
		}
		else
		{
			printf(HELP_FACTOR_DISPLAY);
			return 0;
		}
	
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = u16SelCH-1;
		printf("\nSet %s Channel %d from %.4f to ",argv[0], u16SelCH, g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32GainFactor[u16CPTType][u16SelCH-1]);
		fTarget = atof((char*)argv[2]);
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.f32GainFactor[u16CPTType][u16SelCH-1] = fTarget;
		printf("%.4f\n",fTarget);
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue =fTarget;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_USFIX;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_GAIN;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;
	}
}

void* MSM_SetAutoOCTCurrFactor(int argc, void* argv[])
{
	U16 i=0;
	U32 u32WaitCnt = 0;
	F32 fTarget = 5.0;
	
	if(argc < 2)
	{
		VT100_scrclr();
		VT100_goto(0,40);

		printf("\n\n");
		printf("===+===========+=============================\n");
		printf("[CH]  [ Type ]   [  MAG  PHASE  ]\n");
		printf("===+===========+=============================\n");


		for(i=0; i<MAX_OCT_COUNT; i++)
		{
			printf("%02d |    C T    | %7.3f %3.2f?\n", i+1, g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Imag[i],
														g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[i]);
		}
		
		printf("------+-------------------------------------------\n");
		printf(" CMD  | AIF [CH] [Target A]\n");
		printf("      | CH : ALL(ëª¨ë“  ?„ë¥˜ ì±„ë„) or 1~32\n");
		printf("      | Target A : 5A or other value \n");
		printf("------+-------------------------------------------\n");
		return 0;
	}
	
	// Command Clear
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;

	/*
	*    Calibration Mode & Cmd set
	*/
	if(strncasecmp(((char*)argv[0]), "ALL", sizeof("ALL")) == 0)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x000001ff;
	}
	else if(atoi((char*)argv[0]) > MAX_OCT_COUNT)
	{
		printf("[ERR] Invalid channel!\n");
		return 0;
	}
	else
	{
		U16 u16SelCH = atoi((char*)argv[0]);
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = pow(2, u16SelCH-1);
	}
	
	fTarget = atof((char*)argv[1]);
	if( fTarget > 0.1 && fTarget < 20.0)
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue = fTarget;
	else
	{
		printf("[ERR] Wrong range (0.1A ~ 20.0A)\n");
		return 0;
	}

	// Auto Calibration cmd
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_AUTO;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_GAIN;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_OCT;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;


	/*
	*   Calibration ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?. routine
	*/
	printf("\n\tAuto Calibrating %fA... please wait a minute. [CMD:0x%x] \n OPER >> ",g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue, g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return 0;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("\n\tDone.\n");

			return 0;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}
	
	return 0;
}

void* MSM_SetAutoBCTCurrFactor(int argc, void* argv[])
{
	U16 i=0;
	U32 u32WaitCnt = 0;
	F32 fTarget = 5.0;
	
	if(argc < 2)
	{
		VT100_scrclr();
		VT100_goto(0,40);

		printf("\n\n");
		printf("===+===========+=============================\n");
		printf("[CH]  [ Type ]   [  MAG  PHASE  ]\n");
		printf("===+===========+=============================\n");

		for(i=0; i<MAX_BCT_COUNT; i++)
		{
			printf("%02d |    C T    | %7.3f %3.2f?\n", i+1, g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Imag[i],
														g_pAISharedMem->m_dsMeasValueData.m_dsOCTDataInfo.m_f32Ang[i]);
		}
		
		printf("------+-------------------------------------------\n");
		printf(" CMD  | AIF [CH] [Target A]\n");
		printf("      | CH : ALL(ëª¨ë“  ?„ë¥˜ ì±„ë„) or 1~32\n");
		printf("      | Target A : 5A or other value \n");
		printf("------+-------------------------------------------\n");
		return 0;
	}
	
	// Command Clear
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;

	/*
	*    Calibration Mode & Cmd set
	*/
	if(strncasecmp(((char*)argv[0]), "ALL", sizeof("ALL")) == 0)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x0000003f;
	}
	else if(atoi((char*)argv[0]) > MAX_BCT_COUNT)
	{
		printf("[ERR] Invalid Channel !\n");
		return 0;
	}
	else
	{
		U16 u16SelCH = atoi((char*)argv[0]);
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = pow(2, u16SelCH-1);
	}
	
	fTarget = atof((char*)argv[1]);
	if( fTarget > 0.1 && fTarget < 20.0)
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue = fTarget;
	else
	{
		printf("[ERR] Wrong Range (0.1A ~ 20.0A) \n");
		return 0;
	}

	// Auto Calibration cmd
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_AUTO;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_GAIN;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_BCT;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;


	/*
	*   Calibration ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?. routine
	*/
	printf("\n\tAuto Calibrating %fA... please wait a minute. [CMD:0x%x] \n OPER >> ",g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue, g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return 0;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("\n\tDone.\n");

			return 0;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}
	
	return 0;
}

void* MSM_SetAutoVoltFactor(int argc, void* argv[])
{
	U16 i=0;
	U32 u32WaitCnt = 0;
	F32 fTarget = 63.5;
	
	if(argc < 2)
	{
		VT100_scrclr();
		VT100_goto(0,40);

		printf("\n\n");
		printf("===+===========+=============================\n");
		printf("[CH]  [ Type ]   [  MAG  PHASE  ]\n");
		printf("===+===========+=============================\n");

		for(i=0; i<MAX_PT_COUNT; i++)
		{
			printf("%02d |    P T    | %7.3f %3.2f?\n", i+1, g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Imag[i],
												 		g_pAISharedMem->m_dsMeasValueData.m_dsPTDataInfo.m_f32Ang[i]);
		}
		
		printf("------+-------------------------------------------\n");
		printf(" CMD  | AVF [CH] [Target V]\n");
		printf("      | CH : ALL(ëª¨ë“  ?„ì•• ì±„ë„) or 1~32\n");
		printf("      | Target V : 63.5V or other value \n");
		printf("------+-------------------------------------------\n");
		return 0;
	}

	// Command Clear
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;

	/*
	*    Calibration Mode & Cmd set
	*/
	if(strncasecmp(((char*)argv[0]), "ALL", sizeof("ALL")) == 0)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x00000007;
	}
	else if(atoi((char*)argv[0]) > MAX_PT_COUNT)
	{
		printf("[ERR] Invalid channel!\n");
		return 0;
	}
	else
	{
		U16 u16SelCH = atoi((char*)argv[0]);
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = pow(2, u16SelCH-1);
	}
	fTarget = atof((char*)argv[1]);
	if( fTarget > 6.35 && fTarget < 220.0)
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue = fTarget;
	else
	{
		printf("[ERR] Wrong range (6.35V ~ 220.0V)\n");
		return 0;
	}

	// Auto Calibration cmd
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_AUTO;


	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_GAIN;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_PT;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;


	/*
	*   Calibration ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?. routine
	*/
	printf("\n\tAuto Calibrating %fV... please wait a minute. [CMD:0x%x] \n OPER >> ",g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.fTargetValue, g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return 0;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("\n\tDone.\n");

			return 0;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}
	
	return 0;
}

void* MSM_SetACDAutoFactor(int argc, void* argv[])
{
	U16 i=0, j=0;
	U16 u16CalibType;
	U16 u16PassCode;
	U32 u32WaitCnt = 0;
	
	if(argc <2)
	{
		printf("\nADCADJ [0:ì´ˆê¸°?? 1:ADC Offset ì¸¡ì • ë°?ë°˜ì˜] [PassCode]\n ");
		return 0;
	}
	else 
	{
		u16CalibType = strtoul(argv[0], NULL, 10);
		u16PassCode = strtoul(argv[1], NULL, 10);
	}
	
	if(u16PassCode == CON_PASSCODE && u16CalibType == 1)
	{
		MSM_SetOffsetAutoFactor(0);
		MSM_SetOffsetAutoFactor(1);
		MSM_SetOffsetAutoFactor(2);
	}
	else if(u16PassCode == CON_PASSCODE && u16CalibType == 0)
	{
		MSM_ResetOffsetAutoFactor();
	}
	else
	{
		printf("?˜ëª»??PassCode?…ë‹ˆ?? \n");
		return 0;
	}
	
	return 0;
}

void MSM_SetOffsetAutoFactor(U8 u8CTPT)
{
	U32 u32WaitCnt = 0;
	
	// Command Clear
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;

	// Auto Calibration cmd
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_AUTO;
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_DCOFFSET;
	if(u8CTPT == 1)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x000001ff;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_OCT;
	}
	else if(u8CTPT == 2)
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x0000003f;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_BCT;
	}
	else
	{
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32ChannelSelect = 0x00000007;
		g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_SEL_PT;
	}
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;

	/*
	*   Calibration ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?. routine
	*/
	printf("\n\tAuto Calibration ... please wait a minute. [CMD:0x%x] \n OPER >> ", g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("\n\tDone.\n");
			return;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}
	return;
}

void* MSM_FactorReset(void)
{
	U32 u32WaitCnt = 0;

	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_RESET;
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;
	printf("Reset factor ... please wait a minute. [CMD:0x%x] \n >> ", g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return 0;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			F32 f32FactorVal = 0.0;
			U16 i=0;

			// Factor value check routine 
			printf("value check ....\n");
			
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("Factor Reset Done.\n\n");
			return 0;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}

	return 0;
}


void MSM_ResetOffsetAutoFactor(void)
{
	U32 u32WaitCnt = 0;
	
	// Command Clear
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD = 0;
	// Auto Calibration cmd
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_ACT_RESET;
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD |= DEFS_CALIBCMD_MODE_DCOFFSET;
	g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_SET_BY_MPU;

	/*
	*   Calibration ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿?. routine
	*/
	printf("\n\tReset Offset ... please wait a minute. [CMD:0x%x] \n OPER >> ", g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32CalibrationCMD);

	while(1)
	{
		if(u32WaitCnt > 3000) // 3sec
		{
			printf("\n\t[ERR] %d: Timeout!\n", __LINE__);
			return;
		}
	
		if(g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag == DATA_SET_BY_DSP)
		{
			g_pAISharedMem->m_dsMeasValueData.m_dsMeasCalibration.u32HFlag = DATA_READ_BY_MPU;
			printf("\n\tDone.\n");
			return;
		}
		else
		{
			u32WaitCnt++;
			printf(".");
			fflush(stdout);
			usleep(1000);
		}
	}
	return;
}

#define TD_STRING_LENGTH 32
#define TD_MODE_FIRST_POINT 1
#define TD_MODE_SECOND_POINT 2

void* Measurment_SetTDFactor(int argc, void* argv[])
{
	U8 au8Name[TD_STRING_LENGTH];
	U8 au8Data[TD_STRING_LENGTH];
	U8 u8TDChannel, i;

	if(argc < 1)
	{
		printf("Invalid argument count\n");
		printf(HELP_TDFACTOR_SET);
		return 0;
	}
	else if(argc == 2) //argv[0] RESET/SET argv[1] channel
	{
		strcpy(au8Data, (char*)argv[0]);
		if(strncasecmp(au8Data, "SET", sizeof("SET")) == 0)
		{
			u8TDChannel = atoi(argv[1]);

			printf("Put 4mA to TD ch.%d, then press any key\n", u8TDChannel);
			fgets(au8Data, TD_STRING_LENGTH, stdin);

			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32Mode= TD_MODE_FIRST_POINT;
			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32ChannelSelect	= u8TDChannel; 
			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32HFlag = DATA_SET_BY_MPU;

			while(g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32HFlag == DATA_SET_BY_MPU);
	
			printf("Put 20mA to TD ch.%d, then press any key\n", u8TDChannel);
			fgets(au8Data, TD_STRING_LENGTH, stdin);

			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32Mode= TD_MODE_SECOND_POINT;
			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32ChannelSelect	= u8TDChannel; 
			g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32HFlag = DATA_SET_BY_MPU;

			while(g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.u32HFlag == DATA_SET_BY_MPU);

		}
		else if(strncasecmp(au8Data, "RESET", sizeof("RESET")) == 0)
		{
			if( *(char *)argv[1] == 'A' || *(char *)argv[1] == 'a') //RESET ALL
			{
				for(i=0; i<MAX_TD_COUNT; i++)
				{
					g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fGain[i]	= 1.0;
					g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fOffset[i]	= 0.0;
				}
			}
			else	//RESET n channel
			{
				u8TDChannel = atoi(argv[1]);
				if(u8TDChannel < MAX_TD_COUNT)
				{
					g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fGain[u8TDChannel]	= 1.0;
					g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fOffset[u8TDChannel]	= 0.0;
				}
				else
					printf("\nWrong channel (ch:0 ~ ch:7)\n");
			}
						
		}
		else
		{
			printf("\nWrong arguments number: %d\n", argc);
			for(i=0; i<argc; i++)
				printf("string: %s\n", argv[i]);				
		}

		printf("Result gain:%f offset:%f\n", g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fGain[u8TDChannel], g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fOffset[u8TDChannel]);
	}
	else
	{
		printf("\nWrong arguments number: %d\n", argc);
		for(i=0; i<argc; i++)
			printf("String: %s\n", argv[i]);				
	}

}

void* Measurment_PrintTDFactor(void)
{
	U8 i;
	S8 s8String[TD_STRING_LENGTH];
	float fTemp1, fTemp2;

	printf("\n///////////TD Shared Memory Factor/////////////////\n");
	for(i=0; i<MAX_TD_COUNT; i++)
		printf("CH[%d] gain: %f, offset: %f\n",i,g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fGain[i], g_pAISharedMem->m_dsMeasValueData.m_dsTDCalibration.fOffset[i]);
}

