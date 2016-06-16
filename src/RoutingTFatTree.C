#include "RoutingTFatTree.h"
#include "TopologyFatTree.h"

#define _DEBUG_ROUTING_TTREE

RoutingTFatTree::RoutingTFatTree()
{
}

RoutingTFatTree::~RoutingTFatTree()
{
}

void RoutingTFatTree::init()
{
    m_routing_name = "TFat Tree (upward and downward adaptively)";
    m_topology_tfattree = (TopologyFatTree*) m_topology;
}

int RoutingTFatTree::selectOutPC(Router* p_cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;

    int cur_router_id = p_cur_router->id();
    int cur_router_tree_level = m_topology_tfattree->getTreeLevel(p_cur_router);

    int src_core_id = p_flit->src_core_id();
    int src_router_id = p_flit->src_router_id();

    int dest_core_id = p_flit->dest_core_id();
    int dest_router_id = p_flit->dest_router_id();

    int common_ancestor_router_level = m_topology_tfattree->getCommonAncestorTreeLevel(src_router_id, dest_router_id);

    if (cur_router_id == dest_router_id) {	// destination(current) router is external
        assert(cur_router_tree_level == 0);
        cur_out_pc = p_cur_router->num_internal_pc() + dest_core_id % m_topology_tfattree->way() + p_flit->getPkt()->m_NI_out_pos;

#ifdef _DEBUG_ROUTING_TTREE
        printf("  arrived... core(%d->%d) router(%d->%d cur=%d) cur_out_pc=%d\n", src_core_id, dest_core_id, src_router_id, dest_router_id, cur_router_id, cur_out_pc);
#endif
    } else {
        if (cur_router_tree_level == common_ancestor_router_level) {
            p_flit->m_fattree_up_dir = false;	// change direction to down
#ifdef _DEBUG_ROUTING_TTREE
            printf("  up->down direction changed...\n");
#endif
        }

        if (p_flit->m_fattree_up_dir) {	// up direction (multiple next routers)
            int num_up_out_pc = (int)pow(2.0, double(cur_router_tree_level + 1));
            vector< int > out_pc_vec;
            for (int search_out_pc=0; search_out_pc<num_up_out_pc; search_out_pc++)
                out_pc_vec.push_back(search_out_pc);

            // select one of next_router_vec
            cur_out_pc = selectOutVC(cur_router_id, out_pc_vec);

#ifdef _DEBUG_ROUTING_TTREE
            int next_router_id = p_cur_router->nextRouters()[cur_out_pc].first;
            int next_in_pc = p_cur_router->nextRouters()[cur_out_pc].first;
            printf("  up cur_router=%d cur_out_pc=%d next_router=%d next_in_pc=%d\n", cur_router_id, cur_out_pc, next_router_id, next_in_pc);
#endif
        } else {	// down direction 

            assert(cur_router_tree_level > 0); // current router should be internal.
            int down_router_tree_level = cur_router_tree_level - 1;
// printf("down_router_tree_level=%d\n", down_router_tree_level);
	    vector<pair<int, int> > down_router_vec;
	    int counter=0;
	    bool find_first = false;
            for ( vector< pair< int, int > >::iterator iter= p_cur_router->nextRouters().begin()+p_cur_router->num_internal_pc()/2; iter<p_cur_router->nextRouters().end(); iter++){//find the proper down_router_id and in_pc
		int ancester_level = m_topology_tfattree->getCommonAncestorTreeLevel((*iter).first, dest_router_id);
		if(ancester_level == down_router_tree_level){	
			down_router_vec.push_back((*iter));
			find_first = true;
		}
		
		if(find_first == false){
			counter++;
		}
            }
            int select_pos = selectDownRouter(cur_router_id, down_router_vec);
	    cur_out_pc = counter + select_pos + p_cur_router->num_internal_pc()/2; 

#ifdef _DEBUG_ROUTING_TTREE
            int next_router_id = p_cur_router->nextRouters()[cur_out_pc].first;
            int next_in_pc = p_cur_router->nextRouters()[cur_out_pc].first;
            printf("  down cur_router=%d cur_out_pc=%d next_router=%d next_in_pc=%d\n", cur_router_id, cur_out_pc, next_router_id, next_in_pc);
#endif
        }
    }

    return cur_out_pc;
}

unsigned int RoutingTFatTree::selectOutVC(unsigned int cur_router_id, vector< int > & out_pc_vec) const
{
    // Adaptive selection of out PC
    // cost: the number of reserved VCs
    vector< int > out_pc_cost_vec;
    out_pc_cost_vec.resize(out_pc_vec.size());
    Router* p_cur_router = g_Router_vec[cur_router_id];

    for (unsigned int i=0; i<out_pc_vec.size(); i++) {
        int out_pc = out_pc_vec[i];

        int num_rsv_out_vc = 0;
        for (int out_vc=0; out_vc<p_cur_router->num_vc(); out_vc++) {
            if (p_cur_router->outputModule(out_pc, out_vc).m_state != OUT_MOD_I) {
                num_rsv_out_vc++;
            }
        }

        out_pc_cost_vec[i] = num_rsv_out_vc;
    }

    // select minimum cost output PC
    int min_pos = 0;
    int min_cost = out_pc_cost_vec[0];
    for (unsigned int i=1; i<out_pc_vec.size(); i++) {
        if (out_pc_cost_vec[i] < min_cost) {
            min_pos = i;
            min_cost = out_pc_cost_vec[i];
        }
    }

#ifdef _DEBUG_ROUTING_TTREE
    for (unsigned int i=0; i<out_pc_vec.size(); i++) {
        printf("    up_cost out_pc=%d cost=%d", out_pc_vec[i], out_pc_cost_vec[i]);
        if (i == min_pos) printf(" *");
        printf("\n");
    }
#endif

    return out_pc_vec[min_pos];
}

unsigned int RoutingTFatTree::selectDownRouter(unsigned int cur_router_id, vector< pair<int, int> > & downRouter_vec) const
{
    // Adaptive selection with free VCs
    vector< int > downRouterCost_vec;
    downRouterCost_vec.resize(downRouter_vec.size());

    for (unsigned int i=0; i<downRouter_vec.size(); i++) {
        Router* p_next_router = g_Router_vec[downRouter_vec[i].first];
        int next_router_in_pc = downRouter_vec[i].second;
        Router* p_cur_router = g_Router_vec[cur_router_id];
        int cur_router_out_pc = p_cur_router->prevRouters()[next_router_in_pc].second;

        int num_rsv_out_vc = 0;
        for (int out_vc=0; out_vc<p_cur_router->num_vc(); out_vc++) {
            if (p_cur_router->outputModule(cur_router_out_pc, out_vc).m_state != OUT_MOD_I) {
                num_rsv_out_vc++;
            }
        }

        downRouterCost_vec[i] = num_rsv_out_vc;
    }

    // select minimum cost
    int min_idx = 0;
    int min_cost = downRouterCost_vec[0];
    for (unsigned int i=1; i<downRouter_vec.size(); i++) {
        if (downRouterCost_vec[i] < min_cost) {
            min_idx = i;
            min_cost = downRouterCost_vec[i];
        }
    }

// printf("router[%02d]\n", cur_router_id);
 // for (unsigned int i=0; i<upRouter_vec.size(); i++) { 
  // printf("    up cost up_router=%d in_pc=%d cost=%d", upRouter_vec[i].first, upRouter_vec[i].second, upRouterCost_vec[i]);
  // if (i == min_idx) printf(" *");
  // printf("\n");
// }

    return min_idx;
}

void RoutingTFatTree::printStats(ostream& out) const
{
}

void RoutingTFatTree::print(ostream& out) const
{
}
