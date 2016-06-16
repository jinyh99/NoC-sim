#ifndef _TOPOLOGY_NUCA_H_
#define _TOPOLOGY_NUCA_H_

#include "Topology2DMesh.h"

/**
 * Basic topology is mesh.
 * One router supports 4 L2 banks. (baseline radix=8)
 * 8 routers support a core. (radix=9)
 *
 * - Concentration example for Router R0
 * <pre>
 *     C0    C1  
 *        R0-------------R1      
 *     C4  | C5           |
 *         |              |      
 *
 *     R(west)     -----(0)------
 *     R(east)     -----(1)------
 *     R(north)    -----(2)------
 *     R(south)    -----(3)------
 *     C0(L2)---NI------(4)------
 *     C1(L2)---NI------(5)------
 *     C4(L2)---NI------(6)------
 *     C5(L2)---NI------(7)------
 *  </pre>
 */

class TopologySNUCA : public Topology2DMesh {
public:
    // Constructors
    TopologySNUCA();

    // Destructor
    ~TopologySNUCA();

    // Public Methods
    void buildTopology();
    void buildTopology(int degree);
    int getMinHopCount(int src_router_id, int dst_router_id);

    int degree() const { return m_degree; };
    unsigned int getNetXCoord(unsigned int router_id) const;
    unsigned int getNetYCoord(unsigned int router_id) const;
    int getRouterID(int mt, int mt_id);
    int getCoreID(int mt, int mt_id);
    int getExternalPC(int mt, int mt_id);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

public:
    int m_num_L2bank;
    int m_num_L1bank;
    int m_num_dir;

private:
    int m_degree;	// concentration degree

    vector< vector < Core* > > m_router_external_pc_vec;	// X[router_id][core_pos]
};

// Output operator declaration
ostream& operator<<(ostream& out, const TopologySNUCA& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TopologySNUCA& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

void config_snucaCMP_network();
#endif
