#include "RoutingFatTree.h"
#include "TopologyFatTree.h"

// #define _DEBUG_ROUTING_FTREE

RoutingFatTree::RoutingFatTree()
{
}

RoutingFatTree::~RoutingFatTree()
{
    for (unsigned int i=0; i<m_stream_UpSelect_vec.size(); i++)
        delete m_stream_UpSelect_vec[i];
    m_stream_UpSelect_vec.clear();
}

void RoutingFatTree::init()
{
    switch (g_cfg.net_routing) {
    case ROUTING_FAT_TREE_ADAPTIVE:
        m_routing_name = "Fat Tree (upward adaptively & downward deterministically)";
        break;
    case ROUTING_FAT_TREE_RANDOM:
        m_routing_name = "Fat Tree (upward adaptively & downward randomly)";
        for (int i=0; i<g_cfg.router_num; i++)
            m_stream_UpSelect_vec.push_back(new stream);
        break;
    default:
        assert(0);
    }

    m_topology_ftree = (TopologyFatTree*) m_topology;
}

int RoutingFatTree::selectOutPC(Router* p_cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;		// return value

    int cur_router_id = p_cur_router->id();
    int cur_router_tree_level = m_topology_ftree->getTreeLevel(p_cur_router);

    int src_core_id = p_flit->src_core_id();
    int src_router_id = p_flit->src_router_id();

    int dest_core_id = p_flit->dest_core_id();
    int dest_router_id = p_flit->dest_router_id();

    int common_ancestor_router_level = m_topology_ftree->getCommonAncestorTreeLevel(src_router_id, dest_router_id);

    if (cur_router_id == dest_router_id) {
        // destination(current) router is external
        assert(cur_router_tree_level == 0);
        cur_out_pc = p_cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
#ifdef _DEBUG_ROUTING_FTREE
        printf("  arrived... core(%d->%d) router(%d->%d cur=%d) cur_out_pc=%d\n", src_core_id, dest_core_id, src_router_id, dest_router_id, cur_router_id, cur_out_pc);
#endif
    } else {
        if (cur_router_tree_level == common_ancestor_router_level) {
            p_flit->m_fattree_up_dir = false;	// change direction to down
#ifdef _DEBUG_ROUTING_FTREE
            printf("  up->down direction changed...\n");
#endif
        }

        if (p_flit->m_fattree_up_dir) {	// up direction (multiple next routers)
            int num_up_out_pc = m_topology_ftree->way();
            vector< int > out_pc_vec;
            for (int search_out_pc=0; search_out_pc<num_up_out_pc; search_out_pc++)
                out_pc_vec.push_back(search_out_pc);

            // select out_pc in out_pc_vec
            cur_out_pc = selectUpOutVC(cur_router_id, out_pc_vec);

#ifdef _DEBUG_ROUTING_FTREE
            int next_router_id = p_cur_router->nextRouters()[cur_out_pc].first;
            int next_in_pc = p_cur_router->nextRouters()[cur_out_pc].first;
            printf("  up cur_router=%d cur_out_pc=%d next_router=%d next_in_pc=%d\n", cur_router_id, cur_out_pc, next_router_id, next_in_pc);
#endif

        } else {	// down direction (single next router)
            assert(cur_router_tree_level > 0); // current router should be internal.
            int down_router_tree_level = cur_router_tree_level - 1;
            int stride = (int)pow((double)m_topology_ftree->way(), (double)down_router_tree_level);
            int leftmost_child_router_id = (dest_router_id / stride) * stride + m_topology_ftree->num_router_per_level() * down_router_tree_level;
            int rightmost_child_router_id = leftmost_child_router_id + stride - 1;
            int down_router_id = INVALID_ROUTER_ID;

            // find the proper out_pc
            for ( vector< pair< int, int > >::iterator iter=p_cur_router->nextRouters().begin()+p_cur_router->num_internal_pc()/2; iter<p_cur_router->nextRouters().end(); iter++){
                if((*iter).first >= leftmost_child_router_id && (*iter).first <= rightmost_child_router_id){// between leftmost and rightmost ?
                    down_router_id = (*iter).first;
                    cur_out_pc = (*iter).second + p_cur_router->num_internal_pc()/2;
#ifdef _DEBUG_ROUTING_FTREE
                    printf("  cur_out_pc=%d down_router_id=%d leftmost=%d rightmost=%d stride=%d\n", cur_out_pc, (*iter).first, leftmost_child_router_id, rightmost_child_router_id, stride);
#endif
                    break;
                }
            }
            assert(down_router_id != INVALID_ROUTER_ID);
            assert(cur_out_pc < p_cur_router->num_pc());

#ifdef _DEBUG_ROUTING_FTREE
            int next_router_id = p_cur_router->nextRouters()[cur_out_pc].first;
            int next_in_pc = p_cur_router->nextRouters()[cur_out_pc].first;
            printf("  down cur_router=%d cur_out_pc=%d next_router=%d next_in_pc=%d\n", cur_router_id, cur_out_pc, next_router_id, next_in_pc);
#endif

        }
    }

    return cur_out_pc;
}

unsigned int RoutingFatTree::selectUpOutVC(unsigned int cur_router_id, vector< int > & up_out_pc_vec) const
{
    // Random selection
    if (g_cfg.net_routing == ROUTING_FAT_TREE_RANDOM)
        return up_out_pc_vec[m_stream_UpSelect_vec[cur_router_id]->random(0L, up_out_pc_vec.size()-1)];

    assert(g_cfg.net_routing == ROUTING_FAT_TREE_ADAPTIVE);
    // Adaptive selection of out PC
    // cost: the number of reserved VCs
    vector< int > up_out_pc_cost_vec;
    up_out_pc_cost_vec.resize(up_out_pc_vec.size());
    Router* p_cur_router = g_Router_vec[cur_router_id];

    for (unsigned int i=0; i<up_out_pc_vec.size(); i++) {
        int out_pc = up_out_pc_vec[i];

        int num_rsv_out_vc = 0;
        for (int out_vc=0; out_vc<p_cur_router->num_vc(); out_vc++) {
            if (p_cur_router->outputModule(out_pc, out_vc).m_state != OUT_MOD_I) {
                num_rsv_out_vc++;
            }
        }

        up_out_pc_cost_vec[i] = num_rsv_out_vc;
    }

    // select minimum cost output PC
    int min_pos = 0;
    int min_cost = up_out_pc_cost_vec[0];
    for (unsigned int i=1; i<up_out_pc_vec.size(); i++) {
        if (up_out_pc_cost_vec[i] < min_cost) {
            min_pos = i;
            min_cost = up_out_pc_cost_vec[i];
        }
    }

#ifdef _DEBUG_ROUTING_FTREE
    for (unsigned int i=0; i<up_out_pc_vec.size(); i++) { 
        printf("    up_cost out_pc=%d cost=%d", up_out_pc_vec[i], up_out_pc_cost_vec[i]);
        if (i == min_pos) printf(" *");
        printf("\n");
    }
#endif

    return up_out_pc_vec[min_pos];
}

void RoutingFatTree::printStats(ostream& out) const
{
}

void RoutingFatTree::print(ostream& out) const
{
}
