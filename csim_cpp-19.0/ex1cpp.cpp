// C++/CSIM Model of M/M/1 queue

#include "cpp.h"			// class definitions

#define NARS 5000
#define IAR_TM 2.0
#define SRV_TM 1.0

event done("done");			// the event named done
facility f("facility");			// the facility named f
table tbl("resp tms");			// table of response times
qhistogram qtbl("num in sys", 10l);	// qhistogram of number in system
int cnt;				// count of remaining processes

void customer();
void theory();

extern "C" void sim(int, char **);

void sim(int argc, char *argv[])
{
	set_model_name("M/M/1 Queue");
	create("sim");
	cnt = NARS;
	for(int i = 1; i <= NARS; i++) {
		hold(expntl(IAR_TM));	// interarrival interval
		customer();		// generate next customer
		}
	done.wait();			// wait for last customer to depart
	report();			// model report
	theory();
	mdlstat();			// model statistics
}

void customer()				// arriving customer
{
	float t1;

	create("cust");
	t1 = clock;			// record start time
	qtbl.note_entry();		// note arrival
	f.reserve();			// reserve facility
		hold(expntl(SRV_TM));	// service interval
	f.release();			// release facility
	tbl.record(clock - t1);		// record response time
	qtbl.note_exit();		// note departure
	if(--cnt == 0)
		done.set();		// if last customer, set done
}

void theory()				// print theoretical results
{
	float rho, nbar, rtime, tput;

	printf("\n\n\n\t\t\tM/M/1 Theoretical Results\n");

	tput = 1.0/IAR_TM;
	rho = tput*SRV_TM;
	nbar = rho/(1.0 - rho);
	rtime = SRV_TM/(1.0 - rho);

	printf("\n\n");
	printf("\t\tInter-arrival time = %10.3f\n",IAR_TM);
	printf("\t\tService time       = %10.3f\n",SRV_TM);
	printf("\t\tUtilization        = %10.3f\n",rho);
	printf("\t\tThroughput rate    = %10.3f\n",tput);
	printf("\t\tMn nbr at queue    = %10.3f\n",nbar);
	printf("\t\tMn queue length    = %10.3f\n",nbar-rho);
	printf("\t\tResponse time      = %10.3f\n",rtime);
	printf("\t\tTime in queue      = %10.3f\n",rtime - SRV_TM);
}

