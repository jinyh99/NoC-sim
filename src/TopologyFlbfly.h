#ifndef _TOPOLOGY_Flbfly_H_
#define _TOPOLOGY_Flbfly_H_

/*
 * Flattened Butterfly, MICRO 2007
 */

#include "Topology.h"

class TopologyFlbfly : public Topology {
public:
    // Constructors
    TopologyFlbfly();

    // Destructor
    ~TopologyFlbfly();

    // Public Methods
    void buildTopology();
    int getMinHopCount(int src_router_id, int dst_router_id);

    int cols() const { return m_num_cols; };
    int rows() const { return m_num_rows; };
    int concentration() const { return m_concentration; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    pair< int, int > getNextConn(int cur_router_id, int cur_out_pc);
    pair< int, int > getPrevConn(int cur_router_id, int cur_in_pc);

protected:
    int m_num_cols;
    int m_num_rows;
    int m_concentration;
};

// Output operator declaration
ostream& operator<<(ostream& out, const TopologyFlbfly& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TopologyFlbfly& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
