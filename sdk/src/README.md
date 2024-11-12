
# Preventive Diagnosis Part
## Development

### Communication with ACQ

LU Unit communicates with ACQ over ModbusTcp continiously at a regular interval. 
Basic functions are in MTrMonitoring.c file. 
Sample function SendModbusTCPReadCommand(...)
This function should only send ModBus commands to ACQ. 
The basic commands should be ModBus specific. 

!! project import !!
clone Git repository
import->Existing....

!! project Setting !!
Properties -> C/C++Build -> Environment

CGT_INSTALL_DIR : CROSS COMPILER DIR
ex) /opt/ti/mcsdk_1_01_00_02/linux-devkit/sysroots/i686-arago-linux/usr/bin

IPC_INSTALL_DIR : IPC DIR
ex) /opt/ti/ipc_1_25_03_15

SYSLINK_INSTAL_DIR : SYSLINK DIR
ex) /opt/ti/syslink_2_21_01_05

PATH : 
${CGT_INSTALL_DIR}: