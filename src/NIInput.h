#ifndef _NI_INPUT_H_
#define _NI_INPUT_H_

#include "NI.h"

class Packet;
class Flit;

class NIInput : public NI {
public:
    // Constructors
    NIInput();
    NIInput(Core* p_Core, int NI_id, int NI_pos, int num_vc);

    // Destructors
    ~NIInput();

    double getAvgOccupancy(double sim_cycles) {
        return (sim_cycles <= 0.0) ? 0.0 : m_sum_occupancy / sim_cycles; };
    double getMaxOccupancy() { return m_max_occupancy; };
    void writePacket(Packet* p_pkt);
    Packet* readPacket(int NI_vc);
    virtual void segmentPacket(int NI_vc);
    const int pktQ_sz() { return m_pktQ.size(); };
    void resetStats();
    void init();
    void takeSnapshot(FILE* fp);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    vector< Flit* > pkt2flit(Packet* p_pkt);

protected:
    int m_last_free_vc;		// router's input VC for injection port

    deque< Packet* > m_pktQ;	// packet buffer
    deque< Packet* > m_extra_pktQ;	// packet is stored when m_pktQ is full.
    int m_num_pktQ_full;
    vector< event* > m_ev_wakeup_vec;

    unsigned long long m_num_reads;
    unsigned long long m_num_writes;
    int m_max_occupancy;
    double m_sum_occupancy;
    double m_last_update_buf_clk;
    int m_num_vc_router;
    
    vector< bool > m_vc_free_vec;	// X[vc]
};

// Output operator declaration
ostream& operator<<(ostream& out, const NIInput& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const NIInput& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _NI_INPUT_H_
