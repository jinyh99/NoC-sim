#include "RoutingHMeshStatic.h"
#include "TopologyHMesh.h"

RoutingHMeshStatic::RoutingHMeshStatic()
{
}

RoutingHMeshStatic::~RoutingHMeshStatic()
{
}

void RoutingHMeshStatic::init()
{
    m_routing_name = "Hierarchical-Mesh Ascent";
    m_topology_hmesh = (TopologyHMesh*) m_topology;
}

int RoutingHMeshStatic::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    if (g_cfg.net_routing == ROUTING_HMESH_ASCENT)
        return selectOutPCAscent(cur_router, cur_vc, p_flit);

    return selectOutPCDescent(cur_router, cur_vc, p_flit);
}

int RoutingHMeshStatic::selectOutPCAscent(Router* cur_router, unsigned int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;
    int next_router_id = INVALID_ROUTER_ID;       // not used
    int next_in_pc = INVALID_PC;  // not used

    int bypass_hop = m_topology_hmesh->bypass_hop();
    int dest_router_id = p_flit->dest_router_id();
    int cur_router_id = cur_router->id();

    int dest_x_coord = dest_router_id % m_topology_hmesh->cols();
    int dest_y_coord = dest_router_id / m_topology_hmesh->cols();

    int cur_x_coord = cur_router_id % m_topology_hmesh->cols();
    int cur_y_coord = cur_router_id / m_topology_hmesh->cols();

    int x_offset = dest_x_coord - cur_x_coord;
    int y_offset = dest_y_coord - cur_y_coord;

    int four_dir = DIR_INVALID;
    int link_express_level = -1;
    if (x_offset == 0 && y_offset == 0) {
        cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
        next_router_id = INVALID_ROUTER_ID;
        next_in_pc = INVALID_PC;
// printf("X1 out_pc=%d, next_router_id=%d next_in_pc=%d\n", cur_out_pc, next_router_id, next_in_pc);
        return cur_out_pc;
    } else if (x_offset > 0) {
        four_dir = DIR_EAST;

        for (int l=bypass_hop; l>=0; l--) {
            if (x_offset >= l+1) {
                link_express_level = l;
                break;
            }
        }
    } else if (x_offset < 0) {
        four_dir = DIR_WEST;

        for (int l=bypass_hop; l>=0; l--) {
            if (abs(x_offset) >= l+1) {
                link_express_level = l;
                break;
            }
        }
    } else if (y_offset > 0) {
        four_dir = DIR_SOUTH;

        for (int l=bypass_hop; l>=0; l--) {
            if (y_offset >= l+1) {
                link_express_level = l;
                break;
            }
        }
    } else if (y_offset < 0) {
        four_dir = DIR_NORTH;

        for (int l=bypass_hop; l>=0; l--) {
            if (abs(y_offset) >= l+1) {
                link_express_level = l;
                break;
            }
        }
    } else {
        assert(0);
    }
    cur_out_pc = getOutPCFromDir(four_dir, link_express_level);

    switch (four_dir) {
    case DIR_WEST:
        next_router_id = cur_router_id - (link_express_level+1);
        next_in_pc = link_express_level*4 + DIR_EAST;
        break;
    case DIR_EAST:
        next_router_id = cur_router_id + (link_express_level+1);
        next_in_pc = link_express_level*4 + DIR_WEST;
        break;
    case DIR_NORTH:
        next_router_id = cur_router_id - m_topology_hmesh->cols()*(link_express_level+1);
        next_in_pc = link_express_level*4 + DIR_SOUTH;
        break;
    case DIR_SOUTH:
        next_router_id = cur_router_id + m_topology_hmesh->cols()*(link_express_level+1);
        next_in_pc = link_express_level*4 + DIR_NORTH;
        break;
    default:
        assert(0);
    }

// printf("x_offset=%d y_offset=%d\n", x_offset, y_offset);
// printf("X2 cur_router_id=%d out_pc=%d, next_router_id=%d next_in_pc=%d\n", cur_router_id, cur_out_pc, next_router_id, next_in_pc);
    return cur_out_pc;
}

int RoutingHMeshStatic::selectOutPCDescent(Router* cur_router, unsigned int cur_vc, FlitHead* p_flit)
{
    // FIXME: NOT IMPLEMENTED YET
    assert(0);

    return INVALID_PC;
}

int RoutingHMeshStatic::getOutPCFromDir(int direction, int level) const
{
    int out_pc;

    if (level == 0) {
        out_pc = direction;
    } else {
        // Mesh has only 4 directions
        out_pc = level*4 + direction;
    }

    return out_pc;
}

void RoutingHMeshStatic::printStats(ostream& out) const
{
}

void RoutingHMeshStatic::print(ostream& out) const
{
}
