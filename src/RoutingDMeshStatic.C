#include "RoutingDMeshStatic.h"
#include "TopologyDMesh.h"

RoutingDMeshStatic::RoutingDMeshStatic()
{
}

RoutingDMeshStatic::~RoutingDMeshStatic()
{
}

void RoutingDMeshStatic::init()
{
    if (g_cfg.net_routing == ROUTING_DMESH_XY)
        m_routing_name = "DMesh-XY";
    else if (g_cfg.net_routing == ROUTING_DMESH_YX)
        m_routing_name = "DMesh-YX";

    m_topology_dmesh = (TopologyDMesh*) m_topology;
}

int RoutingDMeshStatic::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;
    int net_id = m_topology_dmesh->getNetworkID(cur_router->id()); 
    int cur_router_id = cur_router->id();

    // We don't use p_flit->getDestRouterID().
    int dest_router_id = m_topology_dmesh->getRouterID(net_id, p_flit->dest_core_id());
    int net_dest_router_id = m_topology_dmesh->getNetRouterID(dest_router_id);
    int net_dest_x_coord = m_topology_dmesh->getNetXCoord(dest_router_id);
    int net_dest_y_coord = m_topology_dmesh->getNetYCoord(dest_router_id);

    int net_cur_router_id = m_topology_dmesh->getNetRouterID(cur_router_id);
    int net_cur_x_coord = m_topology_dmesh->getNetXCoord(cur_router_id);
    int net_cur_y_coord = m_topology_dmesh->getNetYCoord(cur_router_id);

    switch (g_cfg.net_routing) {
    case ROUTING_DMESH_XY:
        if ( net_dest_x_coord > net_cur_x_coord ) {
            cur_out_pc = DIR_EAST;
        } else if ( net_dest_x_coord < net_cur_x_coord ) {
            cur_out_pc = DIR_WEST;
        } else if ( net_dest_y_coord > net_cur_y_coord ) {
            cur_out_pc = DIR_SOUTH;
        } else if ( net_dest_y_coord < net_cur_y_coord ) {
            cur_out_pc = DIR_NORTH;
        } else {
            assert(net_dest_router_id == net_cur_router_id);
            cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        }
        break;
    case ROUTING_DMESH_YX:
        if ( net_dest_y_coord > net_cur_y_coord ) {
            cur_out_pc = DIR_SOUTH;
        } else if ( net_dest_y_coord < net_cur_y_coord ) {
            cur_out_pc = DIR_NORTH;
        } else if ( net_dest_x_coord > net_cur_x_coord ) {
            cur_out_pc = DIR_EAST;
        } else if ( net_dest_x_coord < net_cur_x_coord ) {
            cur_out_pc = DIR_WEST;
        } else if ( net_dest_router_id == net_cur_router_id ) {
            cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        }
        break;
    default:
        assert(0);
    }

#if 0
printf("  net_id=%d\n", net_id);
printf("  f=%lld, dest_core_id=%d, dest_router_id=%d\n", p_flit->id(), p_flit->getDestCoreID(), p_flit->getDestRouterID());
printf("  cur_router_id=%d\n", cur_router_id);
printf("  net_cur_router_id=%d\n", net_cur_router_id);
printf("  net_cur_x_coord=%d\n", net_cur_x_coord);
printf("  net_cur_y_coord=%d\n", net_cur_y_coord);
printf("  dest_router_id=%d\n", dest_router_id);
printf("  net_dest_router_id=%d\n", net_dest_router_id);
printf("  net_dest_x_coord=%d\n", net_dest_x_coord);
printf("  net_dest_y_coord=%d\n", net_dest_y_coord);
printf("  cur_out_pc=%d\n", cur_out_pc);
#endif

    return cur_out_pc;
}

void RoutingDMeshStatic::printStats(ostream& out) const
{
}

void RoutingDMeshStatic::print(ostream& out) const
{
}
