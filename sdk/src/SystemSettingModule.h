/*
 * SystemSettingModule.h
 *
 *  Created on: May 27, 2015
 *      Author: root
 */

#ifndef SYSTEMSETTINGMODULE_H_
#define SYSTEMSETTINGMODULE_H_

void SSM_DisplayOsTime(void);
void SSM_InitIPSetting(void);
U32 SSM_GetKernelIpAddr(void);
S32 SSM_UpdateIpAddr(U32 u32IPAddr);
S32 SSM_UpdateSubnet(U32 u32SubNet);
S32 SSM_UpdateGateway(U32 u32GateWay);
int SSM_LoadNetworkInfo(sqlite3* DBName,dsNetworkInfo* NetworkData);


#endif /* SYSTEMSETTINGMODULE_H_ */
