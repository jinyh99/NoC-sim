#include "noc.h"
#include "TopologyHMesh.h"
#include "Router.h"
#include "Core.h"

// #define _DEBUG_TOPOLOGY_HMESH

TopologyHMesh::TopologyHMesh()
{
    m_topology_name = "HMesh";
    m_bypass_hop = 0;
}

TopologyHMesh::~TopologyHMesh()
{
}

void TopologyHMesh::buildTopology()
{
    // FIXME
    // Currently, it only supports one-hop express level.
    m_bypass_hop = 1;

    // is the number of cores a square number?
    int core_num_per_dim = (int) sqrt((double) g_cfg.core_num);
    if (g_cfg.core_num != core_num_per_dim*core_num_per_dim) {
        fprintf(stderr, "[HMesh] g_cfg.core_num=%d is not square number!", g_cfg.core_num);
        assert(0);
    }
    m_num_cols = core_num_per_dim;
    m_num_rows = core_num_per_dim;

    g_cfg.router_num = g_cfg.core_num;

    if (g_cfg.router_num_pc != ((m_bypass_hop+1)*4 + g_cfg.core_num_NIs)) {
        fprintf(stderr, "[HMesh] You specified g_cfg.router_num_pc=%d g_cfg.core_num_NIs=%d.\n", g_cfg.router_num_pc, g_cfg.core_num_NIs);
        g_cfg.router_num_pc = (m_bypass_hop+1)*4 + g_cfg.core_num_NIs;
        fprintf(stderr, "[HMesh] We changed g_cfg.router_num_pc=%d.\n", g_cfg.router_num_pc);
    }

    // topology name
    m_topology_name = int2str(m_num_rows) + "x" + int2str(m_num_cols) + " HMesh ("
                    + int2str(m_bypass_hop) +"-hop express channels)";

    // create routers
    for (int n=0; n<g_cfg.router_num; n++) {
        int router_num_ipc = g_cfg.core_num_NIs;
        int router_num_epc = g_cfg.core_num_NIs;
        Router* pRouter = new Router(n, g_cfg.router_num_pc, g_cfg.router_num_vc, router_num_ipc, router_num_epc, g_cfg.router_inbuf_depth);
        g_Router_vec.push_back(pRouter);
    }

    // create cores
    for (int n=0; n<g_cfg.core_num; n++) {
        Core* p_Core = new Core(n, g_cfg.core_num_NIs);
        g_Core_vec.push_back(p_Core);

        Router* p_Router = g_Router_vec[n];

        // map PCs between input-NI and router
        for (int ipc=0; ipc<g_cfg.core_num_NIs; ipc++) {
            int router_in_pc = p_Router->num_internal_pc() + ipc;
            p_Core->getNIInput(ipc)->attachRouter(p_Router, router_in_pc);
            p_Router->appendNIInput(p_Core->getNIInput(ipc));
        }

        // map PCs between output-NI and router
        for (int epc=0; epc<g_cfg.core_num_NIs; epc++) {
            int router_out_pc = p_Router->num_internal_pc() + epc;
            p_Core->getNIOutput(epc)->attachRouter(p_Router, router_out_pc);
            p_Router->appendNIOutput(p_Core->getNIOutput(epc));
        }
    }

    // setup link connection
    for (int i=0; i<g_cfg.router_num; i++) {
        Router* p_router = g_Router_vec[i];
        int router_x_coord = i % m_num_cols;
        int router_y_coord = i / m_num_cols;

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);

            int link_level = getLinkLevel(p_router, out_pc);
            string link_dir_prefix;
            int pc_direction = out_pc%4;

            if (link_level >= 0) {	// links connected with neighbor routers?
                link.m_length_mm *= (double) (link_level+1);
                link.m_delay_factor *= (link_level+1);
            }

            if (out_pc < p_router->num_internal_pc()) {
                switch (pc_direction) {
                case DIR_WEST:
                    link_dir_prefix = "W";
                    if (router_x_coord > link_level)
                        link.m_valid = true;
                    break;
                case DIR_NORTH:
                    link_dir_prefix = "N";
                    if (router_y_coord > link_level)
                        link.m_valid = true;
                    break;
                case DIR_EAST:
                    link_dir_prefix = "E";
                    if (router_x_coord < m_num_cols-1-link_level)
                        link.m_valid = true;
                    break;
                case DIR_SOUTH:
                    link_dir_prefix = "S";
                    if (router_y_coord < m_num_rows-1-link_level)
                        link.m_valid = true;
                    break;
                }

                link.m_link_name = link_dir_prefix + int2str(link_level);
            } else if (out_pc < p_router->num_pc()) {
                int external_pc_index = p_router->num_pc() - out_pc;
                assert(external_pc_index > 0);
                link.m_link_name = "P" + int2str(external_pc_index-1);
            } else {
                assert(0);
            }
        } // for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
    } // for (int i=0; i<g_cfg.router_num; i++) {

    // setup upstream/downstream routers
    for (int r=0; r<g_cfg.router_num; r++) {
        int num_pc = g_Router_vec[r]->num_pc();

#ifdef _DEBUG_TOPOLOGY_HMESH
printf("R-%02d\n", r);
#endif
        // downstream router
        vector< pair< int, int > > connNextRouter_vec;
        connNextRouter_vec.resize(num_pc);
        for (int out_pc=0; out_pc<num_pc; out_pc++) {
            connNextRouter_vec[out_pc] = getNextConn(g_Router_vec[r]->id(), out_pc);
#ifdef _DEBUG_TOPOLOGY_HMESH
printf("  out_pc=%d: next_router_id=%d, next_in_pc=%d\n", out_pc, connNextRouter_vec[out_pc].first, connNextRouter_vec[out_pc].second);
#endif
        }
        g_Router_vec[r]->setNextRouters(connNextRouter_vec);

        // upstream router
        vector< pair< int, int > > connPrevRouter_vec;
        connPrevRouter_vec.resize(num_pc);
        for (int in_pc=0; in_pc<num_pc; in_pc++) {
            connPrevRouter_vec[in_pc] = getPrevConn(g_Router_vec[r]->id(), in_pc);
#ifdef _DEBUG_TOPOLOGY_HMESH
printf("  in_pc=%d: prev_router_id=%d, prev_out_pc=%d\n", in_pc, connPrevRouter_vec[in_pc].first, connPrevRouter_vec[in_pc].second);
#endif
        }
        g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
    } // for (int r=0; r<g_cfg.router_num; r++) {
}

int TopologyHMesh::getLinkLevel(Router* p_router, int out_pc)
{
    assert(out_pc >= 0);

    // 0-level: W, E, S, N	regular channels
    // 1-level: W, E, S, N	express channels
    // ...
    // -1-level:                injection/ejection channels

    int level;

    if (out_pc < p_router->num_internal_pc()) {
        level = out_pc/4;
    } else if (out_pc < p_router->num_pc()) {
        level = -1;
    } else {
        assert(0);
    }

    return level;
} 

int TopologyHMesh::getMinHopCount(int src_router_id, int dst_router_id)
{
    int src_x = src_router_id % m_num_cols;
    int src_y = src_router_id / m_num_cols;
    int dst_x = dst_router_id % m_num_cols;
    int dst_y = dst_router_id / m_num_cols;
    int offset_x = abs(src_x - dst_x);
    int offset_y = abs(src_y - dst_y);

    // FIXME
    // This has a problem.
    int x_hop_count = offset_x / (m_bypass_hop + 1) + offset_x % (m_bypass_hop + 1);
    int y_hop_count = offset_y / (m_bypass_hop + 1) + offset_y % (m_bypass_hop + 1);         

    return x_hop_count + y_hop_count + 1;               
}

pair< int, int > TopologyHMesh::getNextConn(int cur_router_id, int cur_out_pc)
{
    int next_router_id = INVALID_ROUTER_ID;
    int next_in_pc = DIR_INVALID;

    int pc_direction = cur_out_pc%4;
    int level = cur_out_pc/4;
    if (cur_out_pc < (m_bypass_hop+1)*4) {	// internal PC?
        switch (pc_direction) {
        case DIR_WEST:
            next_router_id = cur_router_id - (1+level);
            next_in_pc = DIR_EAST + level*4;
            break;
        case DIR_EAST:
            next_router_id = cur_router_id + (1+level);
            next_in_pc = DIR_WEST + level*4;
            break;
        case DIR_NORTH:
            next_router_id = cur_router_id - (m_num_cols*(1+level));
            next_in_pc = DIR_SOUTH + level*4;
            break;
        case DIR_SOUTH:
            next_router_id = cur_router_id + (m_num_cols*(1+level));
            next_in_pc = DIR_NORTH + level*4;
            break;
        }
    }

// printf("    *cur_out_pc=%d pc_direction=%d level=%d next_router_id=%d\n", cur_out_pc, pc_direction, level, next_router_id);
    if (next_router_id < 0 || next_router_id >= m_num_cols*m_num_rows) {
        next_router_id = INVALID_ROUTER_ID;
        next_in_pc = DIR_INVALID;
    } 

// printf("    #cur_out_pc=%d pc_direction=%d level=%d next_router_id=%d\n", cur_out_pc, pc_direction, level, next_router_id);

    return make_pair(next_router_id, next_in_pc);
}

pair< int, int > TopologyHMesh::getPrevConn(int cur_router_id, int cur_in_pc)
{
    // Note: I reuse getNextConn() function.
    return getNextConn(cur_router_id, cur_in_pc);
}


void TopologyHMesh::printStats(ostream& out) const
{
}

void TopologyHMesh::print(ostream& out) const
{
}

