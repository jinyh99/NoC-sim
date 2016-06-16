#####################################################################
# directory setup
#CSIM_DIR=/usr/local/csim/lib
#CSIM_CPP_DIR=/usr/local/csim_cpp-19.0/lib
CSIM_CPP_DIR=../csim_cpp-19.0/lib
ORION_DIR=../orion_src
TARGET_DIR=../bin
BACKUP_DIR=../backup
#OBJ_DIR=../obj
OBJ_DIR=.

# command setup
CC=g++
RM=rm
TAR=tar

# executable
TARGET=	$(TARGET_DIR)/noc

# compile and link flags
CSIM_CC_FLAGS=-DCPP

#OPT_FLAGS+=-O2 -finline-functions	# recommended for simulation
#OPT_FLAGS+=-O3
#OPT_FLAGS+=-O3 -DNDEBUG
#OPT_FLAGS+=-g
#OPT_FLAGS+=-Wall
#OPT_FLAGS+=-Wall -Werror -Wno-unknown-pragmas

DEBUG_FLAGS+=-D_DEBUG_ROUTER -D_DEBUG_ROUTER_START_CLK=0.0
DEBUG_FLAGS+=-D_DEBUG_ROUTER_RC
DEBUG_FLAGS+=-D_DEBUG_ROUTER_VA
DEBUG_FLAGS+=-D_DEBUG_ROUTER_SA
DEBUG_FLAGS+=-D_DEBUG_ROUTER_ST
DEBUG_FLAGS+=-D_DEBUG_ROUTER_LT
DEBUG_FLAGS+=-D_DEBUG_ROUTER_TN
#DEBUG_FLAGS+=-D_DEBUG_ROUTER_SNAPSHOT -D_DEBUG_ROUTER_SNAPSHOT_CLK=82.0
#DEBUG_FLAGS+=-D_DEBUG_ROUTER_PROCESS
#DEBUG_FLAGS+=-D_DEBUG_CREDIT
#DEBUG_FLAGS+=-D_DEBUG_CHECK_BUFFER_INTEGRITY

# w/ Orion
CFLAGS+=-I$(ORION_DIR) -DORION_MODEL
#CFLAGS+=-I$(ORION_DIR) -DORION_MODEL -DLINK_DVS
LDFLAGS+=-L$(ORION_DIR)

# profile
#CFLAGS+=-pg
#LDFLAGS+=-pg

CFLAGS+=-I$(CSIM_CPP_DIR) $(CSIM_CC_FLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS)
CPPFLAGS=$(CFLAGS)
#LDFLAGS+=-lm -lz
LDFLAGS+=-lm

#32-bit executables on 64-bit architecture
CFLAGS+=-m32
LDFLAGS+=-m32

#####################################################################
# library files
#LIBS+=$(CSIM_DIR)/csim.a
LIBS+=$(CSIM_CPP_DIR)/csim.cpp.a
LIBS+=$(ORION_DIR)/libpower.a
#LIBS+=../lib32/libz.a

# source files
SRC_C=	gzlib.c \

SRC_CPP=main.C \
        parse_arg.C \
	profile.C \
        sim_process.C \
	gzstream.C \
	util.C \
        Flit.C \
	FlitQ.C \
	Packet.C \
	Router.C \
	RouterTunnel.C \
	RouterDebug.C \
	RouterPowerOrion.C \
	RouterPowerStats.C \
	LinkPowerOrion.C \
	LinkPowerRepeated.C \
	LinkPowerRepeatedValue.C \
	Core.C \
	NIInput.C \
	NIInputCompr.C \
	NIOutput.C \
	NIOutputDecompr.C \
	RoutingMeshStatic.C \
	RoutingMeshMinAdaptive.C \
	RoutingFatTree.C \
	RoutingTFatTree.C \
	RoutingTorusStatic.C \
	RoutingDMeshStatic.C \
	RoutingHMeshStatic.C \
	RoutingFlbfly.C \
	Topology.C \
	Topology2DMesh.C \
	Topology2DTorus.C \
	TopologyFatTree.C \
	TopologyDMesh.C \
	TopologyHMesh.C \
	TopologySNUCA.C \
	TopologyFlbfly.C \
	WorkloadGEMSType.C \
	WorkloadSynthetic.C \
	WorkloadSyntheticMatrix.C \
	WorkloadTiledCMP.C \
	WorkloadTiledCMPValue.C \
	WorkloadSNUCAValue.C \
	WorkloadTRIPS.C \
	LinkDVSer.C \
	SwArb.C \
	SwArb2Stage.C \
	VCArb.C \
	VCArbFCFS.C \
	CAMDataEnPrivate.C \
	CAMDataEnShared.C \
	CAMDataDePrivate.C \
	CAMDataDeShared.C \
	CAMManager.C \

# Orion object files
ORION_OBJS = $(ORION_DIR)/SIM_power_util.o \
             $(ORION_DIR)/SIM_router_power.o \
             $(ORION_DIR)/SIM_power_router.o \
             $(ORION_DIR)/SIM_power_misc.o \
             $(ORION_DIR)/SIM_power_static.o \
             $(ORION_DIR)/SIM_power_array_l.o \
             $(ORION_DIR)/SIM_power_time.o \
             $(ORION_DIR)/SIM_power_cam.o \
             $(ORION_DIR)/SIM_power_array_m.o \
             $(ORION_DIR)/SIM_power_test.o \

#SOURCES = $(OBJECTS:%.o=%.c)

OBJ = $(addprefix $(OBJ_DIR)/,$(SRC_C:.c=.o) $(OBJ_DIR)/$(SRC_CPP:.C=.o))
DEP = $(addprefix $(OBJ_DIR)/,$(SRC_C:.c=.d) $(SRC_CPP:.C=.d))

#####################################################################
#.cc.o:
.C.o:
.c.o:

$(TARGET): $(OBJ) $(LIBS)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS) $(LDFLAGS)
#	$(CC) -o $(TARGET) $(OBJ) $(ORION_OBJS) $(LIBS) $(LDFLAGS)

backup:
	$(TAR) cfz $(BACKUP_DIR)/nocpp_`date +%m%d%Y`.tar.gz *.h $(SRC_C) $(SRC_CPP) Makefile

clean:
	$(RM) -f $(OBJ) $(TARGET)

#####################################################################
# auto dependencies
#makefile.dep: *.[Cch]
#	$(RM) -f makefile.dep
#	for i in *.[Cc]; do $(CC) -MM "$${i}"; done > makefile.dep
#-include makefile.dep

