#ifndef _ROUTING_FAT_TREE_H_
#define _ROUTING_FAT_TREE_H_

#include "noc.h"
#include "Routing.h"

class TopologyFatTree;

class RoutingFatTree : public Routing {
public:
    // Constructors
    RoutingFatTree();

    // Destructor
    ~RoutingFatTree();

    // Public Methods
    void init();
    int selectOutPC(Router* p_cur_router, int cur_vc, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    unsigned int selectUpOutVC(unsigned int cur_router_id, vector< int > & up_out_pc_vec) const;

private:
    vector< stream* > m_stream_UpSelect_vec;	// for random selection for up routers
    TopologyFatTree* m_topology_ftree;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingFatTree& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingFatTree& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
