#ifndef _TOPOLOGY_2D_MESH_H_
#define _TOPOLOGY_2D_MESH_H_

#include "Topology.h"

class Topology2DMesh : public Topology {
public:
    // Constructors
    Topology2DMesh();

    // Destructor
    ~Topology2DMesh();

    // Public Methods
    void buildTopology();
    int getMinHopCount(int src_router_id, int dst_router_id);
    int cols() const { return m_num_cols; };
    int rows() const { return m_num_rows; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    pair< int, int > getNextConn(int cur_router_id, int cur_out_pc);
    pair< int, int > getPrevConn(int cur_router_id, int cur_in_pc);

protected:
    int m_num_cols;
    int m_num_rows;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Topology2DMesh& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Topology2DMesh& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
