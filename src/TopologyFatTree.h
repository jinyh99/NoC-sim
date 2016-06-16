#ifndef _TOPOLOGY_FAT_TREE_H_
#define _TOPOLOGY_FAT_TREE_H_

#include "Topology.h"

/**
 * <pre>
 * 16-node fat-tree: 4-way, 2-level
 *
 *         R4            R5            R6            R7         - level 1
 *        ||||          ||||          ||||          ||||
 *        ||||
 *        ||| --------------------------------------
 *        || -------------------------              |
 *        || -----------              |             |
 *        |             ||||          ||||          |||| 
 *         R0            R1            R2            R3         - level 0
 *     |  |  |  |    |  |  |  |    |  |  |   |      |   |   |    |
 *     C0 C1 C2 C3   C4 C5 C6 C7   C8 C9 C10 C11    C12 C13 C14 C15
 *
 *     crossbar size : (m+m) x (m+m) => 2m x 2m
 *
 *     for internal router: (up_level:down_level)x(up_level:down_level)
 *
 *     input PC                  output PC
 *                     -----
 *     upper level ----|   |---- upper level
 *     (0..3)      ----|   |---- (0..3)
 *                 ----|   |----
 *                 ----|   |----
 *                     |   |
 *     lower level ----|   |---- lower level
 *     (4..7)      ----|   |---- (4..7)
 *                 ----|   |----
 *                 ----|   |----
 *                     -----
 *
 *     for external router: (up_level:core)x(up_level:core)
 *
 *     input PC                  output PC
 *                     -----
 *     upper level ----|   |---- upper level
 *                 ----|   |----
 *                 ----|   |----
 *                 ----|   |----
 *                     |   |
 *            core ----|   |---- core
 *                 ----|   |----
 *                 ----|   |----
 *                 ----|   |----
 *                     -----
 *
 *     For CC traffic, we need 12x12 crossbarss for external routers.
 * ----------------------------------------------------------------------
 *   level       # external nodes       # internal nodes
 *   0           4                      1
 *   1           4^2                    1 + 4
 *   2           4^3                    1 + 4 + 4^2
 *   3           4^4                    1 + 4 + 4^2 + 4^3
 * ----------------------------------------------------------------------
 * </pre>
 */


class TopologyFatTree : public Topology {
public:
    // Constructors
    TopologyFatTree();

    // Destructor
    ~TopologyFatTree();

    // Public Methods
    void buildTopology();
    void buildTopology(int num_way);
    int getMinHopCount(int src_router_id, int dst_router_id);

    int way() const { return m_num_way; };
    int level() const { return m_max_level; };
    int num_router_per_level() const { return m_num_router_per_level; };
    unsigned int getTreeLevel(Router* p_router);
    unsigned int getIndexOnLevel(Router* p_router);
    int getCommonAncestorTreeLevel(int router_id1, int router_id2);

    void printStats(ostream& out) const;
    void print(ostream& out) const;

private:
    int m_num_way;
    int m_max_level;
    int m_num_router_per_level;		// #routers in one level
};

// Output operator declaration
ostream& operator<<(ostream& out, const TopologyFatTree& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TopologyFatTree& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif
