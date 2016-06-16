#include "noc.h"
#include "FlitQ.h"
#include "Flit.h"

FlitQ::FlitQ()
{
    m_num_pc = -1;
    m_num_vc = -1;
    m_depth = -1;
}

FlitQ::FlitQ(int num_pc, int num_vc, int depth)
{
    m_num_pc = num_pc;
    m_num_vc = num_vc;
    m_depth = depth;

    m_Q_vec.resize(num_pc);
    m_max_occupancy_vec.resize(num_pc);
    m_sum_occupancy_vec.resize(num_pc);
    m_last_update_clk_vec.resize(num_pc);
    for (int pc=0; pc<num_pc; pc++) {
        m_Q_vec[pc].resize(num_vc);
        m_max_occupancy_vec[pc].resize(num_vc, 0);
        m_sum_occupancy_vec[pc].resize(num_vc, 0.0);
        m_last_update_clk_vec[pc].resize(num_vc, 0.0);
    }
}

FlitQ::~FlitQ()
{
}

Flit* FlitQ::read(int pc, int vc)
{
    assert(! isEmpty(pc, vc));

    m_sum_occupancy_vec[pc][vc] += m_Q_vec[pc][vc].size()*(simtime() - m_last_update_clk_vec[pc][vc]);
    m_last_update_clk_vec[pc][vc] = simtime();

    Flit* p_flit = m_Q_vec[pc][vc].front();
    m_Q_vec[pc][vc].pop_front();

    return p_flit;
}

void FlitQ::write(int pc, int vc, Flit* p_flit)
{
    assert(! isFull(pc, vc));

    m_sum_occupancy_vec[pc][vc] += m_Q_vec[pc][vc].size()*(simtime() - m_last_update_clk_vec[pc][vc]);
    m_last_update_clk_vec[pc][vc] = simtime();

    m_Q_vec[pc][vc].push_back(p_flit);

    if (m_max_occupancy_vec[pc][vc] < ((int) m_Q_vec[pc][vc].size()))
        m_max_occupancy_vec[pc][vc] = m_Q_vec[pc][vc].size();
}

int FlitQ::size(int pc, int vc)
{
    return m_Q_vec[pc][vc].size();
}

int FlitQ::depth()
{
    return m_depth;
}

bool FlitQ::isEmpty(int pc, int vc)
{
    return (m_Q_vec[pc][vc].size() == 0) ? true : false;
}

bool FlitQ::isFull(int pc, int vc)
{
    switch (g_cfg.router_buffer_type) {
    case ROUTER_BUFFER_SAMQ:
        return (((int) m_Q_vec[pc][vc].size()) < m_depth) ? false : true;
    case ROUTER_BUFFER_DAMQ_P:
      {
        int sum_used = 0;
        for (int vc2=0; vc2<m_num_vc; vc2++)
            sum_used += m_Q_vec[pc][vc2].size();
        return (sum_used < m_num_vc*m_depth) ? false : true;
      }
    case ROUTER_BUFFER_DAMQ_R:
      {
        int sum_used = 0;
        for (int pc2=0; pc2<m_num_pc; pc2++)
        for (int vc2=0; vc2<m_num_vc; vc2++)
            sum_used += m_Q_vec[pc2][vc2].size();
        return (sum_used < m_num_pc*m_num_vc*m_depth) ? false : true;
      }
    default:
        assert(0);
    }

    return false;
}

Flit* FlitQ::peek(int pc, int vc)
{
    assert(! isEmpty(pc, vc));

    return m_Q_vec[pc][vc].front();
}

void FlitQ::print(FILE* fp, int pc, int vc)
{
    // head, ..., tail
    for (deque< Flit* >::iterator pos = m_Q_vec[pc][vc].begin(); pos != m_Q_vec[pc][vc].end(); ++pos) {
        Flit* p_flit = *pos;

        fprintf(fp, "[p=%lld f=%lld (%d->%d) ",
               p_flit->getPkt()->id(),
               p_flit->id(),
               p_flit->getPkt()->getSrcCoreID(),
               p_flit->getPkt()->getDestCoreID());

        switch (p_flit->type()) {
        case HEAD_FLIT: fprintf(fp, "H] "); break;
        case MIDL_FLIT: fprintf(fp, "M] "); break;
        case TAIL_FLIT: fprintf(fp, "T] "); break;
        case ATOM_FLIT: fprintf(fp, "A] "); break;
        default: assert(0);
        }
    }
    fprintf(fp, "\n");
}

void FlitQ::printAll(FILE* fp)
{
    for (int pc=0; pc<m_num_pc; pc++)
    for (int vc=0; vc<m_num_vc; vc++)
        print(fp, pc, vc);
}

double FlitQ::getAvgOccupancy(int pc, int vc, double sim_cycles)
{
    return (sim_cycles <= 0.0) ? 0.0: m_sum_occupancy_vec[pc][vc] / sim_cycles;
}

void FlitQ::resetStats()
{
    for (int pc=0; pc<m_num_pc; pc++)
    for (int vc=0; vc<m_num_vc; vc++) {
        m_last_update_clk_vec[pc][vc] = simtime();
        m_max_occupancy_vec[pc][vc] = 0;
        m_sum_occupancy_vec[pc][vc] = 0.0;
    }
}
