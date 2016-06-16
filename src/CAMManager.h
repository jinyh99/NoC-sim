#ifndef _CAM_MANAGER_H_
#define _CAM_MANAGER_H_

#include "CAMDataEnPrivate.h"
#include "CAMDataEnShared.h"
#include "CAMDataDePrivate.h"
#include "CAMDataDeShared.h"

/**
 *  <pre>
 *  CAM_MT_PRIVATE_PER_ROUTER
 *  CAM_MT_PRIVATE_PER_CORE
 *    - shared value in source, per-source value in destination
 *    - source-initiated encoding
 *      size of encoding value: log(num_of_per_dest_entries)
 *    - no synchronization problem
 *    - source encoder table entry format
 *      value : encoding_value : freq : valid_bit
 *    - destination encoder table entry format
 *      encoding_value : value
 *  
 *  CAM_MT_SHARED_PER_ROUTER
 *    - shared value in source, shared value in destination
 *    - source-initiated encoding
 *      size of encoding value: log(num_of_per_dest_entries * num_of_PEs)
 *    - synchronization problem
 *      retransmission mechanism required when decoding is failed
 *    - VLB required at destination
 *    - source encoder table
 *      value : encoding_value : freq : valid_bit
 *    - destination encoder table
 *      encoding_value : value : freq : valid_bit : source_bitmap
 *  </pre>
 */

class CAMManager {
public:
    // Constructors
    CAMManager(int cam_mt);

    // Destructors
    ~CAMManager();

    int cam_mt() const { return m_cam_mt; };
    int num_encoders() const { return m_encoder_data_vec.size(); };
    int num_decoders() const { return m_decoder_data_vec.size(); };
    void print_file(FILE* stat_fp);

public:
    int m_pkt_tab_sync;
    int m_pkt_dyn_control;

private:
    void creatPrivatePerRouter();
    void creatPrivatePerCore();
    void creatSharedPerRouter();

    int m_cam_mt;	// CAM management type

    vector< CAMDataEn* > m_encoder_data_vec;
    vector< CAMDataDe* > m_decoder_data_vec;
};

#endif
