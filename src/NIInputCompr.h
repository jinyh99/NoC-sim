#ifndef _NI_INPUT_COMPR_H_
#define _NI_INPUT_COMPR_H_

#include "NIInput.h"

#include "CAMDataEn.h"

class NIInputCompr : public NIInput {
public:
    // Constructors
    NIInputCompr();
    NIInputCompr(Core* p_Core, int NI_id, int NI_pos, int num_vc);

    // Destructors
    ~NIInputCompr();

    void segmentPacket(int NI_vc);	// override

    // CAM
    void attachCAM(vector< CAMDataEn* > p_CAM_data_vec) { m_CAM_data_vec = p_CAM_data_vec; };
    CAMDataEn* getCAM(int pos) { return m_CAM_data_vec[pos]; };
    // dynamic CAM management
    void disableCAM(int dest_router_id, int ni_out_id);
    void enableCAM(int dest_router_id, int ni_out_id);
    bool CAMsts(int dest_router_id) { return m_CAM_sts_vec[dest_router_id]; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    // CAM
    void compressDataByCAM(Packet* pkt_ptr, vector< Flit* > & flit_store_vec,
                           vector< int > & cam_en_sts_vec,
                           vector< bool > & flit_en_sts_vec);

private:
    // CAM
    vector< CAMDataEn* > m_CAM_data_vec;
    vector< bool > m_CAM_sts_vec;	// X[dest_router_id] : true(enabled), false(disabled)

    table m_xxx_tab;
};

// Output operator declaration
ostream& operator<<(ostream& out, const NIInputCompr& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const NIInputCompr& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
