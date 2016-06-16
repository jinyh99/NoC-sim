#ifndef _TOPOLOGY_H_
#define _TOPOLOGY_H_

// abstract class
class Topology {
public:
    // Constructors
    Topology() {};

    // Destructor
    virtual ~Topology() {};

    // Public Methods
    string getName() const { return m_topology_name; };
    virtual void buildTopology() = 0;
    /// return minimal hop count
    virtual int getMinHopCount(int src_router_id, int dst_router_id) = 0;

    virtual void printStats(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;

    pair< int, int > core2router(int router_id) { return m_core2router_map[router_id]; }
    int router2core(int router_id, int port_pos) { return m_router2core_map[make_pair(router_id, port_pos)]; }

protected:
    void createCores();

protected:
    string m_topology_name;

    // for concentrated topology
    // core to router map: core_id -> (router_id, port_pos (relative to core channels))
    map< int, pair< int, int > > m_core2router_map;
    // router to core map: (router_id, port_pos (relative to core channels)) -> core_id
    map< pair< int, int >, int > m_router2core_map;

    // for multiple networks
    // core to (router, net) map: (core_id, net_id) -> (router_id, port_pos (relative to core channels))
    map< pair< int, int >, pair< int, int > > m_CN2router_map;
    // (router, net) to core map: router_id -> (core_id, net_id)
    map< int, pair< int, int > > m_router2CN_map;
};


// Output operator declaration
ostream& operator<<(ostream& out, const Topology& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Topology& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif // #ifndef _TOPOLOGY_H_
