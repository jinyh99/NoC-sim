#include "noc.h"
#include "TopologyFlbfly.h"
#include "Router.h"
#include "Core.h"

/**
 * Flbfly only support 64 cores, each router connected with 4 cores
 * for each router the last 4 pc connected with cores
 * pc 0, 1, 2, 3 used for W, E, N, S port
 * pc 4, 5, 6, 7 used for 2 hops W, E, N, S port
 * pc 8, 9, 10, 11 used for 3 hops W, E, N, S port
 */

TopologyFlbfly::TopologyFlbfly()
{
    m_topology_name = "Flattened Butterfly";
}

TopologyFlbfly::~TopologyFlbfly()
{
}

void TopologyFlbfly::buildTopology()
{
    assert(g_cfg.core_num == 64);

    //default concentration is 4
    m_concentration = 4;
    // check and adjust topology parameter
    g_cfg.router_num = g_cfg.core_num / m_concentration;
    // for 64 cores, col 4, row 4
    m_num_cols = 4;
    m_num_rows = 4;

    // topology name
    m_topology_name = "Flattened Butterfly (" + int2str(m_concentration) + " concentration)";

    // create cores
    int num_cores_in_row = (int) sqrt(g_cfg.core_num);	// # cores in one row
    for (int n=0; n<g_cfg.core_num; n++) {
        Core* p_Core = new Core(n, g_cfg.core_num_NIs);
        g_Core_vec.push_back(p_Core);
    }

    // create routers
    for (int n=0; n<g_cfg.router_num; n++) {
        int router_num_ipc = g_cfg.core_num_NIs * m_concentration;
        int router_num_epc = g_cfg.core_num_NIs * m_concentration;
        // 4 means we have for direction(W, E, N, S), 3 means in each direction we have three choice, 1-hop link, 2-hop link, 3-hop link
        // nomally we only need 10(4+6) PCs per router, here we assign 16(4+4*3) PCs, 6 of them are not used.
        int router_num_pc = router_num_ipc + 4 * 3;  
        Router* p_Router = new Router(n, router_num_pc, g_cfg.router_num_vc, router_num_ipc, router_num_epc, g_cfg.router_inbuf_depth);

        g_Router_vec.push_back(p_Router);

        int router_x_coord = n % m_num_cols;
        int router_y_coord = n / m_num_rows;

        vector< int > core_id_vec;
        core_id_vec.resize(m_concentration);
        core_id_vec[0] = 2 * router_x_coord + 2 * router_y_coord * num_cores_in_row;	// left top
        core_id_vec[1] = core_id_vec[0] + 1;						// right top
        core_id_vec[2] = core_id_vec[0] + num_cores_in_row;				// left bottom
        core_id_vec[3] = core_id_vec[2] + 1;						// right bottom

        // core to router map: core_id -> (router_id, port_pos (relative))
        m_core2router_map[core_id_vec[0]] = make_pair(n, 0);
        m_core2router_map[core_id_vec[1]] = make_pair(n, 1);
        m_core2router_map[core_id_vec[2]] = make_pair(n, 2);
        m_core2router_map[core_id_vec[3]] = make_pair(n, 3);

        // router to core map: (router_id, port_pos (relative)) -> core_id
        m_router2core_map[make_pair(n, 0)] = core_id_vec[0];
        m_router2core_map[make_pair(n, 1)] = core_id_vec[1];
        m_router2core_map[make_pair(n, 2)] = core_id_vec[2];
        m_router2core_map[make_pair(n, 3)] = core_id_vec[3];

        // The last 4 PCs are used as connection to cores.
        for (int c=0; c<m_concentration; c++) {
            Core* p_Core = g_Core_vec[core_id_vec[c]];
            // map PCs between input-NI and router
            for (int ipc=0; ipc<g_cfg.core_num_NIs; ipc++) {
                int router_in_pc = p_Router->num_internal_pc() + c * g_cfg.core_num_NIs + ipc;
                p_Core->getNIInput(ipc)->attachRouter(p_Router, router_in_pc);
                p_Router->appendNIInput(p_Core->getNIInput(ipc));
            }
            // map PCs between output-NI and router
            for (int epc=0; epc<g_cfg.core_num_NIs; epc++) {
                int router_out_pc = p_Router->num_internal_pc() + c * g_cfg.core_num_NIs + epc;
                p_Core->getNIOutput(epc)->attachRouter(p_Router, router_out_pc);
                p_Router->appendNIOutput(p_Core->getNIOutput(epc));
            }
        }
    }


    // setup link connection
    for (int r=0; r<g_cfg.router_num; r++) {
        Router* p_router = g_Router_vec[r];

        int router_x_coord = r%m_num_cols; 
        int router_y_coord = r/m_num_cols;

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);

            switch (out_pc) {
            case DIR_WEST:
                link.m_link_name = "W";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (router_x_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_NORTH:
                link.m_link_name = "N";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (router_y_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_EAST:
                link.m_link_name = "E";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (router_x_coord < m_num_cols-1)
                    link.m_valid = true;
                break;
            case DIR_SOUTH:
                link.m_link_name = "S";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (router_y_coord < m_num_rows-1)
                    link.m_valid = true;
                break;
            // express link
            case DIR_WEST2:
                link.m_link_name = "W2";
                link.m_length_mm *= 2*2.0;
                link.m_delay_factor *= 2*2;
                if (router_x_coord > 1)
                    link.m_valid = true;
              break;
            case DIR_NORTH2:
                link.m_link_name = "N2";
                link.m_length_mm *= 2*2.0;
                link.m_delay_factor *= 2*2;
                if (router_y_coord > 1)
                    link.m_valid = true;
              break;
            case DIR_EAST2:
                link.m_link_name = "E2";
                link.m_length_mm *= 2*2.0;
                link.m_delay_factor *= 2*2;
                if (router_x_coord < 2)
                    link.m_valid = true;
                break;
            case DIR_SOUTH2:
                link.m_link_name = "S2";
                link.m_length_mm *= 2*2.0;
                link.m_delay_factor *= 2*2;
                if (router_y_coord < 2)
                    link.m_valid = true;
                break;
            case DIR_WEST3:
                link.m_link_name = "W3";
                link.m_length_mm *= 3*2.0;
                link.m_delay_factor *= 3*2;
                if (router_x_coord == 3)
                    link.m_valid = true;
                break;
            case DIR_NORTH3:
                link.m_link_name = "N3";
                link.m_length_mm *= 3*2.0;
                link.m_delay_factor *= 3*2;
                if (router_y_coord == 3)
                    link.m_valid = true;
                break;
            case DIR_EAST3:
                link.m_link_name = "E3";
                link.m_length_mm *= 3*2.0;
                link.m_delay_factor *= 3*2;
                if (router_x_coord == 0)
                    link.m_valid = true;
                break;
            case DIR_SOUTH3:
                link.m_link_name = "S3";
                link.m_length_mm *= 3*2.0;
                link.m_delay_factor *= 3*2;
                if (router_y_coord == 0)
                    link.m_valid = true;
                break;
            default:
              {
                int external_pc = p_router->num_pc() - out_pc - 1;
                assert(external_pc >= 0);
                link.m_link_name = "P" + int2str(external_pc);
              }
                break;
            }
        } // for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
    } // for (int r=0; r<g_cfg.router_num; r++) {

    // setup downstream/upstream routers
    for (int r=0; r<g_cfg.router_num; r++) {
        int num_pc = g_Router_vec[r]->num_pc();

        // downstream router
        vector< pair< int, int > > connNextRouter_vec;
        connNextRouter_vec.resize(num_pc);
        for (int out_pc=0; out_pc<num_pc; out_pc++)
            connNextRouter_vec[out_pc] = getNextConn(g_Router_vec[r]->id(), out_pc);
        g_Router_vec[r]->setNextRouters(connNextRouter_vec);

        // upstream router
        vector< pair< int, int > > connPrevRouter_vec;
        connPrevRouter_vec.resize(num_pc);
        for (int in_pc=0; in_pc<num_pc; in_pc++)
            connPrevRouter_vec[in_pc] = getPrevConn(g_Router_vec[r]->id(), in_pc);
        g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
    } // for (int r=0; r<g_cfg.router_num; r++) {
}

int TopologyFlbfly::getMinHopCount(int src_router_id, int dst_router_id)
{
    assert(0);
}

pair< int, int > TopologyFlbfly::getNextConn(int cur_router_id, int cur_out_pc)
{
    int next_router_id = INVALID_ROUTER_ID;
    int next_in_pc = DIR_INVALID;

    // Only for 8x8 tile configuration
    // here we just use 4 to make it easier

    int router_x_coord = cur_router_id % 4;
    int router_y_coord = cur_router_id / 4;
    switch (cur_out_pc) {
    case DIR_WEST:
        if(router_x_coord > 0){
            next_router_id = cur_router_id - 1;
            next_in_pc = DIR_EAST;
        }
        break;
    case DIR_EAST:
        if(router_x_coord < 3){
            next_router_id = cur_router_id + 1;
            next_in_pc = DIR_WEST;
        }
        break;
    case DIR_NORTH:
        if(router_y_coord > 0){
            next_router_id = cur_router_id - 4;
            next_in_pc = DIR_SOUTH;
        }
        break;
    case DIR_SOUTH:
        if(router_y_coord < 3){
            next_router_id = cur_router_id + 4;
            next_in_pc = DIR_NORTH;
        }
        break;
    //express link
    case DIR_WEST2:
        if(router_x_coord > 1){
            next_router_id = cur_router_id - 2;
            next_in_pc = DIR_EAST2;
        }
        break;
    case DIR_EAST2:
        if(router_x_coord < 2){
            next_router_id = cur_router_id + 2;
            next_in_pc = DIR_WEST2;
        }
        break;
    case DIR_NORTH2:
        if(router_y_coord > 1){
            next_router_id = cur_router_id - 8;
            next_in_pc = DIR_SOUTH2;
        }
        break;
    case DIR_SOUTH2:
        if(router_y_coord < 2){
            next_router_id = cur_router_id + 8;
            next_in_pc = DIR_NORTH2;
        }
        break;

    case DIR_WEST3:
        if(router_x_coord == 3){
            next_router_id = cur_router_id - 3;
            next_in_pc = DIR_EAST3;
        }
        break;
    case DIR_EAST3:
        if(router_x_coord == 0){
            next_router_id = cur_router_id + 3;
            next_in_pc = DIR_WEST3;
        }
        break;
    case DIR_NORTH3:
        if(router_y_coord == 3){
            next_router_id = cur_router_id - 12;
            next_in_pc = DIR_SOUTH3;
        }
        break;
    case DIR_SOUTH3:
        if(router_y_coord == 0){
            next_router_id = cur_router_id + 12;
            next_in_pc = DIR_NORTH3;
            break;
        }
    }

    if (next_router_id < 0 || next_router_id >= m_num_cols*m_num_rows) {
        next_router_id = INVALID_ROUTER_ID;
        next_in_pc = DIR_INVALID;
    }
 
    return make_pair(next_router_id, next_in_pc);
}

pair< int, int > TopologyFlbfly::getPrevConn(int cur_router_id, int cur_in_pc)
{
    // Note: I reuse getNextConn() function.
    return getNextConn(cur_router_id, cur_in_pc);
}

void TopologyFlbfly::printStats(ostream& out) const
{
}

void TopologyFlbfly::print(ostream& out) const
{
}

