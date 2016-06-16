#include "RoutingMeshStatic.h"
#include "Topology2DMesh.h"

RoutingMeshStatic::RoutingMeshStatic()
{
}

RoutingMeshStatic::~RoutingMeshStatic()
{
}

void RoutingMeshStatic::init()
{
    if (g_cfg.net_routing == ROUTING_XY)
        m_routing_name = "XY";
    else if (g_cfg.net_routing == ROUTING_YX)
        m_routing_name = "YX";
    m_topology_mesh = (Topology2DMesh*) m_topology;
}

int RoutingMeshStatic::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;
    int dest_router_id = p_flit->dest_router_id();
    int cur_router_id = cur_router->id();
    int dest_x_coord, dest_y_coord;
    int cur_x_coord, cur_y_coord;

    dest_x_coord = dest_router_id % m_topology_mesh->cols();
    dest_y_coord = dest_router_id / m_topology_mesh->cols();

    cur_x_coord = cur_router_id % m_topology_mesh->cols();
    cur_y_coord = cur_router_id / m_topology_mesh->cols();

    switch (g_cfg.net_routing) {
    case ROUTING_XY:
        if ( dest_x_coord > cur_x_coord ) {
            cur_out_pc = DIR_EAST;
        } else if ( dest_x_coord < cur_x_coord ) {
            cur_out_pc = DIR_WEST;
        } else if ( dest_y_coord > cur_y_coord ) {
            cur_out_pc = DIR_SOUTH;
        } else if ( dest_y_coord < cur_y_coord ) {
            cur_out_pc = DIR_NORTH;
        } else {
            assert( dest_router_id == cur_router_id );
            cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        }
        break;
    case ROUTING_YX:
        if ( dest_y_coord > cur_y_coord ) {
            cur_out_pc = DIR_SOUTH;
        } else if ( dest_y_coord < cur_y_coord ) {
            cur_out_pc = DIR_NORTH;
        } else if ( dest_x_coord > cur_x_coord ) {
            cur_out_pc = DIR_EAST;
        } else if ( dest_x_coord < cur_x_coord ) {
            cur_out_pc = DIR_WEST;
        } else {
            assert ( dest_router_id == cur_router_id );
            cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        }
        break;
    default:
        assert(0);
    }

    return cur_out_pc;
}

vector< Router* > RoutingMeshStatic::getPathVector(Router* src_router, Router* dest_router)
{
    vector< Router* > path_vec;

    Router* cur_router = src_router;
    path_vec.push_back(src_router);

    while (cur_router != dest_router) {
        // make artifical packet and flit
        Packet pkt;
        pkt.setSrcRouterID(src_router->id());
        pkt.addDestRouterID(dest_router->id());
        FlitHead head_flit;
        head_flit.setPkt(&pkt);

        const int cur_in_vc = 0;
        const int out_pc = selectOutPC(cur_router, cur_in_vc, &head_flit);
        const int next_router_id = cur_router->nextRouters()[out_pc].first;
        assert(next_router_id < (int) g_Router_vec.size());
        Router* next_router = g_Router_vec[next_router_id];
        path_vec.push_back(next_router);

        cur_router = next_router;
    }

    return path_vec;
}

void RoutingMeshStatic::printStats(ostream& out) const
{
}

void RoutingMeshStatic::print(ostream& out) const
{
}
