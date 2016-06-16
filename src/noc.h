#ifndef _NOC_H_
#define _NOC_H_

using namespace std;
#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <bitset>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

// STL set type conflict with CSIM set
typedef set< int > IntSet;
typedef set< string > StringSet;

#include <cpp.h>	// CSIM

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include "noc_def.h"
#include "Config.h"
#include "SimOut.h"
#include "Flit.h"
#include "Credit.h"
#include "Pool.h"
#include "util.h"
#include "StatUtil.h"

////////////////////////////////////////////////////////////////////
// global variable declaration
class Topology;
class Routing;
class Router;
class NIInput;
class NIOutput;
class Core;
class Workload;
#ifdef LINK_DVS
class LinkDVSer;
#endif
class CAMManager;

extern event* g_ev_sim_done;
extern bool g_EOS;

extern Config g_cfg;
extern SimOut g_sim;
extern Routing* g_Routing;
extern Topology* g_Topology;
extern vector< Router* > g_Router_vec;
extern vector< NIInput* > g_NIInput_vec;
extern vector< NIOutput* > g_NIOutput_vec;
extern vector< Core* > g_Core_vec;
extern Workload* g_Workload;
extern Pool< Packet > g_PacketPool;
extern Pool< FlitHead > g_FlitHeadPool;
extern Pool< FlitMidl > g_FlitMidlPool;
extern Pool< FlitTail > g_FlitTailPool;
extern Pool< FlitAtom > g_FlitAtomPool;
extern Pool< Credit > g_CreditPool;
#ifdef LINK_DVS
extern LinkDVSer g_LinkDVSer;
#endif
extern CAMManager* g_CamManager;
////////////////////////////////////////////////////////////////////
#endif // #ifndef _NOC_H_
