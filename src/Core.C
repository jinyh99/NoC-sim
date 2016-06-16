#include "noc.h"
#include "Core.h"
#include "NIInput.h"
#include "NIInputCompr.h"
#include "NIOutput.h"
#include "NIOutputDecompr.h"

static int g_ni_input_id = 0;
static int g_ni_output_id = 0;

Core::Core()
{
    assert(0);
}

Core::Core(int core_id, int num_NIs)
{
    m_id = core_id;
    m_type = CORE_TYPE_GENERIC;

    for (int ni_pos=0; ni_pos<num_NIs; ni_pos++) {
        NIInput* ni_input = 0;

        if (g_cfg.cam_data_enable)
            ni_input = new NIInputCompr(this, g_ni_input_id, ni_pos, g_cfg.router_num_vc);
        else
            ni_input = new NIInput(this, g_ni_input_id, ni_pos, g_cfg.router_num_vc);

        m_NI_in_vec.push_back(ni_input);
        g_NIInput_vec.push_back(ni_input);

        g_ni_input_id++;
    }

    for (int ni_pos=0; ni_pos<num_NIs; ni_pos++) {
        NIOutput* ni_output = 0;

        if (g_cfg.cam_data_enable)
            ni_output = new NIOutputDecompr(this, g_ni_output_id, ni_pos);
        else
            ni_output = new NIOutput(this, g_ni_output_id, ni_pos);

        m_NI_out_vec.push_back(ni_output);
        g_NIOutput_vec.push_back(ni_output);

        g_ni_output_id++;
    }

    m_total_energy = 0.0;
}

Core::~Core()
{
    for (unsigned int i=0; i<m_NI_in_vec.size(); i++) {
        delete m_NI_in_vec[i];
    }
    m_NI_in_vec.clear();

    for (unsigned int i=0; i<m_NI_out_vec.size(); i++) {
        delete m_NI_out_vec[i];
    }
    m_NI_out_vec.clear();
}

void Core::forwardPkt2NI(int ni_pos, Packet* p_pkt)
{
    assert(ni_pos < (int) m_NI_in_vec.size());
    m_NI_in_vec[ni_pos]->writePacket(p_pkt);
}
