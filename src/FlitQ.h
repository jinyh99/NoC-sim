#ifndef _FLIT_Q_H_
#define _FLIT_Q_H_

class Flit;

class FlitQ {
public:
    /// Constructors
    FlitQ();
    FlitQ(int num_pc, int num_vc, int depth);

    /// Destructors
    ~FlitQ();

    /// read a flit from buffer
    Flit* read(int pc, int vc);
    /// write a flit to buffer
    void write(int pc, int vc, Flit* p_flit);
    /// return the currently used slots in buffer
    int size(int pc, int vc);
    /// return the buffer depth
    int depth();
    /// true if buffer is empty
    bool isEmpty(int pc, int vc);
    /// true if buffer is full
    bool isFull(int pc, int vc);
    /// return a head flit pointer in the buffer
    Flit* peek(int pc, int vc);
    /// print the content of buffer
    void print(FILE* fp, int pc, int vc);
    void printAll(FILE* fp);

    void resetStats();
    double getAvgOccupancy(int pc, int vc, double sim_cycles);
    int getMaxOccupancy(int pc, int vc) { return m_max_occupancy_vec[pc][vc]; };

protected:
   int m_num_pc;
   int m_num_vc;
   int m_depth;	// for each VC

   /// per-VC queue: m_Q_vec[pc][vc]
   vector< vector< deque< Flit* > > > m_Q_vec;

   vector< vector< int > > m_max_occupancy_vec;		// m_max_occupancy[pc][vc]
   vector< vector< double > > m_sum_occupancy_vec;	// m_sum_occupancy_vec[pc][vc]
   vector< vector< double > > m_last_update_clk_vec;	// clk for last update
};

#endif // #ifndef _FLIT_Q_H_
