#ifndef _ROUTING_HIER_MESH_H_
#define _ROUTING_HIER_MESH_H_

#include "noc.h"
#include "Routing.h"

class TopologyHMesh;

class RoutingHMeshStatic : public Routing {
public:
    // Constructors
    RoutingHMeshStatic();

    // Destructor
    ~RoutingHMeshStatic();

    // Public Methods
    void init();
    int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    int selectOutPCAscent(Router* cur_router, unsigned int cur_vc, FlitHead* p_flit);
    int selectOutPCDescent(Router* cur_router, unsigned int cur_vc, FlitHead* p_flit);
    int getOutPCFromDir(int direction, int level) const;

private:
    TopologyHMesh* m_topology_hmesh;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingHMeshStatic& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingHMeshStatic& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
