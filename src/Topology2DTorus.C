#include "noc.h"
#include "Topology2DTorus.h"
#include "Router.h"
#include "Core.h"

Topology2DTorus::Topology2DTorus()
{
    m_topology_name = "Unspecified Torus";
}

Topology2DTorus::~Topology2DTorus()
{
}

void Topology2DTorus::buildTopology()
{
    // check and adjust topology parameter
    g_cfg.router_num = g_cfg.core_num;

    // is the number of cores a square number?
    int core_num_per_dim = (int) sqrt((double) g_cfg.core_num);
    if (g_cfg.core_num != core_num_per_dim*core_num_per_dim) {
        fprintf(stderr, "[2DTorus] g_cfg.core_num=%d is not square number!", g_cfg.core_num);
        assert(0);
    }
    m_num_cols = core_num_per_dim;
    m_num_rows = core_num_per_dim;

    m_topology_name = int2str(m_num_cols) + "x" + int2str( m_num_rows) + " Torus";

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
    for (int r=0; r<g_cfg.router_num; r++) {
        Router* p_router = g_Router_vec[r];

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);

            link.m_valid = true;

            switch (out_pc) {
            case DIR_WEST:
                link.m_link_name = "W";
                link.m_length_mm *= 2.0;	// 2X link length in folded torus
                link.m_delay_factor *= 2;
                break;
            case DIR_EAST:
                link.m_link_name = "E";
                link.m_length_mm *= 2.0;	// 2X link length in folded torus
                link.m_delay_factor *= 2;
                break;
            case DIR_NORTH:
                link.m_link_name = "N";
                link.m_length_mm *= 2.0;	// 2X link length in folded torus
                link.m_delay_factor *= 2;
                break;
            case DIR_SOUTH:
                link.m_link_name = "S";
                link.m_length_mm *= 2.0;	// 2X link length in folded torus
                link.m_delay_factor *= 2;
                break;
            default: 
                if (out_pc < p_router->num_pc()) {
                    int external_pc = p_router->num_pc() - out_pc - 1;
                    assert(external_pc >= 0);
                    link.m_link_name = "P" + int2str(external_pc);
                } else {
                    assert(0);
                }
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

int Topology2DTorus::getMinHopCount(int src_router_id, int dst_router_id)
{
    int src_x_coord, src_y_coord;
    int dst_x_coord, dst_y_coord;
    int x_offset, y_offset;
    int x_hop_count, y_hop_count;
    // hop count except the source
    int NW_hop_count, NE_hop_count, SW_hop_count, SE_hop_count;

    src_x_coord = src_router_id % m_num_cols;
    src_y_coord = src_router_id / m_num_cols;
    dst_x_coord = dst_router_id % m_num_cols;
    dst_y_coord = dst_router_id / m_num_cols;
// printf("src=(%d,%d) dst(%d,%d)\n", src_x_coord, src_y_coord, dst_x_coord, dst_y_coord);

    x_offset = dst_x_coord - src_x_coord;
    y_offset = dst_y_coord - src_y_coord;

    // SE quadrant
    if (x_offset < 0)
        x_hop_count = m_num_cols + x_offset;
    else
        x_hop_count = x_offset;
    if (y_offset < 0)
        y_hop_count = m_num_rows + y_offset;
    else
        y_hop_count = y_offset;
// printf("SE x_hop_count=%d y_hop_count=%d\n", x_hop_count, y_hop_count);
    SE_hop_count = x_hop_count + y_hop_count;

    // NW quadrant
    if (x_offset <= 0)
        x_hop_count = abs(x_offset);
    else
        x_hop_count = m_num_cols - x_offset;
    if (y_offset <= 0)
        y_hop_count = abs(y_offset);
    else
        y_hop_count = m_num_rows - y_offset;
// printf("NW x_hop_count=%d y_hop_count=%d\n", x_hop_count, y_hop_count);
    NW_hop_count = x_hop_count + y_hop_count;

    // NE quadrant
    if (x_offset < 0)
        x_hop_count = m_num_cols + x_offset;
    else
        x_hop_count = x_offset;
    if (y_offset <= 0)
        y_hop_count = abs(y_offset);
    else
        y_hop_count = m_num_rows - y_offset;
// printf("NE x_hop_count=%d y_hop_count=%d\n", x_hop_count, y_hop_count);
    NE_hop_count = x_hop_count + y_hop_count;

    // SW quadrant
    if (x_offset <= 0)
        x_hop_count = abs(x_offset);
    else
        x_hop_count = m_num_cols - x_offset;
    if (y_offset < 0)
        y_hop_count = m_num_rows + y_offset;
    else
        y_hop_count = y_offset;
// printf("SW x_hop_count=%d y_hop_count=%d\n", x_hop_count, y_hop_count);
    SW_hop_count = x_hop_count + y_hop_count;

    // find the minimum-hop-count quadrant
    int min_hop_count = SE_hop_count;
    if (NW_hop_count < min_hop_count) {
        min_hop_count = NW_hop_count;
    }
    if (NE_hop_count < min_hop_count) {
        min_hop_count = NE_hop_count;
    }
    if (SW_hop_count < min_hop_count) {
        min_hop_count = SW_hop_count;
    }

    return min_hop_count + 1;
}

pair< int, int > Topology2DTorus::getNextConn(int cur_router_id, int cur_out_pc)
{
    int cur_x_coord = cur_router_id % m_num_cols;
    int cur_y_coord = cur_router_id / m_num_cols;
    int next_router_id = INVALID_ROUTER_ID;
    int next_in_pc = DIR_INVALID;

    switch (cur_out_pc) {
    case DIR_WEST:
        next_router_id = (cur_x_coord == 0) ? // wrap-around link ?
                         cur_router_id + (m_num_cols-1) :
                         cur_router_id - 1;
        next_in_pc = DIR_EAST;
        break;
    case DIR_NORTH:
        next_router_id = (cur_y_coord == 0) ?
                         cur_x_coord + (m_num_rows-1)*m_num_cols :
                         cur_router_id - m_num_cols;
        next_in_pc = DIR_SOUTH;
        break;
    case DIR_EAST:
        next_router_id = (cur_x_coord == m_num_cols-1) ?
                         cur_router_id - (m_num_cols-1) :
                         cur_router_id + 1;
        next_in_pc = DIR_WEST;
        break;
    case DIR_SOUTH:
        next_router_id = (cur_y_coord == m_num_rows-1) ?
                         cur_x_coord :
                         cur_router_id + m_num_cols;
        next_in_pc = DIR_NORTH;
        break;
    }

    return make_pair(next_router_id, next_in_pc);
}

pair< int, int > Topology2DTorus::getPrevConn(int cur_router_id, int cur_in_pc)
{
    // Note: I reuse getNextConn() function.
    return getNextConn(cur_router_id, cur_in_pc);
}

void Topology2DTorus::printStats(ostream& out) const
{
}

void Topology2DTorus::print(ostream& out) const
{
}

