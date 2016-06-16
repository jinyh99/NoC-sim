#ifndef _NI_H_
#define _NI_H_

// NOTE: 1:1 mapping between core and NI
//       1:1 mapping between NI and router port 

class Core;

class NI {
public:
    // Constructors
    NI() {};

    // Destructors
    virtual ~NI() {};

    int id() const { return m_NI_id; };
    int pos() const { return m_NI_pos; };
    Core* getCore() const { return m_attached_core; };
    Router* getRouter() const { return m_attached_router; };
    void attachRouter(Router* p_Router, int router_pc) {
        m_attached_router = p_Router;
        m_router_pc = router_pc;
        init();
    };

    virtual void init() = 0;
    virtual void resetStats() = 0;
    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

protected:
    int m_NI_id;	// NI id
    int m_NI_pos;	// NI position from the view of the core
    Core* m_attached_core;
    Router* m_attached_router;
    int m_router_pc;	// router's physical channel for NI
    int m_num_vc;
};

// Output operator declaration
ostream& operator<<(ostream& out, const NI& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const NI& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
