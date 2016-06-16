#include "RoutingFlbfly.h"
#include "TopologyFlbfly.h"

RoutingFlbfly::RoutingFlbfly()
{
}

RoutingFlbfly::~RoutingFlbfly()
{
}

void RoutingFlbfly::init()
{
    if (g_cfg.net_routing == ROUTING_FLBFLY_XY)
        m_routing_name = "FLBFLY_XY";
    else if (g_cfg.net_routing == ROUTING_FLBFLY_YX)
        m_routing_name = "FLBFLY_YX";

    m_topology_flbfly = (TopologyFlbfly *) m_topology;
}

int RoutingFlbfly::selectOutPC(Router* cur_router, int cur_vc, FlitHead* p_flit)
{
    int cur_out_pc = INVALID_PC;

    int dest_router_id = p_flit->dest_router_id();
    int cur_router_id = cur_router->id();
    int dest_x_coord, dest_y_coord;
    int cur_x_coord, cur_y_coord;
    int x_offset, y_offset;

    dest_x_coord = dest_router_id % m_topology_flbfly->cols();
    dest_y_coord = dest_router_id / m_topology_flbfly->cols();

    cur_x_coord = cur_router_id % m_topology_flbfly->cols();
    cur_y_coord = cur_router_id / m_topology_flbfly->cols();
  
    x_offset = dest_x_coord - cur_x_coord;
    y_offset = dest_y_coord - cur_y_coord;

    switch (g_cfg.net_routing) {
    case ROUTING_FLBFLY_XY:
       if(x_offset > 0){ // go to EAST
         if(x_offset >= 3 && isPCAvailable(cur_router, DIR_EAST3)){
           cur_out_pc = DIR_EAST3;
         } else if(x_offset >=2 && isPCAvailable(cur_router, DIR_EAST2)){
           cur_out_pc = DIR_EAST2;
         } else{
           cur_out_pc = DIR_EAST;
         }
       } else if(x_offset < 0){ // go to WEST
         if(x_offset <= -3 && isPCAvailable(cur_router, DIR_WEST3)){ 
           cur_out_pc = DIR_WEST3;
         } else if(x_offset <= -2 && isPCAvailable(cur_router, DIR_WEST2)){
           cur_out_pc = DIR_WEST2;
         } else{
           cur_out_pc = DIR_WEST;
         }
       } else{
          if(y_offset > 0){ // go to SOUTH
              if(y_offset >= 3 && isPCAvailable(cur_router, DIR_SOUTH3)){ 
                  cur_out_pc = DIR_SOUTH3;
               } else if(y_offset >=2 && isPCAvailable(cur_router, DIR_SOUTH2)){
                  cur_out_pc = DIR_SOUTH2;
               } else{
                  cur_out_pc = DIR_SOUTH;
               }
          } else if(y_offset < 0){ // go to NORTH
              if(y_offset <= -3 && isPCAvailable(cur_router, DIR_NORTH3)){
                  cur_out_pc = DIR_NORTH3;
              } else if(y_offset <= -2 && isPCAvailable(cur_router, DIR_NORTH2)){
                  cur_out_pc = DIR_NORTH2;
              } else{ 
                  cur_out_pc = DIR_NORTH;
              }  
          } else{ // arrive at destination
              cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
          }   
        }
        break;
    
    case ROUTING_FLBFLY_YX:
        if(y_offset > 0){ // go to SOUTH
          if(y_offset >= 3 && isPCAvailable(cur_router, DIR_SOUTH3)){
              cur_out_pc = DIR_SOUTH3;
          } else if(y_offset >=2 && isPCAvailable(cur_router, DIR_SOUTH2)){
              cur_out_pc = DIR_SOUTH2;
          } else{
              cur_out_pc = DIR_SOUTH;
          }
        } else if(y_offset < 0){ // go to NORTH
          if(y_offset <= -3 && isPCAvailable(cur_router, DIR_NORTH3)){
              cur_out_pc = DIR_NORTH3;
          } else if(y_offset <= -2 && isPCAvailable(cur_router, DIR_NORTH2)){
              cur_out_pc = DIR_NORTH2;
          } else{
              cur_out_pc = DIR_NORTH;
          }
        } else{ 
          if(x_offset > 0){ // go to EAST
              if(x_offset >= 3 && isPCAvailable(cur_router, DIR_EAST3)){
                  cur_out_pc = DIR_EAST3;
              } else if(x_offset >=2 && isPCAvailable(cur_router, DIR_EAST2)){
                  cur_out_pc = DIR_EAST2;
              } else{
                  cur_out_pc = DIR_EAST;
              }
          } else if(x_offset < 0){ // go to WEST
              if(x_offset <= -3 && isPCAvailable(cur_router, DIR_WEST3)){
                  cur_out_pc = DIR_WEST3;
              } else if(x_offset <= -2 && isPCAvailable(cur_router, DIR_WEST2)){
                  cur_out_pc = DIR_WEST2;
              } else{
                  cur_out_pc = DIR_WEST;
              }  
          } else{ // arrive at destination
              cur_out_pc = cur_router->num_internal_pc() + p_flit->getPkt()->m_NI_out_pos;
          } 
        }
        break; 
    default:
        assert(0);
    }

    return cur_out_pc;
}

bool RoutingFlbfly::isPCAvailable(Router* cur_router, int cur_out_pc)
{
    int num_rsv_out_vc = 0;
    for (int out_vc=0; out_vc<cur_router->num_vc(); out_vc++) {
        if (! cur_router->outputModule(cur_out_pc, out_vc).m_state != OUT_MOD_I) {
            num_rsv_out_vc++;
        }
    }

    return (num_rsv_out_vc == cur_router->num_vc()) ? false : true;
}

void RoutingFlbfly::printStats(ostream& out) const
{
}

void RoutingFlbfly::print(ostream& out) const
{
}
