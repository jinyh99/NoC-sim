#ifndef _LINK_POWER_H_
#define _LINK_POWER_H_

class Flit;
class Router;

/////////////////////////////////////////////////////////
// virtual class for Link Power
/////////////////////////////////////////////////////////

class LinkPower {
public:
    LinkPower() { assert(0); };
    LinkPower(Router* router, int out_pc, int num_wires) {
        m_router = router;
        m_out_pc = out_pc;
        m_num_wires = num_wires;

        m_op_trav = 0;
    };
    virtual ~LinkPower() {};

    virtual double dynamicE() = 0;
    virtual double staticE() = 0;
    virtual void traverse(Flit* p_flit) = 0;
    int op() { return m_op_trav; };

    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

protected:
    Router* m_router;	// attached router
    int m_out_pc;	// router output PC for this link
    int m_num_wires;	// #wires for this link

    int m_op_trav;	// #operations

    vector< unsigned long long > m_link_trav_data_vec;    // X[flit_pos]
};

#endif
