#ifndef _ROUTING_TORUS_STATIC_H_
#define _ROUTING_TORUS_STATIC_H_

/**
 * Deadlock-Free Message Routing in Multiprocessor Interconnection Networks,
 * Dally and Seitz,
 * TC Vol.36 No.5 May 1987
 *
 * In each dimension i, a message is routed in that dimension until it reaches
 * a node whose subscript matches the destination address in the i-th position.
 * The message is routed on the high channel(VC) if the i-th digit of the present
 * node's address. Otherwise, the message is routed on the low channel(VC).
 *
 * The number of VCs should be equal to or greater than 2.
 * We categorize low VCs as 0 ~ <# VCs>/2-1, and high VCs as <# VCs>/2 ~ <# VCs>-1.
 */

#include "noc.h"
#include "Routing.h"

class Topology2DTorus;

class RoutingTorusStatic : public Routing {
public:
    // Constructors
    RoutingTorusStatic();

    // Destructor
    ~RoutingTorusStatic();

    // Public Methods
    void init();
    int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);
    bool isOutVCDeadlockFree(int out_vc, int in_pc, int in_vc, int out_pc, Router* cur_router, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    Topology2DTorus* m_topology_torus;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingTorusStatic& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingTorusStatic& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
