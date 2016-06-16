#ifndef _ROUTER_POWER_H_
#define _ROUTER_POWER_H_

#include "Router.h"
#include "LinkPower.h"
#include "LinkPowerOrion.h"
#include "LinkPowerRepeated.h"
#include "LinkPowerRepeatedValue.h"

class Flit;

class RouterPower {
public:
    RouterPower() { assert(0); };
    RouterPower(Router* p_router) {
        assert(p_router);
        m_router = p_router;
        m_xbar_trav_tab = new table("xbar_trav");
    };
    virtual ~RouterPower() {};

    // dynamic energy
    virtual double dynamicE_buffer() = 0;
    virtual double dynamicE_vc_arb() = 0;
    virtual double dynamicE_sw_arb() = 0;
    virtual double dynamicE_xbar() = 0;
    double dynamicE_link(int out_pc) { return m_link_power_vec[out_pc]->dynamicE(); };
    double dynamicE_all_link();

    // static energy
    virtual double staticE_buffer() = 0;
    virtual double staticE_xbar() = 0;
    virtual double staticE_vc_arb() = 0;
    virtual double staticE_xbar_v1_arb() = 0;
    virtual double staticE_xbar_p1_arb() = 0;

    virtual void record_link_trav(Flit* p_flit, int out_pc) = 0;
    virtual void record_buffer_read(Flit* p_flit, int pc) = 0;
    virtual void record_buffer_write(Flit* p_flit, int pc) = 0;
    virtual void record_xbar_trav(Flit* p_flit, int in_pc, int out_pc) = 0;
    void record_xbar_trav_num(int num_traversal) { m_xbar_trav_tab->tabulate((double) num_traversal); };
    virtual void record_vc_arb(int out_pc, unsigned int req, unsigned int grant) = 0;
    virtual void record_v1_sw_arb(int in_pc, unsigned int req, unsigned int grant) = 0;
    virtual void record_p1_sw_arb(int out_pc, unsigned int req, unsigned int grant) = 0;

    // link power model
    void connectLinkPowerModel();
    LinkPower* getLinkPower(int out_pc) const { return m_link_power_vec[out_pc]; };

    // switching activity
    int total_op_buf_rd() { return StatSum(m_op_buf_rd_vec); };
    int total_op_buf_wr() { return StatSum(m_op_buf_wr_vec); };
    double avg_xbar_trav() const { return m_xbar_trav_tab->mean(); };
    double sum_xbar_trav() const { return m_xbar_trav_tab->sum(); };
    double cnt_xbar_trav() const { return m_xbar_trav_tab->cnt(); };

protected:
    virtual void init() = 0;

protected:
    Router* m_router;

    vector< LinkPower* > m_link_power_vec;

    // switching activity measurement
    vector< int > m_op_buf_wr_vec; // X[in_pc] write operations
    vector< int > m_op_buf_rd_vec; // X[in_pc] read operations
    table* m_xbar_trav_tab; // # flit traversals
    int m_op_vc_arb;
    int m_op_sw_input_arb;
    int m_op_sw_output_arb;
};

inline double RouterPower::dynamicE_all_link()
{
    double sum_link_power = 0.0;

    for (int out_pc=0; out_pc<m_router->num_pc(); out_pc++) {
        if (m_router->getLink(out_pc).m_valid) {
            sum_link_power += dynamicE_link(out_pc);
        }
    }

    return sum_link_power;
}

inline void RouterPower::connectLinkPowerModel()
{
    m_link_power_vec.resize(m_router->num_pc(), 0);

    for (int out_pc=0; out_pc<m_router->num_pc(); out_pc++) {
        Link& link = m_router->getLink(out_pc);

        if (link.m_valid) {
            switch (g_cfg.link_power_model) {
            case LINK_POWER_MODEL_ORION:
                m_link_power_vec[out_pc] = new LinkPowerOrion(m_router, out_pc, g_cfg.link_width);
                break;
            case LINK_POWER_MODEL_DELAY_OPT_REPEATED:
                m_link_power_vec[out_pc] = new LinkPowerRepeated(m_router, out_pc, g_cfg.link_width);
                break;
            case LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE:
                m_link_power_vec[out_pc] = new LinkPowerRepeatedValue(m_router, out_pc, g_cfg.link_width);
                break;
            default: 
                assert(0);
            }
        }
    }
}

#endif // #ifndef _ROUTER_POWER_H_
