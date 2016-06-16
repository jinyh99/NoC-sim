#ifndef _LINK_POWER_REPEATED_H_
#define _LINK_POWER_REPEATED_H_

#include "LinkPower.h"

class LinkPowerRepeated : public LinkPower {
public:

    LinkPowerRepeated() {};
    LinkPowerRepeated(Router* router, int out_pc, int num_wires);
    ~LinkPowerRepeated();

    void traverse(Flit* p_flit) { m_op_trav++; };
    double dynamicE();
    double staticE();

    void printStats(ostream& out) const;
    void print(ostream& out) const;

protected:
    void initPowerModel();

protected:
    double m_intra_dynamic_power;	// intra-wire dynamic power (W/mm)
    double m_inter_dynamic_power;	// inter-wire dynamic power (W/mm)
    double m_leakage_power;		// leakage power
};

// Output operator declaration
ostream& operator<<(ostream& out, const LinkPowerRepeated& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const LinkPowerRepeated& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
