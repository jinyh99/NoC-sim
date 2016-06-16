#ifndef _TOPOLOGY_DMESH_H_
#define _TOPOLOGY_DMESH_H_

#include "Topology2DMesh.h"

/**
 * 2 mesh networks
 * Each network is separated to each other.
 *
 * An 8x8 core chip has 2 4x4 router mesh networks.
 *
 *   -- Network-0
 *   C00       C01  C02       C03
 *      \     /        \     /
 *        R00------------R01---------   
 *      /  |  \        /  |  \
 *   C08   |   C09  C10   |   C11
 *         |              |
 *
 *
 *   -- Network-1
 *   C00       C01  C02       C03
 *      \     /        \     /
 *        R16------------R17---------   
 *      /  |  \        /  |  \
 *   C08   |   C09  C10   |   C11
 *         |              |
 */

class TopologyDMesh : public Topology2DMesh {
public:
    // Constructors
    TopologyDMesh();

    // Destructor
    ~TopologyDMesh();

    // Public Methods
    void buildTopology();
    void buildTopology(int num_networks);
    int getMinHopCount(int src_router_id, int dst_router_id);

    int num_networks() const { return m_num_networks; };
    int getNetXCoord(int router_id) const;
    int getNetYCoord(int router_id) const;
    int getNetRouterID(int router_id) const;
    int getRouterID(int net_id, int net_router_id) const;
    int getNetworkID(int router_id) const;

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    pair< int, int > getNextConn(int cur_router_id, int cur_out_pc);
    pair< int, int > getPrevConn(int cur_router_id, int cur_in_pc);

private:
    int m_concentration;
    int m_num_networks;	// # of networks
    int m_num_routers_per_network;
    int m_num_routers_per_dim;
    int m_num_cores_per_dim;
};

// Output operator declaration
ostream& operator<<(ostream& out, const TopologyDMesh& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TopologyDMesh& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
