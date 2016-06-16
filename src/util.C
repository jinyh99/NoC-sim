#include "noc.h"

int factorial(int n)
{
    int product = 1;
    for (int i=2; i<=n; i++)
        product = product * i;
    return product;
}

int combination(int a, int b)
{
    return factorial(a) * factorial(b) / factorial(a-b);
}

vector< string > split_str(string str, char separator)
{
    vector< string > substr_vec;

    string::size_type start_loc = 0;
    string::size_type find_loc = 0;
    while ((find_loc = str.find(separator, start_loc)) != string::npos) {
        string sub_str = str.substr(start_loc, find_loc-start_loc);
        if (sub_str.size() != 0)
            substr_vec.push_back(sub_str);
        start_loc = find_loc + 1;
    }
    string sub_str = str.substr(start_loc, find_loc-start_loc);
    if (sub_str.size() != 0)
        substr_vec.push_back(sub_str);

    return substr_vec;
}

unsigned long long random_64bit()
{
    unsigned long long high, low;

    high = random(0x0, 0x7FFFFFFF);
    low = random(0x0, 0x7FFFFFFF);
    high = (high << 32);
    low = (low << 32) >> 32;

    return (low | high);
}

unsigned long long pattern_64bit()
{
    unsigned long long high, low;

    high = 0x0;
    low = random(0x0, 0x0000000F);
    high = (high << 32);
    low = (low << 32) >> 32;

    return (low | high);
}

///////////////////////////////////////////////////////////////////////////////
// bit counting (BEGIN)
int iterated_bitcount (unsigned int n)
{
    int count=0;
    while (n) {
        count += n & 0x1u;
        n >>= 1 ;
    }
    return count;
}

static char bits_in_16bits [0x1u << 16] ;

void precompute_bitcount_16bit(void)
{
    unsigned int i ;
    for (i = 0; i < (0x1u<<16); i++)
        bits_in_16bits [i] = iterated_bitcount (i) ;
}

// NOTE: works only for 64-bit int
int compute_bitcount_64bit (unsigned long long n)
{
    return bits_in_16bits [n         & 0xffffu]
        +  bits_in_16bits [(n >> 16) & 0xffffu]
        +  bits_in_16bits [(n >> 32) & 0xffffu]
        +  bits_in_16bits [(n >> 48) & 0xffffu];
}

// inter-bit transitions
static char interbits_in_char [16]  = {
        0,  // 00 -> 00
        1,  // 00 -> 01
        1,  // 00 -> 10
        0,  // 00 -> 11
        1,  // 01 -> 00
        0,  // 01 -> 01
        2,  // 01 -> 10
        1,  // 01 -> 11
        1,  // 10 -> 00
        2,  // 10 -> 01
        0,  // 10 -> 10
        1,  // 10 -> 11
        0,  // 11 -> 00
        1,  // 11 -> 01
        1,  // 11 -> 10
        0   // 11 -> 11
};

// NOTE: works only for 64-bit int
int compute_interbit_trans_64bit(unsigned long long x, unsigned long long y)
{
    int total_interbit_trans = 0;

    for (int i=0; i<64; i++)
        total_interbit_trans += interbits_in_char[(((x >> i) & 0x3u) << 2) + ((y >> i) & 0x3u)];

    return total_interbit_trans;
}
// bit counting (END)
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
string zeroStr(int num_byte)
{
    string str = "";

    while (num_byte > 0) {
        str += "00";
        --num_byte;
    }

    return str;
}

string int2str(int i)
{
    char buf[64];

    sprintf(buf, "%d", i);
    return string(buf);
}

string longlong2str(long long l)
{
    char buf[64];

    sprintf(buf, "%lld", l);
    return string(buf);
}

string double2str(double d, int precision)
{
    char buf[64];

    sprintf(buf, "%.*lf", precision, d);
    return string(buf);
}

string ulonglong2hstr(unsigned long long l)
{
    string str = "";

    unsigned char c[8];
    for (int i=0; i<8; i++)
        c[7-i] = (l >> (8*i));

    for (int i=0; i<8; i++) {
        char buf[4];
        sprintf(buf, "%02X", c[i]);
        str += buf;
    }

    return str;
}

bool isZeroStr(string str)
{
    assert(str.length()%2 == 0);

    for (unsigned int n=0; n<str.length(); n++)
        if (str[n] != '0')
            return false;

    return true;
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
static map< char, unsigned int > hexa_str_int_map;

void prepare_hexa_str_int()
{
    hexa_str_int_map['0'] = 0x0;
    hexa_str_int_map['1'] = 0x1;
    hexa_str_int_map['2'] = 0x2;
    hexa_str_int_map['3'] = 0x3;
    hexa_str_int_map['4'] = 0x4;
    hexa_str_int_map['5'] = 0x5;
    hexa_str_int_map['6'] = 0x6;
    hexa_str_int_map['7'] = 0x7;
    hexa_str_int_map['8'] = 0x8;
    hexa_str_int_map['9'] = 0x9;
    hexa_str_int_map['A'] = 0xA;
    hexa_str_int_map['B'] = 0xB;
    hexa_str_int_map['C'] = 0xC;
    hexa_str_int_map['D'] = 0xD;
    hexa_str_int_map['E'] = 0xE;
    hexa_str_int_map['F'] = 0xF;
}

vector< unsigned long long > str2ullv(string str)
{
    vector< unsigned long long > data_vec;

    int str_len = str.size();
    int str_len_64bit = str_len / 16;
// printf("str_len=%d\n", str_len);
// printf("str_len_64bit=%d\n", str_len_64bit);

    for (int i=0; i<str_len_64bit; i++) {
        unsigned long long data_64bit = 0x0;
        for (int j=0; j<16; j++) {
            int offset = i*16 + j;
            data_64bit <<= 4;
            data_64bit |= hexa_str_int_map[str[offset]];
// printf("data_64bit=%016llX\n", data_64bit);
        }
        data_vec.push_back(data_64bit);
    }

    if (str_len > str_len_64bit*16) {
        assert((str_len - str_len_64bit*16) < 16);

        unsigned long long data_64bit = 0x0;
        for (int i=str_len_64bit*16; i<str_len; i++) {
            data_64bit <<= 4;
            data_64bit |= hexa_str_int_map[str[i]];
        }
        data_64bit <<= 4*(16 - (str_len - str_len_64bit*16));
        data_vec.push_back(data_64bit);
    }

    return data_vec;
}
///////////////////////////////////////////////////////////////////////////////

const char* get_router_power_model_name()
{
    switch (g_cfg.router_power_model) {
    case ROUTER_POWER_MODEL_ORION_CALL: return "Orion call";
    case ROUTER_POWER_MODEL_STATS: return "Orion stats";
    }

    return "Undefined";
}

const char* get_link_power_model_name()
{
    switch (g_cfg.link_power_model) {
    case LINK_POWER_MODEL_ORION: return "Orion call";
    case LINK_POWER_MODEL_DELAY_OPT_REPEATED: return "Delay-optimal";
    case LINK_POWER_MODEL_DELAY_OPT_REPEATED_VALUE: return "Delay-optimal + Value";
    }

    return "Undefined";
}

const char* get_sa_type_name(int sa_type)
{
    switch (sa_type) {
    case SW_ALLOC_MAXIMAL: return "Maximal";
    case SW_ALLOC_RR: return "RR";
    case SW_ALLOC_LRS: return "LRS";
    }

    return "Undefined";
}

const char* get_pkttype_name(unsigned int pkt_type)
{
    string str = "";

    switch (pkt_type) {
    case PACKET_TYPE_UNICAST_SHORT:	str += "US"; break;
    case PACKET_TYPE_UNICAST_LONG:	str += "UL"; break;
    case PACKET_TYPE_MULTICAST_SHORT:	str += "MS"; break;
    case PACKET_TYPE_MULTICAST_LONG:	str += "ML"; break;
    default:	break;
    }

    return str.c_str();
}

const char* get_router_buffer_type_name(int router_buffer_type)
{
    switch (router_buffer_type) {
    case ROUTER_BUFFER_SAMQ: return "SAMQ";
    case ROUTER_BUFFER_DAMQ_P: return "DAMQ_P";
    case ROUTER_BUFFER_DAMQ_R: return "DAMQ_R";
    default: ;
    } 

    return "Undefined";
}

const char* get_compression_scheme_name(int compression_scheme)
{
    switch (compression_scheme) {
    case CAM_MT_PRIVATE_PER_ROUTER: return "PRIVATE";
    case CAM_MT_PRIVATE_PER_CORE: return "PRIVATE_PER_CORE";
    case CAM_MT_SHARED_PER_ROUTER: return "SHARED";
    default: ;
    } 

    return "Undefined";
}
///////////////////////////////////////////////////////////////////////////////

// test if n is the power of 2.
const bool is_power2(int n)
{
    return (n & (n-1)) ? false : true;
}

/// return log_base(n)
const unsigned int log_int(int n, int base)
{
    if (n < base)
        return 0;

    unsigned int k = 0;
    while (n) {
        k++;
        n /= base;
    }

    return k-1;
}

/// return n^e
const unsigned int pow_int(int n, int e)
{
    if (e <= 0)
        return 1;

    unsigned int k = 1;
    while (e--)
        k *= n;

    return k;
}

///////////////////////////////////////////////////////////////////////////////
// DMESH topology
// Assumption: concentration degree is 4
//             the size of column is the same as that of row.
int getRouterRelativeID(int core_id, int num_cores_per_dim, int num_routers_per_dim)
{
    int core_x_coord = core_id%num_cores_per_dim;
    int core_y_coord = core_id/num_cores_per_dim;

    // determine if this core is on the even/odd position in one dimension.
    int router_x_coord = core_x_coord / 2;
    int router_y_coord = core_y_coord / 2;
    int router_id = router_y_coord*num_routers_per_dim + router_x_coord;
//printf("     c=%d (x=%d, y=%d), r=%d (x=%d, y=%d)]\n", core_id, core_x_coord, core_y_coord, router_id, router_x_coord, router_y_coord);

    return router_id;
}
///////////////////////////////////////////////////////////////////////////////
