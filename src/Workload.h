#ifndef _GENERIC_WORKLOAD_H_
#define _GENERIC_WORKLOAD_H_

// abstract class
class Workload {
public:
    // Constructors
    Workload() {
        m_workload_name = "Undefined";
        m_workload_config = "Not configured";
        m_synthetic = true;
        m_containData = false;

        // packet spatial distribution
        m_spat_pattern_pkt_vec.resize(g_cfg.core_num);
        m_spat_pattern_flit_vec.resize(g_cfg.core_num);
        for (int s=0; s<g_cfg.core_num; s++) {
            m_spat_pattern_pkt_vec[s].resize(g_cfg.core_num, 0);
            m_spat_pattern_flit_vec[s].resize(g_cfg.core_num, 0);
        }
    };

    // Destructor
    virtual ~Workload() {};

    // Public Methods
    string getName() const { return m_workload_name; };
    string getConfig() const { return m_workload_config; };
    bool isSynthetic() const { return m_synthetic; };
    bool containData() const { return m_containData; };

    unsigned long long getSpatialPattPkt(unsigned int src_core_id, unsigned int dest_core_id) const
        { return m_spat_pattern_pkt_vec[src_core_id][dest_core_id]; };
    unsigned long long getSpatialPattFlit(unsigned int src_core_id, unsigned int dest_core_id) const
        { return m_spat_pattern_flit_vec[src_core_id][dest_core_id]; };
    void resetStats() {
        for (int s=0; s<g_cfg.core_num; s++)
        for (int d=0; d<g_cfg.core_num; d++) {
            m_spat_pattern_pkt_vec[s][d] = 0;
            m_spat_pattern_flit_vec[s][d] = 0;
        }
    }

    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

protected:
    string m_workload_name;
    string m_workload_config;
    bool m_synthetic;
    bool m_containData;

    // #packets for a src-dest pair
    vector< vector< unsigned long long > > m_spat_pattern_pkt_vec; // X[src][dest]
    // #flits for a src-dest pair
    vector< vector< unsigned long long > > m_spat_pattern_flit_vec; // X[src][dest]
};

// Output operator declaration
ostream& operator<<(ostream& out, const Workload& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Workload& obj)
{
    obj.print(out);
    out << flush;
    return out;
}




class Packet;

class WorkloadTrace : public Workload {
public:
    // Constructors
    WorkloadTrace() {
        m_synthetic = false;
        m_trace_fp = 0;
        m_trace_file_id = 0;

        m_trace_dir_name = g_cfg.wkld_trace_dir_name;
        m_benchmark_name = g_cfg.wkld_trace_benchmark_name;
        m_trace_skip_cycles = g_cfg.wkld_trace_skip_cycles;

        m_num_proc_traces = 0;
        m_num_unproc_traces = 0;
    };

    // Destructor
    virtual ~WorkloadTrace() { };

    // Public Methods
    unsigned long long getProcessedTraceCount() const { return m_num_proc_traces; };
    int trace_file_id() { return m_trace_file_id; };

    virtual bool openTraceFile() = 0;
    virtual void skipTraceFile() = 0;
    virtual void closeTraceFile() = 0;
    virtual vector< Packet* > readTrace() = 0;

    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

protected:
    string m_benchmark_name; 
    string m_trace_dir_name;
    FILE* m_trace_fp;
    int m_trace_file_id;	// trace file id to open
    double m_trace_skip_cycles;

    unsigned long long m_num_proc_traces; 	// #traces processed
    unsigned long long m_num_unproc_traces; 	// #traces not processed due to format error, ...
};

// Output operator declaration
ostream& operator<<(ostream& out, const WorkloadTrace& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const WorkloadTrace& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif // #ifndef _GENERIC_WORKLOAD_H_
