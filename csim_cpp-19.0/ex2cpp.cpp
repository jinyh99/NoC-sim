// C++/CSIM Model of M/M/1 queue

#include "cpp.h"			// class definitions
#include <stdio.h>

#define NARS 5000
#define IAR_TM 2.0
#define SRV_TM 1.0

event *done;				// the event pointer named done
facility *f;				// the facility pointer named f
table *tbl;					// table pointer of response times
qtable *qtbl;				// qtable of number in system
int cnt;					// count of remaining processes
FILE *fp;

void customer();
void theory();

extern "C" void sim(int, char *[]);

void sim(int argc, char *argv[])
{
	fp = fopen("csim.out", "w");
	set_output_file(fp);
	set_error_file(fp);
	set_trace_file(fp);
	
	set_model_name("M/M/1 Queue");
	create("sim");
	done = new event("done");
	f = new facility("f");
	tbl = new table("resp tms");
	qtbl = new qtable("num in fac", 10L, 0L, 10L);
	cnt = NARS;
	for(int i = 1; i <= NARS; i++) {
		hold(expntl(IAR_TM));	// interarrival interval
		customer();		// generate next customer
		}
	done->wait();			// wait for last customer to depart
	report();			// model report
	theory();
	mdlstat();			// model statistics
}

void customer()				// arriving customer
{
	TIME t1;

	create("cust");
	t1 = clock;			// record start time
	qtbl->note_entry();		// note arrival
	f->reserve();			// reserve facility
		hold(expntl(SRV_TM));	// service interval
	f->release();			// release facility
	tbl->record(clock - t1);		// record response time
	qtbl->note_exit();		// note departure
	if(--cnt == 0)
		done->set();		// if last customer, set done
}

void theory()				// print theoretical results
{
	double rho, nbar, rtime, tput;

	fprintf(fp, "\n\n\n\t\t\tM/M/1 Theoretical Results\n");

	tput = 1.0/IAR_TM;
	rho = tput*SRV_TM;
	nbar = rho/(1.0 - rho);
	rtime = SRV_TM/(1.0 - rho);

	fprintf(fp, "\n\n");
	fprintf(fp, "\t\tInter-arrival time = %10.3f\n",IAR_TM);
	fprintf(fp, "\t\tService time       = %10.3f\n",SRV_TM);
	fprintf(fp, "\t\tUtilization        = %10.3f\n",rho);
	fprintf(fp, "\t\tThroughput rate    = %10.3f\n",tput);
	fprintf(fp, "\t\tMn nbr at queue    = %10.3f\n",nbar);
	fprintf(fp, "\t\tMn queue length    = %10.3f\n",nbar-rho);
	fprintf(fp, "\t\tResponse time      = %10.3f\n",rtime);
	fprintf(fp, "\t\tTime in queue      = %10.3f\n",rtime - SRV_TM);
}

