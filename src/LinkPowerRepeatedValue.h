#ifndef _LINK_POWER_REPEATED_VALUE_H_
#define _LINK_POWER_REPEATED_VALUE_H_

#include "LinkPowerRepeated.h"

class LinkPowerRepeatedValue : public LinkPowerRepeated {
public:
    LinkPowerRepeatedValue() {};
    LinkPowerRepeatedValue(Router* router, int out_pc, int num_wires);
    ~LinkPowerRepeatedValue();

    double report_dynamic_energy();
    double report_static_energy();
    void traverse(Flit* p_flit);

    unsigned long long intra_trans() const { return m_intra_trans; };
    unsigned long long inter_trans() const { return m_inter_trans; };

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    unsigned long long m_intra_trans;	// # of intra transitions
    unsigned long long m_inter_trans;	// # of inter transitions
};

// Output operator declaration
ostream& operator<<(ostream& out, const LinkPowerRepeatedValue& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const LinkPowerRepeatedValue& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
