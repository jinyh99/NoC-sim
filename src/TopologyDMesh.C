#include "noc.h"
#include "TopologyDMesh.h"
#include "Router.h"
#include "Core.h"

// #define _DEBUG_TOPOLOGY_DMESH

TopologyDMesh::TopologyDMesh()
{
    m_topology_name = "Two 2D Meshes";
    m_num_networks = 0;
    m_num_routers_per_network = 0;
}

TopologyDMesh::~TopologyDMesh()
{
}

void TopologyDMesh::buildTopology()
{
    g_cfg.net_networks = 2;
    buildTopology(g_cfg.net_networks);
}

void TopologyDMesh::buildTopology(int num_networks)
{
    m_num_networks = num_networks;
    m_concentration = 4;

    m_num_cores_per_dim = (int) sqrt((double) g_cfg.core_num);
    m_num_routers_per_dim = (int) sqrt((double) (g_cfg.core_num/m_concentration));
    m_num_cols = m_num_routers_per_dim;
    m_num_rows = m_num_routers_per_dim;
    m_num_routers_per_network = m_num_cols * m_num_rows;

    // topology name
    m_topology_name = int2str(m_num_networks) + "X "
                    + int2str(m_num_cols) + "x" + int2str(m_num_rows) + " DMesh "
                    + int2str(m_concentration) + "-concentration";

    // create cores
    int core_num_ipc = g_cfg.core_num_NIs * m_num_networks;
    int core_num_epc = g_cfg.core_num_NIs * m_num_networks;
    fprintf(stderr, "[DMesh] g_cfg.core_num=%d core_num_NIs=%d\n", g_cfg.core_num, g_cfg.core_num_NIs);
    for (int n=0; n<g_cfg.core_num; n++) {
        Core* p_Core = new Core(n, g_cfg.core_num_NIs);
        g_Core_vec.push_back(p_Core);
    }

    // create routers
    g_cfg.router_num = m_num_routers_per_network * m_num_networks;
    fprintf(stderr, "[DMesh] m_num_routers_per_network=%d m_num_networks=%d g_cfg.router_num=%d\n", m_num_routers_per_network, m_num_networks, g_cfg.router_num);
    int router_num_ipc = m_concentration;
    int router_num_epc = m_concentration;
    for (int r=0; r<g_cfg.router_num; r++) {
        int router_num_pc = 4 + router_num_ipc;

        Router* pRouter = new Router(r, router_num_pc, g_cfg.router_num_vc,
                                     router_num_ipc, router_num_epc, g_cfg.router_inbuf_depth);
        g_Router_vec.push_back(pRouter);
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("[DMesh] router=%d num_pc=%d num_ipc=%d num_epc=%d\n", pRouter->id(), router_num_pc, router_num_ipc, router_num_epc);
#endif
    }

    // connect cores to routers
    for (unsigned int c=0; c<g_Core_vec.size(); c++) {
        Core* p_Core = g_Core_vec[c];
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("[DMesh] core=%d\n", c);
#endif

        for (int net=0; net<m_num_networks; net++) {
            int router_id = m_num_routers_per_network*net + getRouterRelativeID(c, m_num_cores_per_dim, m_num_routers_per_dim);
            Router* p_Router = g_Router_vec[router_id];
            int core_ipc_base = net;
            int core_epc_base = net;

            // map PCs between input-NI and router
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("          network=%d router=%d(%d):\n", net, router_id, getRouterRelativeID(c, m_num_cores_per_dim, m_num_routers_per_dim));
#endif
            for (int core_ipc=0; core_ipc<g_cfg.core_num_NIs; core_ipc++) {
                int router_in_pc = p_Router->num_internal_pc() + core_ipc;
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("            core_ipc=%d router_in_pc=%d\n", core_ipc_base+core_ipc, router_in_pc);
#endif
                p_Core->getNIInput(core_ipc_base + core_ipc)->attachRouter(p_Router, router_in_pc);
                p_Router->appendNIInput(p_Core->getNIInput(core_ipc_base+core_ipc));
            }

            // map PCs between output-NI and router
            for (int core_epc=0; core_epc<g_cfg.core_num_NIs; core_epc++) {
                int router_out_pc = p_Router->num_internal_pc() + core_epc;
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("            core_epc=%d router_out_pc=%d\n", core_epc_base+core_epc, router_out_pc);
#endif
                p_Core->getNIOutput(core_epc_base + core_epc)->attachRouter(p_Router, router_out_pc);
                p_Router->appendNIOutput(p_Core->getNIOutput(core_epc_base+core_epc));
            }
        }
    }

    // setup link configuration
    for (unsigned int i=0; i<g_Router_vec.size(); i++) {
        Router* p_router = g_Router_vec[i];
        int net_id = p_router->id() / m_num_routers_per_network;
        int net_router_id = p_router->id() % m_num_routers_per_network;
        int net_router_x_coord = net_router_id % m_num_cols;
        int net_router_y_coord = net_router_id / m_num_cols;

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);
            string link_name_prefix;

            switch (out_pc) {
            case DIR_WEST:
                link_name_prefix = "W";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (net_router_x_coord > 0) link.m_valid = true;
                break;
            case DIR_NORTH:
                link_name_prefix = "N";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (net_router_y_coord > 0) link.m_valid = true;
                break;
            case DIR_EAST:
                link_name_prefix = "E";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (net_router_x_coord < m_num_cols-1) link.m_valid = true;
                break;
            case DIR_SOUTH:
                link_name_prefix = "S";
                link.m_length_mm *= 2.0;
                link.m_delay_factor *= 2;
                if (net_router_y_coord < m_num_rows-1) link.m_valid = true;
                break;
            default:
              {
                int external_pc = out_pc - p_router->num_internal_pc();
                assert(external_pc >= 0);
                link.m_link_name = "P" +int2str(external_pc-1);
              }
            }

            if (link.m_valid) {
                link.m_link_name = link_name_prefix + int2str(net_id);
            }
        } // for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
    } // for (unsigned int i=0; i<g_Router_vec.size(); i++) {

    // setup downstream/upstream routers
    for (int r=0; r<g_cfg.router_num; r++) {
        int num_pc = g_Router_vec[r]->num_pc();

#ifdef _DEBUG_TOPOLOGY_DMESH
printf("R-%02d\n", r);
#endif
        // downstream router
        vector< pair< int, int > > connNextRouter_vec;
        connNextRouter_vec.resize(num_pc);
        for (int out_pc=0; out_pc<num_pc; out_pc++) {
            connNextRouter_vec[out_pc] = getNextConn(g_Router_vec[r]->id(), out_pc);
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("  out_pc=%d: next_router_id=%d, next_in_pc=%d\n", out_pc, connNextRouter_vec[out_pc].first, connNextRouter_vec[out_pc].second);
#endif
        }
        g_Router_vec[r]->setNextRouters(connNextRouter_vec);

        // upstream router
        vector< pair< int, int > > connPrevRouter_vec;
        connPrevRouter_vec.resize(num_pc);
        for (int in_pc=0; in_pc<num_pc; in_pc++) {
            connPrevRouter_vec[in_pc] = getPrevConn(g_Router_vec[r]->id(), in_pc);
#ifdef _DEBUG_TOPOLOGY_DMESH
printf("  in_pc=%d: prev_router_id=%d, prev_out_pc=%d\n", in_pc, connPrevRouter_vec[in_pc].first, connPrevRouter_vec[in_pc].second);
#endif
        }
        g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
    } // for (int r=0; r<g_cfg.router_num; r++) {
}

int TopologyDMesh::getMinHopCount(int src_router_id, int dst_router_id)
{
    assert(0);
}

pair< int, int > TopologyDMesh::getNextConn(int cur_router_id, int cur_out_pc)
{
    int net_id = cur_router_id/m_num_routers_per_network;

    int cur_net_router_id = getNetRouterID(cur_router_id);
    int next_net_router_id = INVALID_ROUTER_ID;

    int next_router_id = INVALID_ROUTER_ID;
    int next_in_pc = DIR_INVALID;

    switch (cur_out_pc) {
    case DIR_WEST:
        next_net_router_id = cur_net_router_id - 1;
        next_in_pc = DIR_EAST;
        break;
    case DIR_EAST:
        next_net_router_id = cur_net_router_id + 1;
        next_in_pc = DIR_WEST;
        break;
    case DIR_NORTH:
        next_net_router_id = cur_net_router_id - m_num_cols;
        next_in_pc = DIR_SOUTH;
        break;
    case DIR_SOUTH:
        next_net_router_id = cur_net_router_id + m_num_cols;
        next_in_pc = DIR_NORTH;
        break;
    }

    if (next_net_router_id < 0 || next_net_router_id >= m_num_cols*m_num_rows) {
        next_router_id = INVALID_ROUTER_ID;
        next_in_pc = DIR_INVALID;
    } else {
        next_router_id = net_id*m_num_routers_per_network + next_net_router_id;
    }

    return make_pair(next_router_id, next_in_pc);
}

pair< int, int > TopologyDMesh::getPrevConn(int cur_router_id, int cur_in_pc)
{
    // Note: I reuse getNextConn() function.
    return getNextConn(cur_router_id, cur_in_pc);
}

int TopologyDMesh::getNetXCoord(int router_id) const
{
    int net_router_id = router_id % m_num_routers_per_network;
    int net_router_x_coord = net_router_id % m_num_cols;

    return net_router_x_coord;
}

int TopologyDMesh::getNetYCoord(int router_id) const
{
    int net_router_id = router_id % m_num_routers_per_network;
    int net_router_y_coord = net_router_id / m_num_cols;

    return net_router_y_coord;
}

int TopologyDMesh::getNetRouterID(int router_id) const
{
    return router_id % m_num_routers_per_network;
}

int TopologyDMesh::getRouterID(int net_id, int core_id) const
{
    return net_id*m_num_routers_per_network + core_id;
}

int TopologyDMesh::getNetworkID(int router_id) const
{
    return router_id / m_num_routers_per_network;
}

void TopologyDMesh::printStats(ostream& out) const
{
}

void TopologyDMesh::print(ostream& out) const
{
}

