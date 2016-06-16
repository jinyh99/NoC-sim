#include "noc.h"
#include "WorkloadSyntheticMatrix.h"
#include "Core.h"

// #define _DEBUG_WORKLOD_SYNTH_MATRIX

WorkloadSyntheticMatrix::WorkloadSyntheticMatrix() : WorkloadSynthetic()
{
    m_workload_name = "Synthetic (from input-load matrix)";

    m_trf_matrix_vec.resize(g_cfg.core_num);
    m_dest_cumm_prob.resize(g_cfg.core_num);
    for (int s=0; s<g_cfg.core_num; s++) {
        m_trf_matrix_vec[s].resize(g_cfg.core_num);
        m_dest_cumm_prob[s].resize(g_cfg.core_num);
    }
    m_src_input_load.resize(g_cfg.core_num);
    m_inter_arrv_time_vec.resize(g_cfg.core_num);

    ifstream inMatrixFile;
    inMatrixFile.open(g_cfg.wkld_synth_matrix_filename.c_str());
    assert(!inMatrixFile.fail());

    // traffic matrix format:
    // row->src, column->destination

    // fill m_trf_matrix_vec;
    for (int s=0; s<g_cfg.core_num; s++) {
        m_src_input_load[s] = 0.0;
        for (int d=0; d<g_cfg.core_num; d++) {
            inMatrixFile >> m_trf_matrix_vec[s][d];
            m_src_input_load[s] += m_trf_matrix_vec[s][d];
        }
 
        assert(m_src_input_load[s] >= 0.0 && m_src_input_load[s] <= 1.0);

        // inter-arrival time of a packet
        m_inter_arrv_time_vec[s] = 1.0/m_src_input_load[s]*g_cfg.wkld_synth_num_flits_pkt;

        // assign cummulative probability for each destination
        double cumm_prob = 0.0;
        for (int d=0; d<g_cfg.core_num; d++) {
            if (m_src_input_load[s] == 0.0) {
                m_dest_cumm_prob[s][d] = cumm_prob;
            } else {
                cumm_prob += m_trf_matrix_vec[s][d] / m_src_input_load[s];
                m_dest_cumm_prob[s][d] = cumm_prob;
            }
        }
    }

#ifdef _DEBUG_WORKLOD_SYNTH_MATRIX
    printf("input matrix:\n");
    for (int s=0; s<g_cfg.core_num; s++) {
        for (int d=0; d<g_cfg.core_num; d++) {
            printf("%lf ", m_trf_matrix_vec[s][d]);
        }
        printf("\n");
    }
    
    printf("dest cummulative probability:\n");
    for (int s=0; s<g_cfg.core_num; s++) {
        for (int d=0; d<g_cfg.core_num; d++) {
            printf("%lf ", m_dest_cumm_prob[s][d]);
        }
        printf("\n");
    }
#endif

    inMatrixFile.close();

    for (int i=0; i<g_cfg.core_num; i++)
        m_stream_temporal_vec.push_back(new stream());

    // make configuration string
    buildConfigStr();
}

WorkloadSyntheticMatrix::~WorkloadSyntheticMatrix()
{
    for (unsigned int i=0; i<m_stream_temporal_vec.size(); i++) {
        delete m_stream_temporal_vec[i];
    }
    m_stream_temporal_vec.clear();
}

int WorkloadSyntheticMatrix::spatialPattern(int src_id, int src_x, int src_y)
{
    double prob = m_stream_spatial_vec[src_id]->uniform(1.0E-99, 1.0);
    int dest = INVALID_ROUTER_ID;

    if (prob <= m_dest_cumm_prob[src_id][0]) {
        dest = 0;
        goto FOUND_DEST;
    }

    for (unsigned int d=1; d<m_dest_cumm_prob[src_id].size(); d++) {
        if (prob >= m_dest_cumm_prob[src_id][d-1] && prob < m_dest_cumm_prob[src_id][d]) {
            dest = d;
            goto FOUND_DEST;
        }
    }
    assert(0);

FOUND_DEST:
    return dest;
}

double WorkloadSyntheticMatrix::getWaitingTime(int src_core_id)
{
// printf("src_core_id=%d arrv=%lf\n", src_core_id, m_inter_arrv_time_vec[src_core_id]);
    return temporalPoisson(src_core_id);
}

////////////////////////////////////////////////////////////////
// Poisson distribution
////////////////////////////////////////////////////////////////

double WorkloadSyntheticMatrix::temporalPoisson(int src_core_id)
{
    double hold_time = m_stream_temporal_vec[src_core_id]->exponential(m_inter_arrv_time_vec[src_core_id]);

    // hold_time should be integer.
    return ((double) ((int) hold_time));
}

void WorkloadSyntheticMatrix::printStats(ostream& out) const
{
}

void WorkloadSyntheticMatrix::print(ostream& out) const
{
}
