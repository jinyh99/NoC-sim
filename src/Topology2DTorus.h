#ifndef _TOPOLOGY_2D_TORUS_H_
#define _TOPOLOGY_2D_TORUS_H_

#include "Topology2DMesh.h"

class Topology2DTorus : public Topology2DMesh {
public:
    // Constructors
    Topology2DTorus();

    // Destructor
    ~Topology2DTorus();

    // Public Methods
    void buildTopology();
    int getMinHopCount(int src_router_id, int dst_router_id);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    // override mesh topology connections
    pair< int, int > getNextConn(int cur_router_id, int cur_out_pc);
    pair< int, int > getPrevConn(int cur_router_id, int cur_in_pc);

};

// Output operator declaration
ostream& operator<<(ostream& out, const Topology2DTorus& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Topology2DTorus& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
