

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "modbus.h"
#include "Acq.h"

CBEVENT cbevent;

char *event_list[] = {\
    "event_number",\
    "event_type",\
    "event_datetime",\
    "event_millisec",\
    "contact_duty_A",\
    "contact_duty_B",\
    "contact_duty_C",\
    "tc1_peak_curr",\
    "tc1_curr_flow",\
    "tc2_peak_curr",\
    "tc2_curr_flow",\
    "cc_peak_curr",\
    "cc_curr_flow",\
    "contact_op_time_A",\
    "contact_op_time_B",\
    "block_close_time_A",\
    "block_close_time_B",\
    "block_close_time_C"\
};

void printCBEvent(CBEVENT evnt)
{
	printf("\
		event_number:%d\n\
		event_type:%d\n\
		event_datetime%d\n\
		event_millisec:%d\n\
		contact_duty_A:%f\n\
		evnt.contact_duty_B:%f\n\
		evnt.contact_duty_C:%f\n\
		tc1_peak_curr:%f\n\
		tc1_curr_flow:%f\n\
		tc2_peak_curr:%f\n\
		tc2_curr_flow:%f\n\
		cc_peak_curr:%f\n\
		cc_curr_flow:%f\n\
		contact_op_time_A:%f\n\
		contact_op_time_B:%f\n\
		block_close_time_A:%f\n\
		block_close_time_B:%f\n\
		block_close_time_C:%f\n\
		",\
		evnt.event_number, \
		evnt.event_type, \
		evnt.event_datetime, \
		evnt.event_millisec, \
		evnt.contact_duty_A,\
		evnt.contact_duty_B,\
		evnt.contact_duty_C,\
		evnt.tc1_peak_curr,\
		evnt.tc1_curr_flow,\
		evnt.tc2_peak_curr,\
		evnt.tc2_curr_flow,\
		evnt.cc_peak_curr,\
		evnt.cc_curr_flow,\
		evnt.contact_op_time_A,\
		evnt.contact_op_time_B,\
		evnt.block_close_time_A,\
		evnt.block_close_time_B,\
		evnt.block_close_time_C\
		);
}

int main(int argc, char **argv)
{

	u16 buf[3064];

	int i=0;
	int flag_recs = 0;

   //while(1)
   {
	sleep(2);
	//testAcqConnection();
	//readAll();

	if(openAcqBus() == 0)
	{
		//readSingleBlock(64, 1300, buf);

		// readMultiBlock(64, 36, 1300, buf);
		// for(i=0;i<64;i++)
		// {
		// 	printf("%04X", buf[i]);
		// }

		//readSetsA();
		flag_recs = checkRecordFlags();
		printf("Flag : %04X\n", flag_recs);
		writeDateTime(34, 0, 23, 5, 20, 1); // Minute, (Hour - 7)(UTC + 9), Day, (Month - 1), Year

		readMultiBlock(34, 1, 1200, (u16 *)&cbevent);
		printCBEvent(cbevent);

		//readAll();
		closeModbus();
	}
   }

}
