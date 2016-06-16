#ifndef _TOPOLOGY_HMESH_H_
#define _TOPOLOGY_HMESH_H_

#include "Topology2DMesh.h"

/**
 * 1-express level Hierarchical Mesh
 * References: 
 *   S. Hauck, et al.
 *   Mesh routing topologies for multi-FPGA systems,
 *   IEEE TVLSI, Vol 6, No. 3, pp.400-408 1998
 *
 *   Hangsheng Wang, et al
 *   A Technology-aware and Energy-oriented Topology Exploration for On-chip Networks
 *   DATE 2005
 * 
 * Router physical channels
 *  0 .. 3: regular channels
 *  4 .. 7: 1-level express channels
 *  8 ..  : external channels
 */

class TopologyHMesh : public Topology2DMesh {
public:
    // Constructors
    TopologyHMesh();

    // Destructor
    ~TopologyHMesh();

    // Public Methods
    void buildTopology();
    int getMinHopCount(int src_router_id, int dst_router_id);

    int getLinkLevel(Router* p_router, int out_pc);
    int bypass_hop() const { return m_bypass_hop; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    pair< int, int > getNextConn(int cur_router_id, int cur_out_pc);
    pair< int, int > getPrevConn(int cur_router_id, int cur_in_pc);

private:
    int m_bypass_hop;	// bypass hop count
};

// Output operator declaration
ostream& operator<<(ostream& out, const TopologyHMesh& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TopologyHMesh& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
