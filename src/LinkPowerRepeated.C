#include "noc.h"
#include "LinkPowerRepeated.h"
#include "Router.h"

LinkPowerRepeated::LinkPowerRepeated(Router* router, int out_pc, int num_wires)
  : LinkPower(router, out_pc, num_wires)
{
    initPowerModel();
}

LinkPowerRepeated::~LinkPowerRepeated()
{
}

double LinkPowerRepeated::dynamicE()
{
    // sum of power :)
    double total_dynamic_power = 
           (m_intra_dynamic_power + 2.0*m_inter_dynamic_power) *
           ((double) m_num_wires) * ((double) m_op_trav) * g_cfg.power_link_avf;

    // E = P/f
    double total_dynamic_energy = total_dynamic_power / g_cfg.chip_freq;
// printf("cycle=%.0lf router=%d out_pc=%d dynamic_energy(J)=%lg dynamic_power(W)=%lf m_op_trav=%d #wires=%d\n", simtime(), m_router->id(), m_out_pc, total_dynamic_energy, total_dynamic_power, m_op_trav, m_num_wires);

    return total_dynamic_energy;
}

double LinkPowerRepeated::staticE()
{
    return (m_leakage_power / g_cfg.chip_freq) * m_num_wires;
}

void LinkPowerRepeated::initPowerModel()
{
    double wire_length = m_router->getLink(m_out_pc).m_length_mm;

    switch (g_cfg.chip_tech_nano) {
    case 65:
        switch (g_cfg.link_wire_type) {
        case LINK_WIRE_TYPE_GLOBAL:
            m_intra_dynamic_power = 1.58905 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.787507 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00144102 * (UNIT_MILLI * wire_length);
            break;
        case LINK_WIRE_TYPE_INTER:
            m_intra_dynamic_power = 1.38403 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.565399 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00118203 * (UNIT_MILLI * wire_length);
            break;
        default:
            assert(0);
            break;
        }
        break;

    case 45:
        switch (g_cfg.link_wire_type) {
        case LINK_WIRE_TYPE_GLOBAL:
            m_intra_dynamic_power = 1.13527 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.634376 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00162453 * (UNIT_MILLI * wire_length);
            break;
        case LINK_WIRE_TYPE_INTER:
            m_intra_dynamic_power = 0.971822 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.432656 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00128931 * (UNIT_MILLI * wire_length);
            break;
        default:
            assert(0);
            break;
        }
        break;

    case 32:
        switch (g_cfg.link_wire_type) {
        case LINK_WIRE_TYPE_GLOBAL:
            m_intra_dynamic_power = 0.87742 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.456354 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00177682 * (UNIT_MILLI * wire_length);
            break;
        case LINK_WIRE_TYPE_INTER:
            m_intra_dynamic_power = 0.734457 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.314397 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00139725 * (UNIT_MILLI * wire_length);
          break;
        default:
            assert(0);
            break;
        }
        break;

    case 22:
        switch (g_cfg.link_wire_type) {
        case LINK_WIRE_TYPE_GLOBAL:
            m_intra_dynamic_power = 0.693631 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.359685 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00233787 * (UNIT_MILLI * wire_length);
            break;
        case LINK_WIRE_TYPE_INTER:
            m_intra_dynamic_power = 0.603373 * (UNIT_MILLI * wire_length);
            m_inter_dynamic_power = 0.262364 * (UNIT_MILLI * wire_length);
            m_leakage_power = 0.00192153 * (UNIT_MILLI * wire_length);
            break;
        default:
            assert(0);
            break;
        }
        break;

    default:
        assert(0);
        break;
    }
}

void LinkPowerRepeated::printStats(ostream& out) const
{
}

void LinkPowerRepeated::print(ostream& out) const
{
}

