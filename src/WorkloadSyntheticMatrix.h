#ifndef _WORKLOAD_SYNTHETIC_MATRIX_H_
#define _WORKLOAD_SYNTHETIC_MATRIX_H_

#include "WorkloadSynthetic.h"

// synthetic workload

class WorkloadSyntheticMatrix : public WorkloadSynthetic {
public:
    // Constructors
    WorkloadSyntheticMatrix();

    // Destructor
    ~WorkloadSyntheticMatrix();

    // Public Methods
    Packet* genPacket(int src_core_id);
    double getWaitingTime(int src_core_id);
    bool noInjection(int src_core_id) { return m_src_input_load[src_core_id] == 0.0 ? true : false; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    // spatial distribution
    int spatialPattern(int src_id, int src_x, int src_y);

    // temporal distribution
    //   return waiting time
    double temporalPoisson(int src_core_id); 	// Poisson

    // traffic matrix for input load
    vector< vector< double > > m_trf_matrix_vec;	// X[src_core][dest_core]
    vector< double > m_src_input_load;		// X[src_core]
    vector< double > m_inter_arrv_time_vec;	// X[src_core]
    // cummulative probability for each destination
    vector< vector< double > > m_dest_cumm_prob;	// X[src_core][dest_core]
};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadSyntheticMatrix& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadSyntheticMatrix& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _WORKLOAD_SYNTHETIC_H_
