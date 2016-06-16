#ifndef _CORE_H_
#define _CORE_H_

class NIInput;
class NIOutput;
class Packet;

/**
  * core-NI-router relationship
  * core to NI   : 1-to-N mapping
  * NI to router : 1-to-1 mapping
  */

class Core {
public:
    // Constructors
    Core();
    Core(int core_id, int num_NIs);

    // Destructors
    ~Core();

    int id() const { return m_id; };
    int type() const { return m_type; };
    double getEnergy() const { return m_total_energy; };
    void addEnergy(double consumed_energy) { m_total_energy += consumed_energy; };
    NIInput* getNIInput(int ni_pos) { return m_NI_in_vec[ni_pos]; };
    NIOutput* getNIOutput(int ni_pos) { return m_NI_out_vec[ni_pos]; };
    vector< NIInput* > getNIInputVec() { return m_NI_in_vec; };
    vector< NIOutput* > getNIOutputVec() { return m_NI_out_vec; };
    int num_NIInput() const { return m_NI_in_vec.size(); };
    int num_NIOutput() const { return m_NI_out_vec.size(); };

    void forwardPkt2NI(int ni_pos, Packet* p_pkt);

private:
    int m_id;
    int m_type;
    vector< NIInput* > m_NI_in_vec;		// input NI
    vector< NIOutput* > m_NI_out_vec;		// output NI

    double m_total_energy;	// total energy consumption
};

#endif
