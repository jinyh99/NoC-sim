#ifndef _UTIL_H_
#define _UTIL_H_

int factorial(int n);
int combination(int A, int B);
vector< string > split_str(string str, char separator);
unsigned long long random_64bit();
unsigned long long pattern_64bit();
string ulonglong2hstr(unsigned long long value);
void prepare_hexa_str_int();
vector< unsigned long long > str2ullv(string str);
bool isZeroStr(string str);
string zeroStr(int num_byte);
string int2str(int i);
string longlong2str(long long l);
string double2str(double d, int precision=0);

void precompute_bitcount_16bit(void);
int compute_bitcount_64bit(unsigned long long n);
int compute_interbit_trans_64bit(unsigned long long x, unsigned long long y);

const char* get_router_power_model_name();
const char* get_link_power_model_name();
const char* get_sa_type_name(int sa_type);
const char* get_pkttype_name(unsigned int pkt_type);
const char* get_router_buffer_type_name(int router_buffer_type);
const char* get_compression_scheme_name(int compression_scheme);

const bool is_power2(int n);
const unsigned int log_int(int n, int base);
const unsigned int pow_int(int n, int e);

int getRouterRelativeID(int core_id, int num_cores_per_dim, int num_routers_per_dim);

#endif // #ifdef _UTIL_H_
