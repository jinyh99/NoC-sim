#include "noc.h"
#include "Router.h"
#include "Core.h"
#include "TopologySNUCA.h"
#include "WorkloadGEMSType.h"

// #define _DEBUG_TOPOLOGY_SNUCA

////////////////////////////////////////////////////////////////////////////////////
// for CMP-SNUCA
const static int g_8p_NUCA_L1_num = 8;
const static int g_8p_NUCA_L2_num = 256;
const static int g_8p_NUCA_Dir_num = 8;

// attached router ID
const static int g_NUCA_L1_map[g_8p_NUCA_L1_num] = {
     1, 5, 15, 47, 62, 58, 48, 16};
const static int g_NUCA_L2_map[g_8p_NUCA_L2_num] = {
     0,  0,  1,  1,  2,  2,  0,  0,  1,  1,
     2,  2,  9,  9,  9,  9,  4,  4,  5,  5,
     6,  6,  4,  4,  5,  5,  6,  6, 13, 13,
    13, 13,  7,  7,  7,  7, 14, 14, 15, 15,
    14, 14, 15, 15, 23, 23, 23, 23, 39, 39,
    39, 39, 46, 46, 47, 47, 46, 46, 47, 47,
    55, 55, 55, 55, 54, 54, 54, 54, 61, 61,
    62, 62, 63, 63, 61, 61, 62, 62, 63, 63,
    50, 50, 50, 50, 57, 57, 58, 58, 59, 59,
    57, 57, 58, 58, 59, 59, 40, 40, 40, 40,
    48, 48, 49, 49, 48, 48, 49, 49, 56, 56,
    56, 56,  8,  8,  8,  8, 16, 16, 17, 17,
    16, 16, 17, 17, 24, 24, 24, 24,  3,  3,
    10, 10, 11, 10, 10, 11, 18, 18, 19, 18,
    18, 19, 27, 27,  3,  3, 11, 12, 12, 11,
    12, 12, 19, 20, 20, 19, 20, 20, 27, 27,
    21, 21, 22, 22, 21, 21, 22, 22, 28, 28,
    29, 29, 30, 30, 31, 31, 28, 28, 29, 29,
    30, 30, 31, 31, 37, 37, 38, 38, 37, 37,
    38, 38, 36, 36, 44, 45, 45, 44, 45, 45,
    52, 53, 53, 52, 53, 53, 60, 60, 36, 36,
    43, 43, 44, 43, 43, 44, 51, 51, 52, 51,
    51, 52, 60, 60, 32, 32, 33, 33, 34, 34,
    35, 35, 41, 41, 42, 42, 41, 41, 42, 42,
    25, 25, 26, 26, 25, 25, 26, 26, 32, 32,
    33, 33, 34, 34, 35, 35
};
const static int g_NUCA_Dir_map[g_8p_NUCA_Dir_num] = {
    27, 28, 35, 36, 27, 28, 35, 36 };
////////////////////////////////////////////////////////////////////////////////////

TopologySNUCA::TopologySNUCA()
{
    m_topology_name = "8x8 Mesh for CMP-SNUCA";

    m_degree = 0;
    m_num_cols = m_num_rows = 0;
}

TopologySNUCA::~TopologySNUCA()
{
}

void TopologySNUCA::buildTopology()
{
    buildTopology(4);
}

void TopologySNUCA::buildTopology(int degree)
{
    m_degree = degree;
    m_num_L2bank = 256;
    m_num_L1bank = 8;
    m_num_dir = 8;
    assert((m_num_L1bank + m_num_L2bank + m_num_dir) == g_cfg.core_num);
    assert(m_num_L2bank/degree == g_cfg.router_num);
    m_num_cols = m_num_rows = (int) sqrt((double) g_cfg.router_num);
#ifdef _DEBUG_TOPOLOGY_SNUCA
printf("[SNUCA] L2bank=%d L1bank=%d dir=%d core_num=%d router_num=%d\n", m_num_L2bank, m_num_L1bank, m_num_dir, g_cfg.core_num, g_cfg.router_num);
#endif

    // create cores (L2 banks, L1 banks, dirs)
    for (int n=0; n<g_cfg.core_num; n++) {	// L2 banks
        int core_id = n;
        int num_NIs = 1;

        Core* p_Core = new Core(core_id, num_NIs);
        g_Core_vec.push_back(p_Core);
    }

    m_router_external_pc_vec.resize(g_cfg.router_num);

    // determine the number of external PCs for each router
    for (unsigned int c=0; c<g_Core_vec.size(); c++) {
        int attached_router_id;
        if (c < (unsigned int) m_num_L2bank) {
            attached_router_id = g_NUCA_L2_map[c];
        } else if (c < (unsigned int) (m_num_L2bank + m_num_L1bank)) {
            attached_router_id = g_NUCA_L1_map[c - m_num_L2bank];
        } else {
            attached_router_id = g_NUCA_Dir_map[c - m_num_L2bank - m_num_L1bank];
        }

        m_router_external_pc_vec[attached_router_id].push_back(g_Core_vec[c]);
    }

    // create routers
    for (int r=0; r<g_cfg.router_num; r++) {
        const int num_external_pc = m_router_external_pc_vec[r].size();
        const int num_pc = 4 + num_external_pc;	// 4 PCs for neighbors
        Router* p_Router = new Router(r, num_pc, g_cfg.router_num_vc,
                                      num_external_pc, num_external_pc, g_cfg.router_inbuf_depth);
        g_Router_vec.push_back(p_Router);
#ifdef _DEBUG_TOPOLOGY_SNUCA
printf("[SNUCA] router=%d num_pc=%d num_ipc/num_epc=%d\n", p_Router->id(), num_pc, num_external_pc);
#endif

        // setup external PCs (connection between core, NI, and router)
        for (unsigned int c=0; c<m_router_external_pc_vec[r].size(); c++) {
            Core* p_Core = m_router_external_pc_vec[r][c];

            // Each bank or core has a single NI to communicate to a network.

            // map PCs between input-NI and router
            int router_in_pc = p_Router->num_internal_pc() + c;
            NIInput* p_NI_Input = p_Core->getNIInput(0);
            p_NI_Input->attachRouter(p_Router, router_in_pc);
            p_Router->appendNIInput(p_NI_Input);
#ifdef _DEBUG_TOPOLOGY_SNUCA
printf("[SNUCA]   router-%d in_pc=%d core=%d:\n", p_Router->id(), router_in_pc, p_Core->id());
#endif

            // map PCs between output-NI and router
            int router_out_pc = p_Router->num_internal_pc() + c;
            NIOutput* p_NI_Output = p_Core->getNIOutput(0);
            p_NI_Output->attachRouter(p_Router, router_out_pc);
            p_Router->appendNIOutput(p_NI_Output);
#ifdef _DEBUG_TOPOLOGY_SNUCA
// printf("[SNUCA]   router-%d out_pc=%d core=%d:\n", p_Core->id(), p_Router->id(), router_out_pc, p_Core->id());
#endif
        }
    }
#ifdef _DEBUG_TOPOLOGY_SNUCA
printf("[SNUCA] # of created routers=%d\n", g_Router_vec.size());
#endif

    // setup link configuration
    for (unsigned int r=0; r<g_Router_vec.size(); r++) {
        Router* p_router = g_Router_vec[r];
        int router_x_coord = getNetXCoord(r);
        int router_y_coord = getNetYCoord(r);

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);

            string link_name_prefix;

            switch (out_pc) {
            case DIR_WEST:
                link_name_prefix = "W";
                if (router_x_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_NORTH:
                link_name_prefix = "N";
                if (router_y_coord > 0)
                    link.m_valid = true;
                break;
            case DIR_EAST:
                link_name_prefix = "E";
                if (router_x_coord < m_num_cols-1)
                    link.m_valid = true;
                break;
            case DIR_SOUTH:
                link_name_prefix = "S";
                if (router_y_coord < m_num_rows-1)
                    link.m_valid = true;
                break;
            default:
                if (out_pc < p_router->num_pc()) {
                    int external_pc = out_pc - p_router->num_internal_pc();
                    assert(external_pc >= 0);
                    link.m_link_name = "P" + int2str(external_pc - 1);
                    link.m_valid = true;
                    link.m_length_mm = 0.0;
                } else {
                    assert(0);
                }
            } // switch (out_pc) {
        } // for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
    }

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
    }
}

int TopologySNUCA::getMinHopCount(int src_router_id, int dst_router_id)
{
    int src_x = src_router_id % m_num_cols;
    int src_y = src_router_id / m_num_cols;
    int dst_x = dst_router_id % m_num_cols;
    int dst_y = dst_router_id / m_num_cols;

    return abs(src_x - dst_x) + abs(src_y - dst_y) + 1;
}

unsigned int TopologySNUCA::getNetXCoord(unsigned int router_id) const
{
    return router_id % m_num_cols;
}

unsigned int TopologySNUCA::getNetYCoord(unsigned int router_id) const
{
    return router_id / m_num_cols;
}

int TopologySNUCA::getRouterID(int mt, int mt_id)
{
    switch (mt) {
    case MachineType_L1Cache:
        assert(mt_id < g_8p_NUCA_L1_num);
        return g_NUCA_L1_map[mt_id];
    case MachineType_L2Cache:
        assert(mt_id < g_8p_NUCA_L2_num);
        return g_NUCA_L2_map[mt_id];
    case MachineType_Directory:
        assert(mt_id < g_8p_NUCA_Dir_num);
        return g_NUCA_Dir_map[mt_id];
    }

    assert(0);	// NEVER REACHED
    return -1;
}

int TopologySNUCA::getCoreID(int mt, int mt_id)
{
    switch (mt) {
    case MachineType_L1Cache:
        assert(mt_id < g_8p_NUCA_L1_num);
        return mt_id + m_num_L2bank;
    case MachineType_L2Cache:
        assert(mt_id < g_8p_NUCA_L2_num);
        return mt_id;
    case MachineType_Directory:
        assert(mt_id < g_8p_NUCA_Dir_num);
        return mt_id + m_num_L2bank + m_num_L1bank;
    }

    assert(0);	// NEVER REACHED
    return -1;
}

int TopologySNUCA::getExternalPC(int mt, int mt_id)
{
    int router_id = getRouterID(mt, mt_id);
    int core_id = getCoreID(mt, mt_id);
    int external_pc_id = -1;

    for (unsigned int n=0; n<m_router_external_pc_vec[router_id].size(); n++) {
        if (core_id == m_router_external_pc_vec[router_id][n]->id()) {
            external_pc_id = n;
            goto EXTERNAL_PC_FOUND;
        }
    }

    assert(0);

    EXTERNAL_PC_FOUND:

    return external_pc_id;
}

void TopologySNUCA::printStats(ostream& out) const
{
}

void TopologySNUCA::print(ostream& out) const
{
}

void config_snucaCMP_network()
{
    // simulation termination condition
    // end_clk is set when reading a trace file is done.
    g_cfg.sim_end_cond = SIM_END_BY_CYCLE;

    // 256 L2 banks, 8 L1 banks, 8 memory controller
    g_cfg.core_num = 256 + 8 + 8;
    g_cfg.router_num = 64;

    // one L2 bank is assumed to be 1mm x 1mm in 45nm.
    // So the required link length is 2mm to exapnd 2 L2 banks.
    g_cfg.link_length = 2.0;    // 2mm
}
