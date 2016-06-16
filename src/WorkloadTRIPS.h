#ifndef _WORKLOAD_TRIPS_H_
#define _WORKLOAD_TRIPS_H_

#include "Workload.h"

/**
 * TRIPS workload
 */

class WorkloadTRIPS : public WorkloadTrace {
public:
    // Constructors
    WorkloadTRIPS();

    // Destructor
    ~WorkloadTRIPS();

    // Public Methods
    bool openTraceFile();
    void skipTraceFile() {};
    void closeTraceFile() {};
    vector< Packet* > readTrace();

    void printStats(ostream& out) const;
    void print(ostream& out) const;
};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadTRIPS& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadTRIPS& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

void config_TRIPS_network();

#endif // #ifndef _WORKLOAD_TRIPS_H_
