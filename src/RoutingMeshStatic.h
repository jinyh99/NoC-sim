#ifndef _ROUTING_MESH_STATIC_H_
#define _ROUTING_MESH_STATIC_H_

#include "noc.h"
#include "Routing.h"

class Topology2DMesh;

class RoutingMeshStatic : public Routing {
public:
    // Constructors
    RoutingMeshStatic();

    // Destructor
    ~RoutingMeshStatic();

    // Public Methods
    void init();
    virtual int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);
    virtual vector<Router*> getPathVector(Router* src_router, Router* dest_router);

    virtual void printStats(ostream& out) const;
    virtual void print(ostream& out) const;

protected:
    Topology2DMesh* m_topology_mesh;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingMeshStatic& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingMeshStatic& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
