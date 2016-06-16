#ifndef _NI_OUTPUT_DECOMPR_H_
#define _NI_OUTPUT_DECOMPR_H_

#include "NIOutput.h"

#include "CAMDataDe.h"

class NIOutputDecompr : public NIOutput {
public:
    // Constructors
    NIOutputDecompr();
    NIOutputDecompr(Core* p_Core, int NI_id, int NI_pos);

    // Destructors
    ~NIOutputDecompr();

    void assembleFlit();	// override
    void attachCAM(vector< CAMDataDe* > p_CAM_data_vec) { m_CAM_data_vec = p_CAM_data_vec; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    void decompressDataByCAM(Packet* p_pkt);
    void controlCompression(Packet* p_pkt, const int decode_latency);

private:
    // CAM
    vector< CAMDataDe* > m_CAM_data_vec;
    double m_lastCAM_clk;	// to simulate pipelined CAM
public:
    // for CAM dynamic control
    // m_cont_delay_src_tab_vec[src_router] = per-source contention-delays
    vector< table* > m_cont_delay_src_tab_vec; 
};

// Output operator declaration
ostream& operator<<(ostream& out, const NIOutputDecompr& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const NIOutputDecompr& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
