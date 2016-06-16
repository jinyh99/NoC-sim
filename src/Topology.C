#include "noc.h"
#include "Router.h"
#include "Core.h"
#include "Topology.h"

void Topology::createCores()
{
    assert(g_cfg.core_num == g_cfg.router_num);
    for (int n=0; n<g_cfg.core_num; n++) {
        Core* p_Core = new Core(n, g_cfg.core_num_NIs);
        g_Core_vec.push_back(p_Core);

        Router* p_Router = g_Router_vec[n];

        // map PCs between input-NI and router
        for (int ipc=0; ipc<g_cfg.core_num_NIs; ipc++) {
            int router_in_pc = g_Router_vec[n]->num_internal_pc() + ipc;
            p_Core->getNIInput(ipc)->attachRouter(p_Router, router_in_pc);
            p_Router->appendNIInput(p_Core->getNIInput(ipc));
        }

        // map PCs between output-NI and router
        for (int epc=0; epc<g_cfg.core_num_NIs; epc++) {
            int router_out_pc = g_Router_vec[n]->num_internal_pc() + epc;
            p_Core->getNIOutput(epc)->attachRouter(p_Router, router_out_pc);
            p_Router->appendNIOutput(p_Core->getNIOutput(epc));
        }
    }
}
