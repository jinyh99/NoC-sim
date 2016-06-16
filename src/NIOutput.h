#ifndef _NI_OUTPUT_H_
#define _NI_OUTPUT_H_

#include "NI.h"

class Packet;
class Flit;

class NIOutput : public NI {
public:
    // Constructors
    NIOutput();
    NIOutput(Core* p_Core, int NI_id, int NI_pos);

    // Destructors
    ~NIOutput();

    void writeFlit(Flit* p_flit);
    Flit* readFlit();
    int getFlitBufferLen() { return m_NIout_flitQ.size(); };
    virtual void assembleFlit();

    void init() {};
    void resetStats() {};

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    deque< Flit* > m_NIout_flitQ; // flit ejection buffer
    vector< bool > m_vc_free_sts_vec;
    event* m_ev_wakeup;
};

// Output operator declaration
ostream& operator<<(ostream& out, const NIOutput& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const NIOutput& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
