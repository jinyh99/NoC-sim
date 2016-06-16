#include "noc.h"
#include "Router.h"
#include "Core.h"
#include "TopologyFatTree.h"

// #define _DEBUG_TOPOLOGY_FTREE

TopologyFatTree::TopologyFatTree()
{
    m_topology_name = "Fat Tree";

    m_num_way = 0;
    m_max_level = 0;
}

TopologyFatTree::~TopologyFatTree()
{
}

void TopologyFatTree::buildTopology()
{
    buildTopology(g_cfg.net_fattree_way);
}

void TopologyFatTree::buildTopology(int num_way)
{
    assert(is_power2(g_cfg.core_num));
    assert(is_power2(num_way));

    m_num_way = num_way;
    m_max_level = log_int(g_cfg.core_num, m_num_way);
  
    m_num_router_per_level = g_cfg.core_num/m_num_way;
    fprintf(stderr, "[Fat Tree] m_num_way=%d m_max_level=%d\n", m_num_way, m_max_level);
    fprintf(stderr, "[Fat Tree] m_num_router_per_level=%d\n", m_num_router_per_level);

    // check relationship between #cores and #routers.
    if (g_cfg.router_num != m_num_router_per_level*m_max_level) {
        fprintf(stderr, "[Fat Tree] g_cfg.core_num=%d g_cfg.router_num=%d #routers_required=%d\n",
               g_cfg.core_num, g_cfg.router_num, m_num_router_per_level*m_max_level);
        g_cfg.router_num = m_num_router_per_level*m_max_level;
        fprintf(stderr, "[Fat Tree] We changed g_cfg.router_num=%d.\n", g_cfg.router_num);
    }

    // topology name
    m_topology_name = int2str(m_num_way) + "-way Fat Tree";

    // create cores
    for (int n=0; n<g_cfg.core_num; n++) {
        Core* p_Core = new Core(n, g_cfg.core_num_NIs);
        g_Core_vec.push_back(p_Core);
    }

    // create routers
    unsigned int router_id = 0;
    for (int l=0; l<m_max_level; l++) {
        for (int w=0; w<m_num_router_per_level; w++) {
            int router_num_pc;
            int router_num_ipc, router_num_epc;

            if (l == 0) {	// 0-level(external) router
                router_num_epc = router_num_ipc = (m_num_way * g_cfg.core_num_NIs);
                router_num_pc = m_num_way + router_num_epc;
            } else {	// internal-level router
                router_num_epc = router_num_ipc = 0;
                router_num_pc = m_num_way * 2;
            }

            Router* pRouter = new Router(router_id, router_num_pc, g_cfg.router_num_vc,
                                         router_num_ipc, router_num_epc, g_cfg.router_inbuf_depth);
#ifdef  _DEBUG_TOPOLOGY_FTREE
printf("[Fat Tree] router=%d level=%d num_pc=%d num_ipc=%d num_epc=%d\n", pRouter->id(), l, router_num_pc, router_num_ipc, router_num_epc);
#endif
            g_Router_vec.push_back(pRouter);
            router_id++;
        }
    }
#ifdef  _DEBUG_TOPOLOGY_FTREE
printf("[Fat Tree] # of created routers=%d\n", g_Router_vec.size());
#endif

    // core and router connection
    int num_cores_in_dim = (int)sqrt(g_cfg.core_num);
    int num_routers_in_dim = (int)sqrt(m_num_router_per_level);
    // printf("num_cores_in_dim = %d, num_routers_in_dim = %d.\n", num_cores_in_dim, num_routers_in_dim);
    for(int n=0; n<m_num_router_per_level; n++){
        int router_x_coord = n % num_routers_in_dim;
        int router_y_coord = n / num_routers_in_dim;

        // FIXME: support only 4-way fat-trees
        vector< int > core_id_vec;
        core_id_vec.resize(m_num_way);
        core_id_vec[0] = 2 * router_x_coord + 2 * num_cores_in_dim * router_y_coord;
        core_id_vec[1] = core_id_vec[0] + 1;
        core_id_vec[2] = core_id_vec[0] + num_cores_in_dim;
        core_id_vec[3] = core_id_vec[2] + 1;

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

#ifdef  _DEBUG_TOPOLOGY_FTREE
printf(" router_id=%d first_core=%d second_core=%d third_core=%d fourth_core=%d\n", n, core_id_vec[0], core_id_vec[1], core_id_vec[2], core_id_vec[3]); 
#endif

        for (int w=0; w<m_num_way; w++) {
            Core * p_Core = g_Core_vec[core_id_vec[w]];
            Router * p_Router = g_Router_vec[n];

            // map PCs between input-NI and router
            for (int ipc=0; ipc<g_cfg.core_num_NIs; ipc++) {
                int router_in_pc = p_Router->num_internal_pc() + w * g_cfg.core_num_NIs + ipc;
                p_Core->getNIInput(ipc)->attachRouter(p_Router, router_in_pc);
                p_Router->appendNIInput(p_Core->getNIInput(ipc));
            }
            // map PCs between output-NI and router
            for (int epc=0; epc<g_cfg.core_num_NIs; epc++) {
                int router_out_pc = p_Router->num_internal_pc() + w * g_cfg.core_num_NIs + epc;
                p_Core->getNIOutput(epc)->attachRouter(p_Router, router_out_pc);
                p_Router->appendNIOutput(p_Core->getNIOutput(epc)); 
            }
        }
    }

    // setup link configuration
    for (unsigned int i=0; i<g_Router_vec.size(); i++) {
        Router* p_router = g_Router_vec[i];
        int router_tree_level = getTreeLevel(p_router);

        for (int out_pc=0; out_pc<p_router->num_pc(); out_pc++) {
            Link& link = p_router->getLink(out_pc);
            link.m_valid = true;

            if (router_tree_level == 0) {	// external routers
                if (out_pc < p_router->num_internal_pc()) {
                    // up link
                    link.m_link_name = "U" + int2str(out_pc);
                    link.m_length_mm *= pow(2.0, (double) (router_tree_level+1));
                    link.m_delay_factor *= 2;
                } else {
                    // link for core
                    int attached_core_id = p_router->id() * m_num_way + (out_pc - p_router->num_internal_pc())/g_cfg.core_num_NIs;
                    link.m_link_name = "C" + int2str(attached_core_id) + "-P" + int2str((out_pc - p_router->num_internal_pc())%g_cfg.core_num_NIs);
                    link.m_length_mm = 0.0;
                }
            } else {	// internal routers
                if (out_pc < p_router->num_pc()/2) {
                    // up link
                    if (router_tree_level < m_max_level-1) {
                        link.m_link_name = "U" + int2str(out_pc);
                        link.m_length_mm *= pow(2.0, (double) (router_tree_level+1));
                        link.m_delay_factor *= pow_int(2, router_tree_level+1);
                    } else {
                        link.m_valid = false;
                    }
                } else {
                    // down link
                    link.m_link_name = "D" + int2str(out_pc - p_router->num_pc()/2);
                    link.m_length_mm *= pow(2.0, (double) (router_tree_level+0));
                    link.m_delay_factor *= pow_int(2, router_tree_level);
                }
            }
        }
    }

    // setup routers
    for (int r=0; r<g_cfg.router_num; r++) {
        Router * p_router = g_Router_vec[r];
        int num_pc = p_router->num_pc();
        int num_internal_pc = p_router->num_internal_pc();
        int level = getTreeLevel(p_router);
        if (level == 0) {//level 0, only need to setup UP connection
            vector< pair< int, int > > connNextRouter_vec;
            connNextRouter_vec.resize(num_pc);
            vector< pair< int, int > > connPrevRouter_vec;
            connPrevRouter_vec.resize(num_pc);
            //UP
            for (int out_pc=0; out_pc<num_pc; out_pc++) {
                int next_router_id;
                int next_in_pc;

                if (out_pc >= num_internal_pc) { // ejection pc ?
                    next_router_id = INVALID_ROUTER_ID;
                    next_in_pc = DIR_INVALID;
                } else { // internal pc
                    int up_router_id_scale = (int)pow((double)m_num_way,(double)(level+1));
                    int up_router_id_base = ((p_router->id() % m_num_router_per_level) / up_router_id_scale) * up_router_id_scale + m_num_router_per_level * (level+1);

                    next_router_id = m_num_router_per_level + p_router->id() + out_pc*(int)pow((double)m_num_way, (double)level);
                    if (next_router_id >= (up_router_id_base + up_router_id_scale))
                        next_router_id -= up_router_id_scale;
                    next_in_pc = g_Router_vec[next_router_id]->num_internal_pc()/2 + out_pc;
                }
                connNextRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
                connPrevRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
            }//pc
            g_Router_vec[r]->setNextRouters(connNextRouter_vec);
            g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
        } else if (level>0 && level< m_max_level-1) {// need to setup UP and DOWN connection
            //for internal router, num_pc = num_internal_pc
            //don't need to separate set ejection pc connection
            vector< pair< int, int > > connNextRouter_vec;
            connNextRouter_vec.resize(num_pc);
            vector< pair< int, int > > connPrevRouter_vec;
            connPrevRouter_vec.resize(num_pc);
            //UP 
            for (int out_pc=0; out_pc<num_internal_pc/2; out_pc++){
                int next_router_id;
                int next_in_pc;

                int up_router_id_scale = (int)pow((double)m_num_way,(double)(level+1));
                int up_router_id_base = ((p_router->id() % m_num_router_per_level) / up_router_id_scale) * up_router_id_scale + m_num_router_per_level * (level+1);

                next_router_id = m_num_router_per_level + p_router->id() + out_pc*(int)pow((double)m_num_way, (double)level);
                if (next_router_id >= (up_router_id_base + up_router_id_scale))
                    next_router_id -= up_router_id_scale;
                next_in_pc = g_Router_vec[next_router_id]->num_internal_pc()/2 + out_pc;
                connNextRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
                connPrevRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
            }//pc
            //DOWN
            for (int out_pc=num_internal_pc/2; out_pc<num_internal_pc; out_pc++){
                // out_pc in for in just used for index, not the real out pc which will be set.
                int next_router_id = INVALID_ROUTER_ID;
                int next_in_pc = INVALID_PC;
                int real_out_pc = INVALID_PC;//this is the real out pc which will be set.
 
                int down_router_id_scale = (int)pow((double)m_num_way,(double)level);
                int down_router_id_base = ((p_router->id() % m_num_router_per_level) / down_router_id_scale) * down_router_id_scale + m_num_router_per_level * (level-1);

                next_router_id = p_router->id() - m_num_router_per_level + (out_pc - num_internal_pc/2) * (int)pow((double)m_num_way, (double)(level-1));
                if (next_router_id >= (down_router_id_base + down_router_id_scale))
                    next_router_id -= down_router_id_scale;
             
                Router * p_next_router = g_Router_vec[next_router_id];
             
                for ( vector< pair< int, int > >::iterator iter= p_next_router->nextRouters().begin(); iter<p_next_router->nextRouters().begin()+num_internal_pc/2; iter++){//find the proper down_router_id and in_pc
                    if((*iter).first == p_router->id()){//find the correct one
                        real_out_pc = (*iter).second;
                        next_in_pc = real_out_pc - num_internal_pc/2;
                        break;
                    }
                }  

                connNextRouter_vec[real_out_pc] = make_pair(next_router_id, next_in_pc);
                connPrevRouter_vec[real_out_pc] = make_pair(next_router_id, next_in_pc);
            }//pc
            g_Router_vec[r]->setNextRouters(connNextRouter_vec);
            g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
        }// internal level 
        else {
            vector< pair< int, int > > connNextRouter_vec;
            connNextRouter_vec.resize(num_internal_pc);
            vector< pair< int, int > > connPrevRouter_vec;
            connPrevRouter_vec.resize(num_internal_pc);
            //DOWN
            //UP set as invalid 
            for (int out_pc=0; out_pc<num_internal_pc; out_pc++) {
                int next_router_id = INVALID_ROUTER_ID;
                int next_in_pc = INVALID_PC;
                if ( out_pc < num_internal_pc/2){//up
                    next_router_id = INVALID_ROUTER_ID;
                    next_in_pc = DIR_INVALID;
                    connNextRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
                    connPrevRouter_vec[out_pc] = make_pair(next_router_id, next_in_pc);
                } else{//down
                    int real_out_pc = INVALID_PC;//this is the real out pc which will be set.

                    int down_router_id_scale = (int)pow((double)m_num_way,(double)level);
                    int down_router_id_base = ((p_router->id() % m_num_router_per_level) / down_router_id_scale) * down_router_id_scale + m_num_router_per_level * (level-1);

                    next_router_id = p_router->id() - m_num_router_per_level + (out_pc - num_internal_pc/2) * (int)pow((double)m_num_way, (double)(level-1));
                    if (next_router_id >= (down_router_id_base + down_router_id_scale))
                        next_router_id -= down_router_id_scale;

                    Router * p_next_router = g_Router_vec[next_router_id];

                    for ( vector< pair< int, int > >::iterator iter= p_next_router->nextRouters().begin(); iter<p_next_router->nextRouters().begin()+num_internal_pc/2; iter++){//find the proper down_router_id and in_pc
                        if((*iter).first == p_router->id()){//find the correct one
                            real_out_pc = (*iter).second;
                            next_in_pc = real_out_pc - num_internal_pc/2;
                            break;
                        }
                    }

                    connNextRouter_vec[real_out_pc] = make_pair(next_router_id, next_in_pc);
                    connPrevRouter_vec[real_out_pc] = make_pair(next_router_id, next_in_pc);
                }
            }//pc
            g_Router_vec[r]->setNextRouters(connNextRouter_vec);
            g_Router_vec[r]->setPrevRouters(connPrevRouter_vec);
        }// top level 
    }//router 
}

int TopologyFatTree::getMinHopCount(int src_router_id, int dst_router_id)
{
    int CA_level = getCommonAncestorTreeLevel(src_router_id, dst_router_id);

    return CA_level*2 + 1;
}

unsigned int TopologyFatTree::getTreeLevel(Router* p_router)
{
    return p_router->id() / m_num_router_per_level;
}

unsigned int TopologyFatTree::getIndexOnLevel(Router* p_router)
{
    return p_router->id() % m_num_router_per_level;
}

int TopologyFatTree::getCommonAncestorTreeLevel(int router_id1, int router_id2)
{
    // router_id1 and router_id2 must be in level 0.
    assert((router_id1/m_num_router_per_level) == 0);
    assert((router_id2/m_num_router_per_level) == 0);

    int ancestor_id1 = router_id1;
    int ancestor_id2 = router_id2;
    int reduce_sz = 1;
    int CA_level = 0;	// tree level for the common ancestor

    while (ancestor_id1 != ancestor_id2) {
        reduce_sz *= m_num_way;
        CA_level++;
        assert(CA_level <= m_max_level);

        ancestor_id1 = router_id1 / reduce_sz;
        ancestor_id2 = router_id2 / reduce_sz;
    }

    return CA_level;
}

void TopologyFatTree::printStats(ostream& out) const
{
}

void TopologyFatTree::print(ostream& out) const
{
}

