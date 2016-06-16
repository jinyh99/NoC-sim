#ifndef _ROUTER_H_
#define _ROUTER_H_

#include "noc.h"
#include "FlitQ.h"
#include "NIInput.h"
#include "NIOutput.h"

enum ROUTER_IN_MOD_STATE {
    IN_MOD_I = 0,	// invalid
    IN_MOD_R,		// RC done
    IN_MOD_V,		// VA done
    IN_MOD_S,		// SA done for a head flit
};

enum ROUTER_OUT_MOD_STATE {
    OUT_MOD_I = 0,	// invalid
    OUT_MOD_V,		// VA done for a head flit
};

class RouterInputModule {
public:
    int m_state;
    int m_out_pc;	// mapped output PC
    int m_out_vc;	// mapped output VC
};

class RouterOutputModule {
public:
    int m_state;

    // credit
    // SAMQ:
    //   - It uses only m_num_credit.
    // DAMQ:
    //   - DAMQ must reserve at least one credit for each VC.
    //   - m_num_credit : shared credits
    //   - m_num_credit_rsv : reserved credits
    int m_num_credit;
    int m_num_credit_rsv;
};

class RouterTunnelModule {
public:
    int m_in_vc;
    int m_out_pc;		// OUTPORT
    int m_out_vc;
    pair< int, int > m_flow;	// FLOW
    int m_dest_router_id;	// DEST
};

class VCArb;
class SwArb;

// P x P crossbar
class XBar {
public:
    /// allocated input VCs (for crossbar traversal) for each input PC
    vector< int > m_waiting_in_vc_vec;	// m_waiting_in_vc_vec[in_pc] = in_vc

    /// output port status
    vector< bool > m_outport_free_vec;  // m_outport_free_vec[out_pc];
};

class LinkPower;

class Link {
public:
    bool m_valid;		// valid flag (no link if false)
    string m_link_name;
    int m_delay_factor;		// link_delay = m_delay_factor * Config.link_latency (#cycles)
    double m_length_mm;		// length (mm)

    // wire pipelining
    // element format: < flit_pointer, < next_in_vc, store_clk > >
    //        next_in_vc: updated at ST stage and used at LT stage
    //        store_clk: clock when a flit starts to cross this link
    deque< pair< Flit*, pair< int, double > > > m_w_pipeline;

#ifdef LINK_DVS
    double m_link_expected_utilz;
    double m_dvs_freq;
    double m_dvs_voltage;
    double m_last_sent_clk;	// timestamp for used last
    double m_dvs_freq_set_clk;
#endif
};

class RouterPower;

class Router {
public:
    // Constructors
    Router();
    Router(int router_id, int num_pc, int num_vc, int num_ipc, int num_epc, int inbuf_depth);

    // Destructors
    virtual ~Router();

    // execute each stage of pipeline for one cycle
    virtual void router_sim();

    int id() { return m_id; };
    int num_pc() { return m_num_pc; };
    int num_vc() { return m_num_vc; };
    int num_ipc() { return m_num_ipc; };
    int num_epc() { return m_num_epc; };
    int num_internal_pc() { return m_num_pc - m_num_ipc; };
    bool isEjectChannel(int out_pc);
    bool isInjectChannel(int in_pc);

    /// reset statistics when simulation warmup is done.
    void resetStats();

    // buffer
    FlitQ* flitQ() { return m_flitQ; };

    // link
    Link& getLink(int out_pc) { return m_link_vec[out_pc]; };
    unsigned long long getTotalLinkOp() const;

    // xbar
    XBar& getXBar() { return m_xbar; }; const

    VCArb* getVCArb() { return m_vc_arb; }; const
    SwArb* getSwArb() { return m_sw_arb; }; const

    // NI
    void appendNIInput(NIInput* ni_input) { m_ni_input_vec.push_back(ni_input); };
    void appendNIOutput(NIOutput* ni_output) { m_ni_output_vec.push_back(ni_output); };
    NIInput* getNIInput(int ni_id) { return m_ni_input_vec[ni_id]; };
    NIOutput* getNIOutput(int ni_id) { return m_ni_output_vec[ni_id]; };
    vector< NIInput* > getNIInputVec() { return m_ni_input_vec; };
    vector< NIOutput* > getNIOutputVec() { return m_ni_output_vec; };

    // credit management
    void depositCredit(Credit* p_credit);
    void decCredit(int out_pc, int out_vc, int num_credits);
    void incCredit();
    bool hasCredit(int out_pc, int out_vc, int num_credits=1);

    // connection to neighbor routers
    void setNextRouters(vector< pair< int, int > > & connNextRouter_vec) {
        assert(((int) connNextRouter_vec.size()) == m_num_pc);
        m_connNextRouter_vec = connNextRouter_vec;
    }
    void setPrevRouters(vector< pair< int, int > > & connPrevRouter_vec) {
        assert(((int) connPrevRouter_vec.size()) == m_num_pc);
        m_connPrevRouter_vec = connPrevRouter_vec;
    }
    vector< pair< int, int > > & nextRouters() { return m_connNextRouter_vec; };
    vector< pair< int, int > > & prevRouters() { return m_connPrevRouter_vec; };

    RouterInputModule & inputModule(int in_pc, int in_vc) { return m_in_mod_vec[in_pc][in_vc]; };
    RouterOutputModule & outputModule(int out_pc, int out_vc) { return m_out_mod_vec[out_pc][out_vc]; };

    // power
    void attachPowerModel();

    // fast simulation
    void incFlitsInside() { m_num_flits_inside++; };
    const bool hasNoFlitsInside() { return (m_num_flits_inside==0) ? true : false; };
    const bool hasNoCreditDepositsInside() { return (m_credit_deposit_vec.size()==0) ? true : false; };
    void sleep() { m_ev_wakeup->wait(); };
    void wakeup() { m_ev_wakeup->set(); };

    // tunnel(pipeline bypass)
    const unsigned long long numPipelineBypass(int in_pc, int in_vc) { return m_num_tunnel_flit_vec[in_pc][in_vc]; };

    // debugging
    ////////////////////////////////////////////////////////////////////
    void debugStage(Flit* p_flit, const char* stage, int in_pc, int in_vc);
    void debugIB(Flit* p_flit, int in_pc, int in_vc);
    void debugRC(Flit* p_flit, int next_router_id, int in_pc, int in_vc, int out_pc, int next_in_pc);
    void debugVA(Flit* p_flit, int in_pc, int in_vc, int out_pc, int out_vc, bool is_precompute);
    void debugSA(Flit* p_flit, int xbar_in_pc, int xbar_in_vc, int xbar_out_pc, int xbar_out_vc, bool is_speculative, bool is_precompute);
    void debugST(Flit* p_flit, int xbar_in_pc, int xbar_out_pc, int next_router_id, int next_in_pc, int next_in_vc, bool isLastRouter);
    void debugLT(Flit* p_flit, int out_pc);
    void debugEX(Flit* p_flit, int eject_pc, int eject_vc, NIOutput* NI_out);
    void debugTN(Flit* p_flit, int in_pc, int in_vc, int out_pc, int out_vc);

#ifdef _DEBUG_CHECK_BUFFER_INTEGRITY
    vector< vector< unsigned long long > > m_debug_pkt_chk_mid_vec; // X[in_pc][in_vc]
    vector< vector< unsigned long long > > m_debug_pkt_chk_fid_vec; // X[in_pc][in_vc]

    bool checkBufferIntegrity(int pc, int vc, Flit* p_flit);
#endif // _DEBUG_CHECK_BUFFER_INTEGRITY

#ifdef _DEBUG_ROUTER_SNAPSHOT
    void takeSnapshot(FILE* fp);
#endif
    ////////////////////////////////////////////////////////////////////

protected:
    // pipeline stages
    void stageRC();
    void stageVA();
    void stageSSA();
    void stageSA();
    void stageST();
    void stageLT();
    void stageTN();

protected:
    int m_id;	// router id

    // e.g. mesh, num_pc=5 : 4 neighbors(0, 1, 2, 3) + 1 injection/ejection (4)
    int m_num_pc;	// # PCs
    int m_num_vc;	// # VCs
    int m_inbuf_depth;

    // NOTE: m_num_ipc should be the same as m_num_epc.
    int m_num_ipc;	// # injection PCs
    int m_num_epc;	// # ejection PCs

    // flit input buffers
    FlitQ* m_flitQ;

    // input modules
    vector< vector< RouterInputModule > > m_in_mod_vec;	// X[in_pc][in_vc]
    // output modules
    vector< vector< RouterOutputModule > > m_out_mod_vec; // X[out_pc][out_vc]

    // inter-router connection:
    // NOTE: Topology class updates two structures by calling buildTopology().
    /// [indexed output PC](downstream router ID, input PC)
    vector< pair< int, int > > m_connNextRouter_vec;	// m_connNextRouter_vec[out_pc] = (next_router_id, next_in_pc)
    /// [indexed input PC](upstream router ID, output PC)
    vector< pair< int, int > > m_connPrevRouter_vec;	// m_connPrevRouter_vec[in_pc] = (prev_router_id, prev_out_pc)

    /// VC arbiter
    VCArb* m_vc_arb;

    /// switch arbiter
    SwArb* m_sw_arb;

    /// crossbar
    XBar m_xbar;

    /// link
    vector< Link > m_link_vec;	// m_link_vec[out_pc]

    // credit delay
    // NOTE: Elements in m_credit_deposit_vec are stored in a non-decreasing order of m_clk_deposit.
    vector< Credit* > m_credit_deposit_vec;	// credits to be deposited later

    /// NI
    vector< NIInput* > m_ni_input_vec;
    vector< NIOutput* > m_ni_output_vec;

    // fast simulation
    int m_num_flits_inside;	// #flits inside the router
    event *m_ev_wakeup;		// wakeup event of router

    // tunneling (pipeline bypass)
    vector< RouterTunnelModule > m_tunnel_info_vec;	// X[in_pc]
    vector< vector< unsigned long long > > m_num_tunnel_flit_vec;	// # bypassed flits for each input VC/PC

public:
    // power model
    RouterPower* m_power_tmpl;
    RouterPower* m_power_tmpl_profile;

    // latency for each stage
    table* m_pipe_lat_LT_tab;	// link traversal
    table* m_pipe_lat_ST_tab;	// switch traversal
    table* m_pipe_lat_SA_tab;	// switch allocation
    table* m_pipe_lat_SSA_tab;	// speculative switch allocation
    table* m_pipe_lat_VA_tab;	// VC allocation
    table* m_pipe_lat_RC_tab;	// routing
#ifdef LINK_DVS
    // link usage for an interval
    vector< unsigned long long > m_sim_pc_dvs_link_op_vec;	// X[out_pc]
#endif  // #ifdef LINK_DVS

    // intra-router flit latency
    table* m_flit_lat_router_tab;

    // router load calculation (#injected pkts/flits)
    unsigned long long m_num_pkt_inj_from_core;		// from cores
    unsigned long long m_num_flit_inj_from_core;	// from cores
    unsigned long long m_num_pkt_inj_from_router;	// from upstream routers
    unsigned long long m_num_flit_inj_from_router;	// from upstream routers
};  // class Router
////////////////////////////////////////////////////////////////////

#endif // #ifndef _ROUTER_H_
