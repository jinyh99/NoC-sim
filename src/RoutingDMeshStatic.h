#ifndef _ROUTING_DMESH_STATIC_H_
#define _ROUTING_DMESH_STATIC_H_

#include "noc.h"
#include "Routing.h"

class TopologyDMesh;

class RoutingDMeshStatic : public Routing {
public:
    // Constructors
    RoutingDMeshStatic();

    // Destructor
    ~RoutingDMeshStatic();

    // Public Methods
    void init();
    int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    TopologyDMesh* m_topology_dmesh;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingDMeshStatic& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingDMeshStatic& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _ROUTING_DMESH_STATIC_H_
