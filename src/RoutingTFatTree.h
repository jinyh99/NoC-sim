#ifndef _ROUTING_TFAT_TREE_H_
#define _ROUTING_TFAT_TREE_H_

#include "noc.h"
#include "Routing.h"

class TopologyFatTree;

class RoutingTFatTree : public Routing {
public:
    // Constructors
    RoutingTFatTree();

    // Destructor
    ~RoutingTFatTree();

    // Public Methods
    void init();
    int selectOutPC(Router* p_cur_router, int cur_vc, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    unsigned int selectOutVC(unsigned int cur_router_id, vector< int > & up_out_pc_vec) const;
    unsigned int selectDownRouter(unsigned int cur_router_id,
                                vector< pair<int, int> > & DownRouter_vec) const;
private:
    TopologyFatTree* m_topology_tfattree;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingTFatTree& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingTFatTree& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
