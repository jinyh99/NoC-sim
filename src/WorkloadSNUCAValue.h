#ifndef _WORKLOAD_SNUCA_VALUE_H_
#define _WORKLOAD_SNUCA_VALUE_H_

#include "WorkloadTiledCMPValue.h"

// SNUCA Cache coherence workload

class WorkloadSNUCAValue : public WorkloadTiledCMPValue {
public:
    // Constructors
    WorkloadSNUCAValue();

    // Destructor
    ~WorkloadSNUCAValue();

    // Public Methods
    vector< Packet* > readTrace();		// overloaded

    void printStats(ostream& out) const;
    void print(ostream& out) const;
};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadSNUCAValue& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadSNUCAValue& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _WORKLOAD_SNUCA_H_
