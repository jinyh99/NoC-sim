#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "noc.h"
#include "WorkloadGEMSType.h"

string MachineType_to_string(const unsigned int& obj)
{
   switch(obj) {
   case MachineType_L1Cache:
     return "L1$";
   case MachineType_L2Cache:
     return "L2$";
   case MachineType_Directory:
     return "Dir";
#ifndef MESI_SCMP_bankdirectory
   case MachineType_Collector:
     return "Col";
#endif
   default:
     assert(0);
     return "";
   }
}

#ifdef MSI_MOSI_CMP_directory
string CoherenceRequestType_to_string(const CoherenceRequestType& obj)
{
    switch(obj) {
    case CoherenceRequestType_GETX: return "GETX";
    case CoherenceRequestType_UPGRADE: return "UPGRADE";
    case CoherenceRequestType_GETS: return "GETS";
    case CoherenceRequestType_GET_INSTR: return "GET_INSTR";
    case CoherenceRequestType_PUTX: return "PUTX";
    case CoherenceRequestType_PUTS: return "PUTS";
    case CoherenceRequestType_INV: return "INV";
    case CoherenceRequestType_INV_S: return "INV_S";
    case CoherenceRequestType_L1_DG: return "L1_DG";
    case CoherenceRequestType_WB_ACK: return "WB_ACK";
    case CoherenceRequestType_EXE_ACK: return "EXE_ACK";
    default: assert(0); return "";
    }
}

string CoherenceResponseType_to_string(const CoherenceResponseType& obj)
{
    switch(obj) {
    case CoherenceResponseType_ACK: return "ACK";
    case CoherenceResponseType_INV_ACK: return "INV_ACK";
    case CoherenceResponseType_DG_ACK: return "DG_ACK";
    case CoherenceResponseType_NACK: return "NACK";
    case CoherenceResponseType_DATA: return "DATA";
    case CoherenceResponseType_DATA_S: return "DATA_S";
    case CoherenceResponseType_DATA_I: return "DATA_I";
    case CoherenceResponseType_FINALACK: return "FINALACK";
    default: assert(0); return "";
    }
}
#endif // #ifdef MSI_MOSI_CMP_directory

#ifdef MESI_SCMP_bankdirectory
string MessageSizeType_to_string(const MessageSizeType& obj)
{
  switch(obj) {
  case MessageSizeType_Undefined:
    return "Undefined";
  case MessageSizeType_Control:
    return "Control";
  case MessageSizeType_Data:
    return "Data";
  case MessageSizeType_Request_Control:
    return "Request_Control";
  case MessageSizeType_Reissue_Control:
    return "Reissue_Control";
  case MessageSizeType_Response_Data:
    return "Response_Data";
  case MessageSizeType_ResponseL2hit_Data:
    return "ResponseL2hit_Data";
  case MessageSizeType_ResponseLocal_Data:
    return "ResponseLocal_Data";
  case MessageSizeType_Response_Control:
    return "Response_Control";
  case MessageSizeType_Writeback_Data:
    return "Writeback_Data";
  case MessageSizeType_Writeback_Control:
    return "Writeback_Control";
  case MessageSizeType_Forwarded_Control:
    return "Forwarded_Control";
  case MessageSizeType_Invalidate_Control:
    return "Invalidate_Control";
  case MessageSizeType_Unblock_Control:
    return "Unblock_Control";
  case MessageSizeType_Persistent_Control:
    return "Persistent_Control";
  case MessageSizeType_Completion_Control:
    return "Completion_Control";
  default:
    assert(0);
    return "";
  }
}

int MessageSizeType_to_int(MessageSizeType size_type)
{
  switch(size_type) {
  case MessageSizeType_Undefined:
    break;
  case MessageSizeType_Control:
  case MessageSizeType_Request_Control:
  case MessageSizeType_Reissue_Control:
  case MessageSizeType_Response_Control:
  case MessageSizeType_Writeback_Control:
  case MessageSizeType_Forwarded_Control:
  case MessageSizeType_Invalidate_Control:
  case MessageSizeType_Unblock_Control:
  case MessageSizeType_Persistent_Control:
  case MessageSizeType_Completion_Control:
    return CONTROL_MESSAGE_SIZE;
    break;
  case MessageSizeType_Data:
  case MessageSizeType_Response_Data:
  case MessageSizeType_ResponseLocal_Data:
  case MessageSizeType_ResponseL2hit_Data:
  case MessageSizeType_Writeback_Data:
    return DATA_MESSAGE_SIZE;
    break;
  default:                                                                                
    break;                                                                                
  }                                                                                       
  assert(0);                                                                              
  return 0;                                                                               
} 

string CoherenceRequestType_to_string(const CoherenceRequestType& obj)
{
  switch(obj) {
  case CoherenceRequestType_GETX:
    return "GETX";
  case CoherenceRequestType_UPGRADE:
    return "UPGRADE";
  case CoherenceRequestType_GETS:
    return "GETS";
  case CoherenceRequestType_GET_INSTR:
    return "GET_INSTR";
  case CoherenceRequestType_INV:
    return "INV";
  case CoherenceRequestType_PUTX:
    return "PUTX";
  default:
    assert(0);
    return "";
  }
}

string CoherenceResponseType_to_string(const CoherenceResponseType& obj)
{
    switch(obj) {
    case CoherenceResponseType_MEMORY_ACK: return "MEMORY_ACK";
    case CoherenceResponseType_DATA: return "DATA";
    case CoherenceResponseType_DATA_EXCLUSIVE: return "DATA_EXCLUSIVE";
    case CoherenceResponseType_MEMORY_DATA: return "MEMORY_DATA";
    case CoherenceResponseType_ACK: return "ACK";
    case CoherenceResponseType_WB_ACK: return "WB_ACK";
    case CoherenceResponseType_UNBLOCK: return "UNBLOCK";
    case CoherenceResponseType_EXCLUSIVE_UNBLOCK: return "EXCLUSIVE_UNBLOCK";
    default: assert(0); return "";
    }
}
#endif // #ifdef MESI_SCMP_bankdirectory
