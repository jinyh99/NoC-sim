######################################################################
######################################################################
TARGET = polaris
OBJECTS = SIM_power_util.o \
	SIM_router_power.o SIM_power_router.o SIM_power_misc.o SIM_power_static.o \
	SIM_power_array_l.o SIM_power_time.o SIM_power_cam.o SIM_power_array_m.o SIM_power_test.o
SOURCES = $(OBJECTS:%.o=%.c)
LFLAGS =
CC	= g++
AR	= ar
RANLIB	= ranlib
#CFLAGS	= -g -DPOWER_TEST -DPARM_TECH_POINT=10 -DVdd=1.2
CFLAGS	= -O2 -DPOWER_TEST -DPARM_TECH_POINT=10 -DVdd=1.0 -m32
LIB	= libpower.a

all: $(LIB)

$(LIB): $(OBJECTS)
	$(AR) cr $@ $?
	$(RANLIB) $@


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS) *~
