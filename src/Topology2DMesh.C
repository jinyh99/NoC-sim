#include "noc.h"
#include "Topology2DMesh.h"
#include "Router.h"
#include "Core.h"

Topology2DMesh::Topology2DMesh()
{
    m_topology_name = "Unspecified Mesh";
}

Topology2DMesh::~Topology2DMesh()
{
}

void Topology2DMesh::buildTopology()
{
    // check and adjust topology parameter.
    g_cfg.router_num = g_cfg.core_num;

    // check if the number of cores is a square number.
    int core_num_per_dim = (int) sqrt((double) g_cfg.core_num);
    if (g_cfg.core_num != core_num_per_dim*core_num_per_dim) {
        fprintf(stderr, "[2DMesh] g_cfg.core_num=%d is not square number!", g_cfg.core_num);
        assert(0);
    }
    m_num_cols = core_num_per_dim;
    m_num_rows = core_num_per_dim;

    // topology name
    m_topology_name = int2str(m_num_cols) + "x" + int2str(m_num_rows) + " Mesh";

    // create routers
    for (int n=0; n<g_cfg.router_num; n++) {
        int router_num_ipc = g_cfg.core_num_NIs;
        int router_num_epc = g_cfg.core_num_NIs;
        Router* pRouter = new Router(n, g_cfg.router_num_pc, g_cfg.router_num_vc,
                                     router_num_ipc, router_num_epc, g_cfg.router_inbuf_depth);

        g_Router_vec.push_back(pRouter);
    }

    // create cores
    createCores();

    // setup link connection
    for (unsigned int r=0; r<g_Router_vec.size(); r++) {
        Router* p_router = g_Router_vec[r];
        int router_x_coord = r % m_num_cols;
        int router_y_coord = r / m_num_cols;

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);

            switch (out_pc) {
            case DIR_WEST:
                link.m_link_name = "W";
                if (router_x_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_NORTH:
                link.m_link_name = "N";
                if (router_y_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_EAST:
                link.m_link_name = "E";
                if (router_x_coord < m_num_cols-1)
                    link.m_valid = true;
                break;
            case DIR_SOUTH:
                link.m_link_name = "S";
                if (router_y_coord < m_num_rows-1)
                    link.m_valid = true;
                break;
            default:
                if (out_pc < p_router->num_pc()) {
                    int external_pc = p_router->num_pc() - out_pc - 1;
                    assert(external_pc >= 0);
                    link.m_link_name = "P" + int2str(external_pc);
                } else {
                    assert(0);
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

int Topology2DMesh::getMinHopCount(int src_router_id, int dst_router_id)
{
    int src_x = src_router_id % m_num_cols;
    int src_y = src_router_id / m_num_cols;
    int dst_x = dst_router_id % m_num_cols;
    int dst_y = dst_router_id / m_num_cols;

    return abs(src_x - dst_x) + abs(src_y - dst_y) + 1;
}

pair< int, int > Topology2DMesh::getNextConn(int cur_router_id, int cur_out_pc)
{
    int next_router_id = INVALID_ROUTER_ID;
    int next_in_pc = DIR_INVALID;

    switch (cur_out_pc) {
    case DIR_WEST:
        next_router_id = cur_router_id - 1;
        next_in_pc = DIR_EAST;
        break;
    case DIR_EAST:
        next_router_id = cur_router_id + 1;
        next_in_pc = DIR_WEST;
        break;
    case DIR_NORTH:
        next_router_id = cur_router_id - m_num_cols;
        next_in_pc = DIR_SOUTH;
        break;
    case DIR_SOUTH:
        next_router_id = cur_router_id + m_num_cols;
        next_in_pc = DIR_NORTH;
        break;
    }

    if (next_router_id < 0 || next_router_id >= m_num_cols*m_num_rows) {
        next_router_id = INVALID_ROUTER_ID;
        next_in_pc = DIR_INVALID;
    }

    return make_pair(next_router_id, next_in_pc);
}

pair< int, int > Topology2DMesh::getPrevConn(int cur_router_id, int cur_in_pc)
{
    // Note: I reuse getNextConn() function.
    return getNextConn(cur_router_id, cur_in_pc);
}

void Topology2DMesh::printStats(ostream& out) const
{
}

void Topology2DMesh::print(ostream& out) const
{
}
