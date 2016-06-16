#include "RoutingTorusStatic.h"
#include "Topology2DTorus.h"

RoutingTorusStatic::RoutingTorusStatic()
{
}

RoutingTorusStatic::~RoutingTorusStatic()
{
}

void RoutingTorusStatic::init()
{
    m_routing_name = "Torus Min XY (Static)";
    m_topology_torus = (Topology2DTorus*) m_topology;

    assert(g_cfg.router_num_vc >= 2);
    assert(g_cfg.router_num_vc%2 == 0);
}

int RoutingTorusStatic::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;
    int dest_router_id = p_flit->dest_router_id();
    int cur_router_id = cur_router->id();

    int dest_x_coord = dest_router_id % m_topology_torus->cols();
    int dest_y_coord = dest_router_id / m_topology_torus->cols();

    int cur_x_coord = cur_router_id % m_topology_torus->cols();
    int cur_y_coord = cur_router_id / m_topology_torus->cols();

    int x_offset = dest_x_coord - cur_x_coord;
    int y_offset = dest_y_coord - cur_y_coord;

    bool x_direction_change, y_direction_change;
    x_direction_change = y_direction_change = false;

    if (abs(x_offset)*2 > m_topology_torus->cols())
        x_direction_change = true;
    if (abs(y_offset)*2 > m_topology_torus->rows())
        y_direction_change = true;

    // determine cur_out_pc
    if ( dest_router_id == cur_router_id ) {
        cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
    } else if ( x_offset > 0 ) {
        cur_out_pc = x_direction_change ? DIR_WEST : DIR_EAST;
    } else if ( x_offset < 0 ) {
        cur_out_pc = x_direction_change ? DIR_EAST : DIR_WEST;
    } else if ( y_offset > 0 ) {
        cur_out_pc = y_direction_change ? DIR_NORTH : DIR_SOUTH;
    } else if ( y_offset < 0 ) {
        cur_out_pc = y_direction_change ? DIR_SOUTH : DIR_NORTH;
    }
    assert(cur_out_pc >= 0);

    return cur_out_pc;
}

bool RoutingTorusStatic::isOutVCDeadlockFree(int out_vc, int in_pc, int in_vc, int out_pc, Router* cur_router, FlitHead* p_flit)
{
    int cur_router_id = cur_router->id();
    int dest_router_id = p_flit->dest_router_id();
    int num_vc = cur_router->num_vc();         // # of VCs in the current router

    int dest_x_coord = dest_router_id % m_topology_torus->cols();
    int dest_y_coord = dest_router_id / m_topology_torus->cols();

    int cur_x_coord = cur_router_id % m_topology_torus->cols();
    int cur_y_coord = cur_router_id / m_topology_torus->cols();

    if (cur_x_coord < dest_x_coord) {
        // select high VC
        if (out_vc >= num_vc/2 && out_vc < num_vc)
            return true;
    } else if (cur_x_coord > dest_x_coord) {
        // select low VC
        if (out_vc >= 0 && out_vc < num_vc/2)
            return true;
    } else {    // cur_x_coord == dest_x_coord
        if (cur_y_coord < dest_y_coord) {
            // select high VC
            if (out_vc >= num_vc/2 && out_vc < num_vc)
                return true;
        } else if (cur_y_coord > dest_y_coord) {
            // select low VC
            if (out_vc >= 0 && out_vc < num_vc/2)
                return true;
        } else {
            // select any VC
            return true;
        }
    }

    return false;
}

void RoutingTorusStatic::printStats(ostream& out) const
{
}

void RoutingTorusStatic::print(ostream& out) const
{
}
