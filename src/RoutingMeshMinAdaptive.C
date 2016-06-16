#include "RoutingMeshMinAdaptive.h"
#include "Topology2DMesh.h"

RoutingMeshMinAdaptive::RoutingMeshMinAdaptive()
{
}

RoutingMeshMinAdaptive::~RoutingMeshMinAdaptive()
{
}

void RoutingMeshMinAdaptive::init()
{
    // set routing name
    switch (g_cfg.net_routing) {
    case ROUTING_MIN_OBLIVIOUS:
        m_routing_name = "Mesh MinAdaptive (Oblivious)";
        break;
    case ROUTING_MIN_ADAPTIVE_VC:
        m_routing_name = "Mesh MinAdaptive (Reserved VC#)";
        break;
    default:
        assert(0);
    }
    m_topology_mesh = (Topology2DMesh*) m_topology;
}

int RoutingMeshMinAdaptive::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;

    int dest_router_id = p_flit->dest_router_id();
    int cur_router_id = cur_router->id();
    int dest_x_coord, dest_y_coord;
    int cur_x_coord, cur_y_coord;
    int x_offset, y_offset;
    double cost1, cost2;

    dest_x_coord = dest_router_id % m_topology_mesh->cols();
    dest_y_coord = dest_router_id / m_topology_mesh->cols();

    cur_x_coord = cur_router_id % m_topology_mesh->cols();
    cur_y_coord = cur_router_id / m_topology_mesh->cols();

    x_offset = dest_x_coord - cur_x_coord;
    y_offset = dest_y_coord - cur_y_coord;

    // first, determine the output PC having the smallest cost.
    if (x_offset == 0) {
        if (y_offset == 0)
            cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        else
            cur_out_pc = (y_offset > 0) ? DIR_SOUTH : DIR_NORTH;
    } else if (x_offset > 0) {
        if (y_offset == 0) {
            cur_out_pc = DIR_EAST;
        } else if (y_offset > 0) {
            cost1 = computeCost(cur_router, DIR_EAST);
            cost2 = computeCost(cur_router, DIR_SOUTH);
            cur_out_pc = (cost1 < cost2) ? DIR_EAST : DIR_SOUTH;
        } else { // (y_offset < 0)
            cost1 = computeCost(cur_router, DIR_EAST);
            cost2 = computeCost(cur_router, DIR_NORTH);
            cur_out_pc = (cost1 < cost2) ? DIR_EAST : DIR_NORTH;
        }
    } else {    // x_offset < 0
        if (y_offset == 0) {
            cur_out_pc = DIR_WEST;
        } else if (y_offset > 0) {
            cost1 = computeCost(cur_router, DIR_WEST);
            cost2 = computeCost(cur_router, DIR_SOUTH);
            cur_out_pc = (cost1 < cost2) ? DIR_WEST : DIR_SOUTH;
        } else {
            cost1 = computeCost(cur_router, DIR_WEST);
            cost2 = computeCost(cur_router, DIR_NORTH);
            cur_out_pc = (cost1 < cost2) ? DIR_WEST : DIR_NORTH;
        }
    }
    assert(cur_out_pc != INVALID_PC);

    return cur_out_pc;
}

bool RoutingMeshMinAdaptive::isOutVCDeadlockFree(int out_vc, int in_pc, int in_vc, int out_pc, Router* cur_router, FlitHead* p_flit)
{
    if (cur_router->isEjectChannel(out_pc))
        return true;

    int cur_router_id = cur_router->id();

    int src_router_id = p_flit->src_router_id();
    int dest_router_id = p_flit->dest_router_id();
    int dest_x_coord, dest_y_coord;
    int src_x_coord, src_y_coord;
    int cur_x_coord, cur_y_coord;
    int next_x_coord, next_y_coord;

    src_x_coord = src_router_id % m_topology_mesh->cols();
    src_y_coord = src_router_id / m_topology_mesh->cols();

    dest_x_coord = dest_router_id % m_topology_mesh->cols();
    dest_y_coord = dest_router_id / m_topology_mesh->cols();

    cur_x_coord = cur_router_id % m_topology_mesh->cols();
    cur_y_coord = cur_router_id / m_topology_mesh->cols();

    int next_router_id = cur_router->nextRouters()[out_pc].first;
    assert(next_router_id != INVALID_ROUTER_ID);
    next_x_coord = next_router_id % m_topology_mesh->cols();
    next_y_coord = next_router_id / m_topology_mesh->cols();

    if (dest_y_coord == src_y_coord) {
        if( cur_router->isInjectChannel(in_pc)) {
            return true;
        } else {
            if ( (in_vc < cur_router->num_vc()/2 && out_vc < cur_router->num_vc()/2) ||
                 (in_vc >= cur_router->num_vc()/2 && out_vc >= cur_router->num_vc()/2) )
                return true;
            else
                return false;
        }
    } else if (dest_y_coord > src_y_coord) {
        if (cur_x_coord == next_x_coord) {      // move in Y dimension?
            return true;
        } else {
            if (out_vc < cur_router->num_vc()/2)
                return false;
            else
                return true;
        }
    } else { // (dest_y_coord < src_y_coord)
        if (cur_x_coord == next_x_coord) {      // move in Y dimension?
            return true;
        } else {
            if (out_vc < cur_router->num_vc()/2)
                return true;
            else
                return false;
        }
    }

    // NEVER REACHED.
    assert(0);

    return true;
}

void RoutingMeshMinAdaptive::printStats(ostream& out) const
{
}

void RoutingMeshMinAdaptive::print(ostream& out) const
{
}

double RoutingMeshMinAdaptive::computeCost(Router* cur_router, int cur_out_pc)
{
    switch (g_cfg.net_routing) {
    case ROUTING_MIN_OBLIVIOUS:
        return computeCostRandom();
    case ROUTING_MIN_ADAPTIVE_VC:
        return computeCostVC(cur_router, cur_out_pc);
    }

    // NEVER REACHED
    assert(0);

    return 0.0;
}

double RoutingMeshMinAdaptive::computeCostRandom()
{
    return (double) random(0L, 100L);
}

double RoutingMeshMinAdaptive::computeCostVC(Router* cur_router, int cur_out_pc)
{
    int num_rsv_out_vc = 0;
    for (int out_vc=0; out_vc<cur_router->num_vc(); out_vc++) {
        if (cur_router->outputModule(cur_out_pc, out_vc).m_state != OUT_MOD_I) {
            num_rsv_out_vc++;
        }
    }

    return (double) num_rsv_out_vc;
}
