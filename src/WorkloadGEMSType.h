#ifndef _WORKLOAD_GEMS_TYPES_H_
#define _WORKLOAD_GEMS_TYPES_H_

// Cache coherence workload from GEMS

///////////////////////////////////////////////////////////////
// #define MSI_MOSI_CMP_directory
#define MESI_SCMP_bankdirectory
// #define MOESI_CMP_token

// #define TRACE_PACKET_VALUE
///////////////////////////////////////////////////////////////

#ifdef TRACE_PACKET_VALUE
const int CONTROL_MESSAGE_SIZE = 8;             // byte
const int DATA_MESSAGE_SIZE = (64+8);           // byte
#else
const int CONTROL_MESSAGE_SIZE = 16;            // byte
const int DATA_MESSAGE_SIZE = (64+16);          // byte
#endif

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long uint64;

///////////////////////////////////////////////////////////////
#ifdef MESI_SCMP_bankdirectory
enum MachineType {
  MachineType_FIRST,
  MachineType_L2Cache = MachineType_FIRST,  /**< No description avaliable */
  MachineType_L1Cache,  /**< No description avaliable */
  MachineType_Directory,  /**< No description avaliable */
  MachineType_NUM
};

enum MessageSizeType {
  MessageSizeType_FIRST,
  MessageSizeType_Undefined = MessageSizeType_FIRST,  /**< Undefined */
  MessageSizeType_Control,  /**< Control Message */
  MessageSizeType_Data,  /**< Data Message */
  MessageSizeType_Request_Control,  /**< Request */
  MessageSizeType_Reissue_Control,  /**< Reissued request */
  MessageSizeType_Response_Data,  /**< data response */
  MessageSizeType_ResponseL2hit_Data,  /**< data response */
  MessageSizeType_ResponseLocal_Data,  /**< data response */
  MessageSizeType_Response_Control,  /**< non-data response */
  MessageSizeType_Writeback_Data,  /**< Writeback data */
  MessageSizeType_Writeback_Control,  /**< Writeback control */
  MessageSizeType_Forwarded_Control,  /**< Forwarded control */
  MessageSizeType_Invalidate_Control,  /**< Invalidate control */
  MessageSizeType_Unblock_Control,  /**< Unblock control */
  MessageSizeType_Persistent_Control,  /**< Persistent request activation messages */
  MessageSizeType_Completion_Control,  /**< Completion messages */
  MessageSizeType_NUM
};

enum CoherenceRequestType {
  CoherenceRequestType_FIRST,
  CoherenceRequestType_GETX = CoherenceRequestType_FIRST,  // Get eXclusive
  CoherenceRequestType_UPGRADE, // UPGRADE to exclusive
  CoherenceRequestType_GETS,  // Get Shared
  CoherenceRequestType_GET_INSTR,  // Get Instruction
  CoherenceRequestType_INV,  // INValidate
  CoherenceRequestType_PUTX,  // replacement message
  CoherenceRequestType_NUM
};

enum CoherenceResponseType {
  CoherenceResponseType_FIRST,
  CoherenceResponseType_MEMORY_ACK = CoherenceResponseType_FIRST,  // Ack from memory controller
  CoherenceResponseType_DATA,  // Data
  CoherenceResponseType_DATA_EXCLUSIVE,  // Data
  CoherenceResponseType_MEMORY_DATA,  // Data
  CoherenceResponseType_ACK,  // Generic invalidate ack
  CoherenceResponseType_WB_ACK,  // writeback ack
  CoherenceResponseType_UNBLOCK,  // unblock
  CoherenceResponseType_EXCLUSIVE_UNBLOCK,  // exclusive unblock
  CoherenceResponseType_NUM
};
#endif // #ifdef MESI_SCMP_bankdirectory
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
#ifdef MSI_MOSI_CMP_directory
enum MachineType {
  MachineType_FIRST,
  MachineType_L1Cache = MachineType_FIRST,
  MachineType_L2Cache,
  MachineType_Directory,
  MachineType_Collector,
  MachineType_NUM
};

enum CoherenceRequestType {
  CoherenceRequestType_FIRST,
  CoherenceRequestType_GETX = CoherenceRequestType_FIRST,
  CoherenceRequestType_UPGRADE,
  CoherenceRequestType_GETS,
  CoherenceRequestType_GET_INSTR,
  CoherenceRequestType_PUTX,
  CoherenceRequestType_PUTS,
  CoherenceRequestType_INV,
  CoherenceRequestType_INV_S,
  CoherenceRequestType_L1_DG,
  CoherenceRequestType_WB_ACK,
  CoherenceRequestType_EXE_ACK,
  CoherenceRequestType_NUM
};

enum CoherenceResponseType {
  CoherenceResponseType_FIRST,
  CoherenceResponseType_ACK = CoherenceResponseType_FIRST,
  CoherenceResponseType_INV_ACK,
  CoherenceResponseType_DG_ACK,
  CoherenceResponseType_NACK,
  CoherenceResponseType_DATA,
  CoherenceResponseType_DATA_S,
  CoherenceResponseType_DATA_I,
  CoherenceResponseType_FINALACK,
  CoherenceResponseType_NUM
};
#endif
///////////////////////////////////////////////////////////////

string MachineType_to_string(const unsigned int& obj);
string MessageSizeType_to_string(const MessageSizeType& obj);
int MessageSizeType_to_int(MessageSizeType size_type);
string CoherenceRequestType_to_string(const CoherenceRequestType& obj);
string CoherenceResponseType_to_string(const CoherenceResponseType& obj);

const unsigned int MAX_PKT_TRACE_DATA_VALUE_SZ = 64;

// -- 03/22/08 old structure (for compression work)
typedef struct _pkt_trace_value {
  unsigned long long cycle;
// 8B

  unsigned int src_mach_type : 4;       // source type
  unsigned int src_mach_num : 28;       // source id

  unsigned int dest_mach_type : 4;      // dest type
  unsigned int dest_mach_num : 28;      // dest id
// 16B

  unsigned int sz_bytes;       // packet size in bytes
  unsigned int unused;
// 32B

  uint64 addr_value;
// 40B
  uint8 data_value[MAX_PKT_TRACE_DATA_VALUE_SZ];
// 104B
} PktTraceValue;

// 08/01/08
typedef struct _pkt_trace {
  unsigned long long cycle;            // cycle#
// 8B
  unsigned long long instr_executed;   // instr
// 16B

  unsigned int src_mach_type : 4;      // source type
  unsigned int src_mach_num : 12;      // source id
  unsigned int dest_mach_type : 4;     // dest type
  unsigned int dest_mach_num : 12;     // dest id

  unsigned int msg_sz_type : 12;       // msg_sz_type
  unsigned int msg_format : 8;         // cache/request/response
  unsigned int coh_type : 12;          // CacheRequestType/CoherenceRequestType/CoherenceResponseType
// 24B

  unsigned long long multicast;        // multicast status
// 32B

  uint64 addr_value;
// 40B
#ifdef TRACE_PACKET_VALUE
  uint8 data_value[MAX_PKT_TRACE_DAVA_VALUE_SZ];
#endif // #ifdef TRACE_PACKET_VALUE
} PktTrace;

#endif // #ifndef _WORKLOAD_GEMS_TYPES_H_
