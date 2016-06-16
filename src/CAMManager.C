#include "CAMManager.h"
#include "NIInputCompr.h"
#include "NIOutputDecompr.h"
#include "Router.h"
#include "Core.h"

CAMManager::CAMManager(int cam_mt)
{
    m_cam_mt = cam_mt;

    switch (cam_mt) {
    case CAM_MT_PRIVATE_PER_ROUTER:
        creatPrivatePerRouter();
        break;
    case CAM_MT_PRIVATE_PER_CORE:
        creatPrivatePerCore();
        break;
    case CAM_MT_SHARED_PER_ROUTER:
        creatSharedPerRouter();
        break;
    default:
        assert(0);
    }

    m_pkt_tab_sync = 0;
    m_pkt_dyn_control = 0;
}

CAMManager::~CAMManager()
{
    for (unsigned int i=0; i<m_encoder_data_vec.size(); i++) {
        delete m_encoder_data_vec[i];
        m_encoder_data_vec[i] = 0;
    }

    for (unsigned int i=0; i<m_decoder_data_vec.size(); i++) {
        delete m_decoder_data_vec[i];
        m_decoder_data_vec[i] = 0;
    }
}

void CAMManager::creatPrivatePerRouter()
{
    int num_cam = g_cfg.router_num * g_cfg.router_num;

    // create encoders
    m_encoder_data_vec.resize(num_cam);
    for (int r=0; r<num_cam; r++) {
        CAMDataEn* cam_data_ptr = new CAMDataEnPrivate();
        cam_data_ptr->alloc(r, 
                            (CAMReplacePolicy) g_cfg.cam_repl_policy,
                            g_cfg.cam_data_en_num_sets,
                            g_cfg.cam_data_blk_byte);
        m_encoder_data_vec[r] = cam_data_ptr;
    }

    for (int r_src=0; r_src<g_cfg.router_num; r_src++) {
        vector< NIInput* > NIInput_vec = g_Router_vec[r_src]->getNIInputVec(); 
        vector< CAMDataEn* > en_cam_vec;
        for (int r_dest=0; r_dest<g_cfg.router_num; r_dest++)
            en_cam_vec.push_back(m_encoder_data_vec[r_src*g_cfg.router_num + r_dest]);

        for (unsigned int n=0; n<NIInput_vec.size(); n++)
            ((NIInputCompr*) NIInput_vec[n])->attachCAM(en_cam_vec);
    }

    // create decoders
    m_decoder_data_vec.resize(num_cam);
    for (int r=0; r<num_cam; r++) {
        CAMDataDe* cam_data_ptr = new CAMDataDePrivate();
        cam_data_ptr->alloc(r, 
                            (CAMReplacePolicy) g_cfg.cam_repl_policy,
                            g_cfg.cam_data_de_num_sets,
                            g_cfg.cam_data_blk_byte,
                            g_cfg.cam_VLB_num_sets);
        m_decoder_data_vec[r] = cam_data_ptr;
    }

    for (int r_dest=0; r_dest<g_cfg.router_num; r_dest++) {
        vector< NIOutput* > NIOutput_vec = g_Router_vec[r_dest]->getNIOutputVec(); 
        vector< CAMDataDe* > de_cam_vec;
        for (int r_src=0; r_src<g_cfg.router_num; r_src++)
            de_cam_vec.push_back(m_decoder_data_vec[r_dest*g_cfg.router_num + r_src]);

        for (unsigned int n=0; n<NIOutput_vec.size(); n++)
            ((NIOutputDecompr*) NIOutput_vec[n])->attachCAM(de_cam_vec);
    }
}

void CAMManager::creatPrivatePerCore()
{
  assert(0);
/*
  // for encoder
  m_encoder_data_vec.resize(g_cfg.core_num);

  for (int c=0; c<g_cfg.core_num; c++) {
    // create data CAM
    CAMDataEn* cam_data_ptr = new CAMDataEnPrivate();
    cam_data_ptr->alloc(c, 
                        (CAMReplacePolicy) g_cfg.cam_repl_policy,
                        g_cfg.cam_data_en_num_sets,
                        g_cfg.cam_data_blk_byte);
    m_encoder_data_vec[c] = cam_data_ptr;

    // attach data CAM
    vector <NIInput*> NIInput_vec = g_Core_vec[c]->getNIInputVec(); 
    for (unsigned int n=0; n<NIInput_vec.size(); n++)
      NIInput_vec[n]->attachCAM(m_encoder_data_vec[c]);
  }

  // for decoder
  m_decoder_data_vec.resize(g_cfg.core_num);

  for (int c=0; c<g_cfg.core_num; c++) {
    // create data CAM
    CAMDataDe* cam_data_ptr = new CAMDataDePrivate();
    cam_data_ptr->alloc(c, 
                        (CAMReplacePolicy) g_cfg.cam_repl_policy,
                        g_cfg.cam_data_de_num_sets,
                        g_cfg.cam_data_blk_byte);
    m_decoder_data_vec[c] = cam_data_ptr;

    // attach data CAM
    vector <NIOutput*> NIOutput_vec = g_Core_vec[c]->getNIOutputVec(); 
    for (unsigned int n=0; n<NIOutput_vec.size(); n++)
      NIOutput_vec[n]->attachCAM(m_decoder_data_vec[c]);
  }
*/
}

void CAMManager::creatSharedPerRouter()
{
    // create encoders
    m_encoder_data_vec.resize(g_cfg.router_num);

    for (int r=0; r<g_cfg.router_num; r++) {
        CAMDataEnShared* cam_data_ptr = new CAMDataEnShared();
        cam_data_ptr->set_num_peer(g_cfg.router_num);
        cam_data_ptr->alloc(r, 
                            (CAMReplacePolicy) g_cfg.cam_repl_policy,
                            g_cfg.cam_data_en_num_sets,
                            g_cfg.cam_data_blk_byte);
        m_encoder_data_vec[r] = cam_data_ptr;
    }

    for (int c=0; c<g_cfg.core_num; c++) {
        vector <NIInput*> NIInput_vec = g_Core_vec[c]->getNIInputVec(); 

        for (unsigned int n=0; n<NIInput_vec.size(); n++) {
            vector< CAMDataEn* > en_cam_vec;
            // attached router id
            int rid = NIInput_vec[n]->getRouter()->id();

            en_cam_vec.push_back(m_encoder_data_vec[rid]);

            // attach data CAM
            ((NIInputCompr*) NIInput_vec[n])->attachCAM(en_cam_vec);
        }
    }

    // create decoders
    m_decoder_data_vec.resize(g_cfg.router_num);

    for (int r=0; r<g_cfg.router_num; r++) {
        CAMDataDeShared* cam_data_ptr = new CAMDataDeShared();
        cam_data_ptr->set_num_peer(g_cfg.router_num);
        cam_data_ptr->alloc(r, 
                            (CAMReplacePolicy) g_cfg.cam_repl_policy,
                            g_cfg.cam_data_de_num_sets,
                            g_cfg.cam_data_blk_byte,
                            g_cfg.cam_VLB_num_sets);
        m_decoder_data_vec[r] = cam_data_ptr;
    }

    for (int c=0; c<g_cfg.core_num; c++) {
        vector <NIOutput*> NIOutput_vec = g_Core_vec[c]->getNIOutputVec(); 

        for (unsigned int n=0; n<NIOutput_vec.size(); n++) {
            vector< CAMDataDe* > de_cam_vec;
            // attached router id
            int rid = NIOutput_vec[n]->getRouter()->id();

            de_cam_vec.push_back(m_decoder_data_vec[rid]);

            // attach data CAM
            ((NIOutputDecompr*) NIOutput_vec[n])->attachCAM(de_cam_vec);
        }
    }
}

void CAMManager::print_file(FILE* stat_fp)
{
    int num_data_en = m_encoder_data_vec.size();
    int num_data_de = m_decoder_data_vec.size();

    // Data encoder
    vector< int > data_en_hit_vec;
    vector< int > data_en_miss_vec;
    vector< int > data_en_hit_zero_vec;
    vector< int > data_en_access_vec;
    vector< double > data_en_hit_rate_vec;

    vector< int > data_de_access_vec;
    vector< int > data_de_hit_vec;
    vector< int > data_de_miss_vec;
    vector< int > data_de_hit_zero_vec;
    vector< int > data_de_replace_req_vec;		// for CAMDataEnShared
    vector< int > data_de_VLB2CAM_move_vec;
    vector< double > data_de_hit_rate_vec;

    data_en_hit_vec.resize(num_data_en);
    data_en_miss_vec.resize(num_data_en);
    data_en_hit_zero_vec.resize(num_data_en);
    data_en_access_vec.resize(num_data_en);
    data_en_hit_rate_vec.resize(num_data_en);
    for (int i=0; i<num_data_en; i++) {
        data_en_hit_vec[i] = m_encoder_data_vec[i]->hits();
        data_en_miss_vec[i] = m_encoder_data_vec[i]->misses();
        data_en_hit_zero_vec[i] = m_encoder_data_vec[i]->hits_zero();
        data_en_access_vec[i] = m_encoder_data_vec[i]->accesses();
        data_en_hit_rate_vec[i] = m_encoder_data_vec[i]->hit_rate();
    }

    data_de_access_vec.resize(num_data_de);
    data_de_hit_vec.resize(num_data_de);
    data_de_miss_vec.resize(num_data_de);
    data_de_hit_zero_vec.resize(num_data_de);
    data_de_hit_rate_vec.resize(num_data_de);
    data_de_replace_req_vec.resize(num_data_de, 0);
    data_de_VLB2CAM_move_vec.resize(num_data_de, 0);
    for (int i=0; i<num_data_de; i++) {
        data_de_access_vec[i] = m_decoder_data_vec[i]->accesses();

        CAMDataDeShared* SCAM_ptr = (dynamic_cast<CAMDataDeShared*> (m_decoder_data_vec[i]));
        if (SCAM_ptr) {
            data_de_hit_vec[i] = SCAM_ptr->hits();
            data_de_miss_vec[i] = SCAM_ptr->misses();
            data_de_hit_zero_vec[i] = SCAM_ptr->hits_zero();
            data_de_hit_rate_vec[i] = SCAM_ptr->hit_rate();
            data_de_replace_req_vec[i] = SCAM_ptr->replace_reqs();
            data_de_VLB2CAM_move_vec[i] = SCAM_ptr->VLB2CAM_moves();
        }
    }

    fprintf(stat_fp, "Compression control packet:\n");
    fprintf(stat_fp, "  #pkt_tab_sync: %d\n", m_pkt_tab_sync);
    fprintf(stat_fp, "  #pkt_dyn_control: %d\n", m_pkt_dyn_control);

    fprintf(stat_fp, "Data Encoder:\n");
    fprintf(stat_fp, "  #encoders: %d\n", num_data_en);
    fprintf(stat_fp, "  Den_total_hits: %d zero_hits: %d (%.2lf%%)\n", StatSum(data_en_hit_vec),
            StatSum(data_en_hit_zero_vec),
            ((double) StatSum(data_en_hit_zero_vec))/StatSum(data_en_hit_vec)*100.0);
    fprintf(stat_fp, "  Den_total_misses: %d\n", StatSum(data_en_miss_vec));
    fprintf(stat_fp, "  Den_total_accesses: %d\n", StatSum(data_en_access_vec));
    fprintf(stat_fp, "  Den_avg. hit rate (sum of hit rates): %.4lf\n", 
            StatAvg(data_en_hit_rate_vec));
    fprintf(stat_fp, "  Den_avg. hit rate: %.4lf\n", 
            ((double) StatSum(data_en_hit_vec)) / ((double) (StatSum(data_en_hit_vec) + StatSum(data_en_miss_vec))));
    fprintf(stat_fp, "  Den_hits (per encoder): ");
    for (int i=0; i<num_data_en; i++)
        fprintf(stat_fp, "%d ", data_en_hit_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  Den_misses (per encoder): ");
    for (int i=0; i<num_data_en; i++)
        fprintf(stat_fp, "%d ", data_en_miss_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  Den_accesses (per encoder): ");
    for (int i=0; i<num_data_en; i++)
        fprintf(stat_fp, "%d ", data_en_access_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  Den_hit_rate (per encoder): ");
    for (int i=0; i<num_data_en; i++)
        fprintf(stat_fp, "%.04lf ", data_en_hit_rate_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "\n");


    fprintf(stat_fp, "Data Decoder:\n");
    fprintf(stat_fp, "  #decoders: %d\n", num_data_de);
    fprintf(stat_fp, "  Dde_total_hits: %d zero_hits: %d (%.2lf%%)\n", StatSum(data_de_hit_vec),
            StatSum(data_de_hit_zero_vec),
            ((double) StatSum(data_de_hit_zero_vec))/StatSum(data_de_hit_vec)*100.0);
    fprintf(stat_fp, "  Dde_total_misses: %d\n", StatSum(data_de_miss_vec));
    fprintf(stat_fp, "  Dde_total_accesses: %d\n", StatSum(data_de_access_vec));
    fprintf(stat_fp, "  Dde_avg. hit rate (sum of hit rates): %.4lf\n",
            StatAvg(data_de_hit_rate_vec));
    fprintf(stat_fp, "  Dde_avg. hit rate: %.4lf\n",
            ((double) StatSum(data_de_hit_vec)) / ((double) (StatSum(data_de_hit_vec) + StatSum(data_de_miss_vec))));
    fprintf(stat_fp, "  Dde_hits (per decoder): ");
    for (int i=0; i<num_data_de; i++)
        fprintf(stat_fp, "%d ", data_de_hit_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  Dde_misses (per decoder): ");
    for (int i=0; i<num_data_de; i++)
        fprintf(stat_fp, "%d ", data_de_miss_vec[i]);
    fprintf(stat_fp, "\n");
    fprintf(stat_fp, "  Dde_accesses (per decoder): ");
    for (int i=0; i<num_data_de; i++)
        fprintf(stat_fp, "%d ", data_de_access_vec[i]);
    fprintf(stat_fp, "total=%d\n", StatSum(data_de_access_vec));
    fprintf(stat_fp, "  Dde_hit_rates (per decoder): ");
    for (int i=0; i<num_data_de; i++)
        fprintf(stat_fp, "%4.2lf ", data_de_hit_rate_vec[i]);
    fprintf(stat_fp, "\n");
    if (m_cam_mt == CAM_MT_SHARED_PER_ROUTER) {
        fprintf(stat_fp, "  VLB2CAM moves (per decoder): ");
        for (int i=0; i<num_data_de; i++)
            fprintf(stat_fp, "%d ", data_de_VLB2CAM_move_vec[i]);
        fprintf(stat_fp, "total=%d\n", StatSum(data_de_VLB2CAM_move_vec));
        fprintf(stat_fp, "  Dde_replacement reqs (per decoder): ");
        for (int i=0; i<num_data_de; i++)
            fprintf(stat_fp, "%d ", data_de_replace_req_vec[i]);
        fprintf(stat_fp, "total=%d\n", StatSum(data_de_replace_req_vec));
    }
    fprintf(stat_fp, "\n");

    fprintf(stat_fp, "Data encoder final contents:\n");
    for (int i=0; i<num_data_en; i++)
        m_encoder_data_vec[i]->print(stat_fp);

    fprintf(stat_fp, "Data decoder final contents:\n");
    for (int i=0; i<num_data_de; i++)
        m_decoder_data_vec[i]->print(stat_fp);
}
