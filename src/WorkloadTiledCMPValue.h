#ifndef _WORKLOAD_TILED_CMP_VALUE_H_
#define _WORKLOAD_TILED_CMP_VALUE_H_

#include "WorkloadTiledCMP.h"

class WorkloadTiledCMPValue : public WorkloadTiledCMP {
public:
    // Constructors
    WorkloadTiledCMPValue();

    // Destructor
    ~WorkloadTiledCMPValue();

    // Public Methods
    bool openTraceFile();
    vector< Packet* > readTrace();
    void skipTraceFile();

    void printStats(ostream& out) const;
    void print(ostream& out) const;
    void printTrace(ostream& out, const PktTraceValue & pkt);

};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadTiledCMPValue& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadTiledCMPValue& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

void config_tiledCMP_network();

#endif // #ifndef _WORKLOAD_TILED_CMP_VALUE_H_
