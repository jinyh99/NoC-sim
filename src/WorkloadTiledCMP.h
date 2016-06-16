#ifndef _WORKLOAD_TILED_CMP_H_
#define _WORKLOAD_TILED_CMP_H_

#include "Workload.h"
#include "WorkloadGEMSType.h"

class Packet;

class WorkloadTiledCMP : public WorkloadTrace {
public:
    // Constructors
    WorkloadTiledCMP();

    // Destructor
    ~WorkloadTiledCMP();

    // Public Methods
    void buildConfigStr();
    bool openTraceFile();
    vector< Packet* > readTrace();
    void skipTraceFile();
    void closeTraceFile();

    void printStats(ostream& out) const;
    void print(ostream& out) const;
    void printTrace(ostream& out, const PktTrace & pkt);

    int getTileID(int mach_type, int mach_num);

};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadTiledCMP& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadTiledCMP& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

void config_tiledCMP_network();

#endif // #ifndef _WORKLOAD_TILED_CMP_H_
