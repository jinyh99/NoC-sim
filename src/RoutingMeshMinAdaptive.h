#ifndef _ROUTING_MESH_MIN_ADAPTIVE_H_
#define _ROUTING_MESH_MIN_ADAPTIVE_H_

/*
 * ROUTING_MIN_ADAPTIVE_VC: fully-adaptive routing
 * Duato Book: p.162, Linder-Harden's two virtual networks
 */

#include "noc.h"
#include "Routing.h"
#include "RoutingMeshStatic.h"

class RoutingMeshMinAdaptive : public RoutingMeshStatic {
public:
    // Constructors
    RoutingMeshMinAdaptive();

    // Destructor
    ~RoutingMeshMinAdaptive();

    // Public Methods
    void init();
    int selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit);
    bool isOutVCDeadlockFree(int out_vc, int in_pc, int in_vc, int out_pc, Router* cur_router, FlitHead* p_flit);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    double computeCost(Router* cur_router, int cur_out_pc);
    double computeCostRandom();
    /// return # of reserved VCs for downstream router
    double computeCostVC(Router* cur_router, int cur_out_pc);

private:
    // For computeCostHistory()
    // per output-PC flit delay for each router for one period
    vector< vector < double > > m_cost_flit_outpc_delay_vec;	// X[router_id][out_pc]
    // per output-PC flit count for each router for one period
    vector< vector < unsigned long long > > m_cost_flit_outpc_count_vec;	// X[router_id][out_pc]
};

// Output operator declaration
ostream& operator<<(ostream& out, const RoutingMeshMinAdaptive& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const RoutingMeshMinAdaptive& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
