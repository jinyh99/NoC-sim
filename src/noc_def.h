#ifndef _NOC_DEF_H_
#define _NOC_DEF_H_

////////////////////////////////////////////////////////////////////
// macros
#define _MAX(a, b)    (((a) < (b)) ? (b) : (a))
#define _MIN(a, b)    (((a) < (b)) ? (a) : (b))

#define STRLEN2BYTESZ(n)	((n)/2)	// for packet payload processing
#define BYTESZ2STRLEN(n)	((n)*2)	// for packet payload processing
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// constants
#define INVALID_ROUTER_ID       (-1)
#define INVALID_CORE_ID         (-1)
#define INVALID_PC              (-1)
#define INVALID_VC              (-2)
#define INVALID_CLK             (-1.0)
#define INFINITE_CLK            (1.0e99)
#define INVALID_CREDIT          (-1)

#define UNIT_KILO               (1.0e3)
#define UNIT_MEGA               (1.0e6)
#define UNIT_GIGA               (1.0e9)
#define UNIT_TERA               (1.0e12)
#define UNIT_MILLI              (1.0e-3)
#define UNIT_MICRO              (1.0e-6)
#define UNIT_NANO               (1.0e-9)
#define UNIT_PICO               (1.0e-12)
#define UNIT_FEMTO              (1.0e-15)

#define BITS_IN_BYTE            8
#define ONE_CYCLE               (1.0)
//////////////////////////////////////////////////////////////////// 

////////////////////////////////////////////////////////////////////
// network topology
enum NETWORK_TOPOLOGY_TYPE {
    TOPOLOGY_UNDEFINED=0,
    TOPOLOGY_MESH,
    TOPOLOGY_HMESH,
    TOPOLOGY_DMESH,
    TOPOLOGY_TORUS,
    TOPOLOGY_FAT_TREE,
    TOPOLOGY_MECS,
    TOPOLOGY_TTREE,
    TOPOLOGY_FLBFLY,
    TOPOLOGY_SNUCA,
    TOPOLOGY_TILED_CMP_MESH,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// switch allocation
enum SW_ALLOC_TYPE {
    SW_ALLOC_UNDEFINED=0,
    SW_ALLOC_MAXIMAL,	// maximal matching
    SW_ALLOC_RR,	// round-robin
    SW_ALLOC_LRS,	// least recently selected
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// VC allocation
enum VC_ALLOC_TYPE {
    VC_ALLOC_UNDEFINED=0,
    VC_ALLOC_RR,	// round-robin
    VC_ALLOC_FCFS,	// FCFS
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// buffer type
enum ROUTER_BUFFER_TYPE {
    ROUTER_BUFFER_UNDEFINED=0,
    ROUTER_BUFFER_SAMQ,
    ROUTER_BUFFER_DAMQ_P,	// sharing VCs in the same PC
    ROUTER_BUFFER_DAMQ_R,	// sharing all VCs in the router
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// routing
enum ROUTING_TYPE {
    ROUTING_UNDEFINED=0,
    ROUTING_XY,
    ROUTING_YX,
    ROUTING_MIN_OBLIVIOUS,      // minimal oblivious routing
    ROUTING_MIN_ADAPTIVE_VC,    // minimal adaptive routing using free VCs
    ROUTING_MIN_SCHEDULED,      // minimal scheduled routing
    ROUTING_TORUS_DALLY,
    ROUTING_HMESH_ASCENT,       // high-level first
    ROUTING_HMESH_DESCENT,      // low-level first
    ROUTING_DMESH_XY,           // 2D double mesh xy routing
    ROUTING_DMESH_YX,           // 2D double mesh yx routing
    ROUTING_FAT_TREE_ADAPTIVE,  // fat tree (downward adaptively)
    ROUTING_FAT_TREE_RANDOM,    // fat tree (downward randomly)
    ROUTING_TTREE,              // tapered fat tree
    ROUTING_FLBFLY_XY, 		// flattened butterfly xy routing
    ROUTING_FLBFLY_YX, 		// flattened butterfly yx routing
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// direction
enum {
    DIR_INVALID = -1,
    DIR_WEST,   // left
    DIR_EAST,   // right
    DIR_NORTH,  // up
    DIR_SOUTH,  // down

    DIR_WEST2,   // 2 hops left
    DIR_EAST2,   // 2 hops right
    DIR_NORTH2,  // 2 hops up
    DIR_SOUTH2,  // 2 hops down

    DIR_WEST3,   // 3 hops left
    DIR_EAST3,   // 3 hops right
    DIR_NORTH3,  // 3 hops up
    DIR_SOUTH3,  // 3 hops down
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// router power model
enum ROUTER_POWER_MODEL_TYPE {
    ROUTER_POWER_MODEL_UNDEFINED=0,
    ROUTER_POWER_MODEL_ORION_CALL,
    ROUTER_POWER_MODEL_STATS,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// link power model
enum LINK_POWER_MODEL_TYPE {
    LINK_POWER_MODEL_UNDEFINED=0,
    LINK_POWER_MODEL_ORION,
    LINK_POWER_MODEL_DELAY_OPT_REPEATED,
    LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// link wire type
enum LINK_WIRE_TYPE {
    LINK_WIRE_TYPE_UNDEFINED=0,
    LINK_WIRE_TYPE_GLOBAL,
    LINK_WIRE_TYPE_INTER,	// intermediate
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// core
enum CORE_TYPE {
    CORE_TYPE_UNDEFINED=0,
    CORE_TYPE_GENERIC,
    CORE_TYPE_L1,
    CORE_TYPE_L2,
    CORE_TYPE_MEM,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// simulation
#define SIM_END_BY_INJ_PKT		1
#define SIM_END_BY_EJT_PKT		2
#define SIM_END_BY_CYCLE		3
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// link DVS
#ifdef LINK_DVS
enum LINK_DVS_TYPE {
    LINK_DVS_NODVS=0,
    LINK_DVS_HISTORY,
    LINK_DVS_FLIT_RATE_PREDICT,
};
#endif
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// workload types
enum WORKLOAD_TYPE {
    WORKLOAD_UNDEFINED=0,
    WORKLOAD_SYNTH_SPATIAL,
    WORKLOAD_SYNTH_TRAFFIC_MATRIX,
    WORKLOAD_TRIPS_TRACE,
    WORKLOAD_TILED_CMP_TRACE,
    WORKLOAD_TILED_CMP_VALUE_TRACE,
    WORKLOAD_SNUCA_CMP_VALUE_TRACE,
};

// synthetic spatial patterns
enum WORKLOAD_SYNTH_SPATIAL_PATTERN {
    WORKLOAD_SYNTH_SP_UNDEFINED=0,
    WORKLOAD_SYNTH_SP_UR,
    WORKLOAD_SYNTH_SP_NN,
    WORKLOAD_SYNTH_SP_BC,
    WORKLOAD_SYNTH_SP_TP,
    WORKLOAD_SYNTH_SP_TOR,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// NI
// Input NI
enum NI_INPUT_TYPE {
    NI_INPUT_TYPE_UNDEFINED=0,
    NI_INPUT_TYPE_PER_PC,
    NI_INPUT_TYPE_PER_VC,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// compression
enum CAM_MANAGE_TYPE {
    CAM_MT_UNDEFINED=0,
    CAM_MT_PRIVATE_PER_ROUTER,		 // private for each destination
    CAM_MT_PRIVATE_PER_CORE,
    CAM_MT_SHARED_PER_ROUTER,            // shared
};

enum CAMReplacePolicy {
    CAM_REPL_UNDEFINED=0,
    CAM_REPL_LRU,
    CAM_REPL_LFU,
};
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// tunneling
enum TUNNELING_TYPE {
    TUNNELING_PER_FLOW=0,
    TUNNELING_PER_DEST,
    TUNNELING_PER_OUTPORT,
};
////////////////////////////////////////////////////////////////////

#endif // #ifndef _NOC_DEF_H_
