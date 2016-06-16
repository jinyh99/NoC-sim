#ifndef _WORKLOAD_SYNTHETIC_H_
#define _WORKLOAD_SYNTHETIC_H_

#include "Workload.h"

// synthetic workload

class WorkloadSynthetic : public Workload {
public:
    // Constructors
    WorkloadSynthetic();

    // Destructor
    ~WorkloadSynthetic();

    // Public Methods
    double interArrvTime() { return m_pkt_inter_arrv_time; };
    Packet* genPacket(int src_core_id);
    virtual double getWaitingTime(int src_core_id);
    void buildConfigStr();
    virtual bool noInjection(int src_core_id) { return false; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    // spatial distribution
    int spatialPattern(int src_core_id);
    int spatialUR(int src_core_id);	// uniform
    int spatialNN(int src_core_id);	// nearest neighbor
    int spatialBC(int src_core_id);	// bit complement
    int spatialTP(int src_core_id);	// transpose
    int spatialTOR(int src_core_id);	// tornado

    vector< int > spatialPatternMulticast(int src_core_id);

    // temporal distribution: return waiting time
    double temporalSelfSimilar(int src_core_id);	// Self-similar
    virtual double temporalPoisson(int src_core_id); 	// Poisson

protected:
    double m_pkt_inter_arrv_time;	// packet inter-arrival time (cycles)
    int m_network_radix;		// #nodes/dimension

    // statistical model
    // NOTE: One stream class must be used for each injection source.
    //       Read CSIM User Guide C++ chapter 18. Random Numbers.
    vector< stream* > m_stream_temporal_vec;
    vector< stream* > m_stream_spatial_vec;
    vector< stream* > m_stream_pktlen_bimodality_vec;

    // self-similarity
    vector< bool > m_SS_OnMode_vec;		// true(ON mode), false(OFF mode)
    vector< double > m_SS_last_OnTimeStamp_vec;

    // multicast : per-core structure
    vector< stream* > m_stream_multicast_ratio_vec;	// ratio of multicast and unicast
    vector< stream* > m_stream_multicast_destnum_vec;	// # of destinations
};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadSynthetic& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadSynthetic& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _WORKLOAD_SYNTHETIC_H_
