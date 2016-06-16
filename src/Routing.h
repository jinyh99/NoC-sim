#ifndef _GENERIC_ROUTING_H_
#define _GENERIC_ROUTING_H_

#include "Router.h"

class Topology;

// abstract class
class Routing {
public:
    // Constructors
    Routing() {};

    // Destructor
    virtual ~Routing() {};

    // Public Methods
    string getName() const { return m_routing_name; };
    void setTopology(Topology* topology) {
        m_topology = topology;
        init();
    };

    virtual void init() = 0;
    virtual int selectOutPC(Router* p_cur_router, int cur_in_vc, FlitHead* p_flit) = 0;
    virtual bool isOutVCDeadlockFree(int out_vc, int in_pc, int in_vc, int out_pc, Router* cur_router, FlitHead* p_flit) {
        return true;
    };
    virtual void writeRouteInfoToHeadFlit(FlitHead* p_flit) { assert(0); };
    virtual vector< Router* > getPathVector(Router* src_router, Router* dest_router) {
        vector< Router* > path_vec; assert(0); return path_vec;
    };

    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

protected:
    string m_routing_name;
    Topology* m_topology;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Routing& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Routing& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
