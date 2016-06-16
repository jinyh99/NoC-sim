#ifndef _ROUTING_FLBFLY_H_
#define _ROUTING_FLBFLY_H_

#include "noc.h"
#include "Routing.h"

class TopologyFlbfly;

class RoutingFlbfly : public Routing {
public:
    // Constructors
    RoutingFlbfly();

    // Destructor
    ~RoutingFlbfly();

    // Public Methods
    void init();
    int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);
    bool isPCAvailable(Router* cur_router, int cur_out_pc);
    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    TopologyFlbfly * m_topology_flbfly;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingFlbfly& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingFlbfly& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _ROUTING_FLBFLY_H_
