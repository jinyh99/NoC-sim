#include "noc.h"
#include "RouterPowerOrion.h"

RouterPowerOrion::RouterPowerOrion(Router* p_router) : RouterPower(p_router)
{
    init();
}

RouterPowerOrion::~RouterPowerOrion()
{
    m_router = 0;

    for (unsigned int out_pc=0; out_pc<m_link_power_vec.size(); out_pc++) {
        delete m_link_power_vec[out_pc];
        m_link_power_vec[out_pc] = 0;
    }

    if (m_xbar_trav_tab)
        delete m_xbar_trav_tab;
}

void RouterPowerOrion::init()
{
    int num_pc = m_router->num_pc();
    int num_vc = m_router->num_vc();

    m_op_buf_rd_vec.resize(num_pc, 0);
    m_op_buf_wr_vec.resize(num_pc, 0);

    FUNC(SIM_router_power_init, &(m_router_info),
                                &(m_router_power));
    SIM_arbiter_init(&(m_arbiter_vc_power_),
                     MATRIX_ARBITER,    // arbiter_model
                     NEG_DFF,           // ff_model
                     num_pc * num_vc,   // req_width
                     0,                 // length
                     NULL);             // SIM_power_array_info
    SIM_arbiter_init(&(m_arbiter_xbar_v1_power_),
                     MATRIX_ARBITER,    // arbiter_model
                     NEG_DFF,           // ff_model
                     num_vc,            // req_width
                     0,                 // length
                     NULL);             // SIM_power_array_info
    SIM_arbiter_init(&(m_arbiter_xbar_p1_power_),
                     MATRIX_ARBITER,    // arbiter_model
                     NEG_DFF,           // ff_model
                     num_pc,            // req_width
                     0,                 // length
                     NULL);             // SIM_power_array_info

/*
printf("info->crossbar_model=%d\n", m_router_info.crossbar_model);
printf("info->n_switch_in=%d\n", m_router_info.n_switch_in);
printf("info->n_switch_out=%d\n", m_router_info.n_switch_out);
printf("info->xb_in_seg=%d\n", m_router_info.xb_in_seg);
printf("info->xb_out_seg=%d\n", m_router_info.xb_out_seg);
printf("info->flit_width=%d\n", m_router_info.flit_width);
printf("info->degree=%d\n", m_router_info.degree);
printf("info->connect_type=%d\n", m_router_info.connect_type);
printf("info->trans_type=%d\n", m_router_info.trans_type);
printf("info->crossbar_in_len=%d\n", m_router_info.crossbar_in_len);
printf("info->crossbar_out_len=%d\n\n", m_router_info.crossbar_out_len);
*/
    SIM_crossbar_init(&(m_router_power.crossbar),
                      MATRIX_CROSSBAR,	// crossbar model
                      num_pc,		// n_switch_in (??)
                      num_pc,		// n_switch_out (??)
                      0,		// xb_in_seg (for segmented xbar) 
                      0,		// xb_in_seg (for segmented xbar)
                      64,		// flit_width (max is 64)
                      0,		// degree - only used by multree crossbar
                      TRANS_GATE,	// crossbar connector type
                      NP_GATE,		// crossbar transmission gate type
                      0,		// crossbar input line length
                      0,		// crossbar output line length
                      NULL);		// req_len

    // buffers
    // int SIM_buf_set_para(SIM_power_array_info_t *info,int is_fifo, u_int n_read_port, u_int n_write_port, u_int n_entry, u_int line_width, int outdrv)
    SIM_buf_set_para(&(m_router_info.in_buf_info),
                     1,			// is_fifo
                     1,                 // n_read_port 
                     1,                 // n_write_port
                     g_cfg.router_inbuf_depth,        // n_entry
                     64,		// line_width 
                     0);		// outdrv
    SIM_array_power_init(&(m_router_info.in_buf_info),
                         &(m_router_power.in_buf));

    m_in_buf_read_vec.resize(m_router->num_pc());
    m_in_buf_write_vec.resize(m_router->num_pc());
    m_xbar_read_vec.resize(m_router->num_pc());
    m_xbar_write_vec.resize(m_router->num_pc());
    m_xbar_input_vec.resize(m_router->num_pc(), 0);
    m_vc_arb_req_vec.resize(m_router->num_pc(), 0);
    m_vc_arb_grant_vec.resize(m_router->num_pc(), 0);
    m_xbar_arb_v1_req_vec.resize(m_router->num_pc(), 0);
    m_xbar_arb_p1_req_vec.resize(m_router->num_pc(), 0);

    for (int pc=0; pc<m_router->num_pc(); pc++) {
        m_in_buf_read_vec[pc].resize(g_cfg.flit_sz_64bit_multiple, 0LL);
        m_in_buf_write_vec[pc].resize(g_cfg.flit_sz_64bit_multiple, 0LL);
        m_xbar_read_vec[pc].resize(g_cfg.flit_sz_64bit_multiple, 0LL);
        m_xbar_write_vec[pc].resize(g_cfg.flit_sz_64bit_multiple, 0LL);
    }

    connectLinkPowerModel();
}

void RouterPowerOrion::record_buffer_read(Flit* p_flit, int pc)
{
    // Orion records 64-bit data read for one function call.

    for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
        FUNC(SIM_buf_power_data_read,
             &(m_router_info.in_buf_info),
             &(m_router_power.in_buf),
             p_flit->m_flitData[i]);

        m_in_buf_read_vec[pc][i] = p_flit->m_flitData[i];
    }

    m_op_buf_rd_vec[pc]++;
}

void RouterPowerOrion::record_buffer_write(Flit* p_flit, int pc)
{
    unsigned long long old_d, new_d;

    // Orion records 64-bit data read for one function call.
    // NOTE: Orion bug
    //   PARM_flit_width in SIM_port.h should be 64.
    //   FUNC(SIM_buf_power_data_write, ...) have N iterations
    //   based on this parameter.
    //   For example, 64 produces 8 iterations.
    //   However, 128 produces 16 iterations, then it causes
    //   data corruption in a loop variable. Finally it makes program hang.

    for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
        old_d = m_in_buf_write_vec[pc][i];
        new_d = p_flit->m_flitData[i];
// printf("pc=%d flit=%d: old_d=%llX new_d=%llX\n", pc, i, old_d, new_d);

        FUNC(SIM_buf_power_data_write,
             &(m_router_info.in_buf_info),
             &(m_router_power.in_buf),
             (u_char*) &old_d,
             (u_char*) &old_d,
             (u_char*) &new_d);

        m_in_buf_write_vec[pc][i] = p_flit->m_flitData[i];
    }

    m_op_buf_wr_vec[pc]++;
}

void RouterPowerOrion::record_xbar_trav(Flit* p_flit, int in_pc, int out_pc)
{
    for (int i=0; i<g_cfg.flit_sz_64bit_multiple; i++) {
        SIM_crossbar_record(&(m_router_power.crossbar),
                            1, // at input port
                            p_flit->m_flitData[i],
                            m_xbar_read_vec[in_pc][i],
                            1,
                            1);
        SIM_crossbar_record(&(m_router_power.crossbar),
                            0, // at output port
                            p_flit->m_flitData[i],
                            m_xbar_write_vec[out_pc][i],
                            m_xbar_input_vec[out_pc],
                            in_pc);
        m_xbar_read_vec[in_pc][i] = p_flit->m_flitData[i];
        m_xbar_write_vec[out_pc][i] = p_flit->m_flitData[i];
        m_xbar_input_vec[out_pc] = in_pc;
    }
}

void RouterPowerOrion::record_vc_arb(int out_pc, unsigned int req, unsigned int grant)
{
    SIM_arbiter_record(&(m_arbiter_vc_power_),
                       req,
                       m_vc_arb_req_vec[out_pc],
                       grant,
                       m_vc_arb_grant_vec[out_pc]);
    m_vc_arb_req_vec[out_pc] = req;
    m_vc_arb_grant_vec[out_pc] = grant;

    m_op_vc_arb++;
}

void RouterPowerOrion::record_v1_sw_arb(int in_pc, unsigned int req, unsigned int grant)
{
    SIM_arbiter_record(&(m_arbiter_xbar_v1_power_),
                       req,
                       m_xbar_arb_v1_req_vec[in_pc],
                       grant,
                       m_xbar_arb_v1_req_vec[in_pc]);
    m_xbar_arb_v1_req_vec[in_pc] = req;
    m_xbar_arb_v1_req_vec[in_pc] = grant;

    m_op_sw_input_arb++;
}

void RouterPowerOrion::record_p1_sw_arb(int out_pc, unsigned int req, unsigned int grant)
{
    SIM_arbiter_record(&(m_arbiter_xbar_p1_power_),
                       req,
                       m_xbar_arb_p1_req_vec[out_pc],
                       grant,
                       m_xbar_arb_p1_req_vec[out_pc]);
    m_xbar_arb_p1_req_vec[out_pc] = req;
    m_xbar_arb_p1_req_vec[out_pc] = grant;

    m_op_sw_output_arb++;
}

void RouterPowerOrion::record_link_trav(Flit* p_flit, int out_pc)
{
    m_link_power_vec[out_pc]->traverse(p_flit);
}

double RouterPowerOrion::dynamicE_buffer()
{
    return SIM_array_power_report(&(m_router_info.in_buf_info),
                                  &(m_router_power.in_buf));
}

double RouterPowerOrion::dynamicE_vc_arb()
{
    return SIM_arbiter_report(&(m_arbiter_vc_power_));
}

double RouterPowerOrion::dynamicE_sw_arb()
{
    return SIM_arbiter_report(&(m_arbiter_xbar_v1_power_))
           + SIM_arbiter_report(&(m_arbiter_xbar_p1_power_));
}

double RouterPowerOrion::dynamicE_xbar()
{
    return SIM_crossbar_report(&(m_router_power.crossbar)) * ((double) g_cfg.flit_sz_64bit_multiple);
}
