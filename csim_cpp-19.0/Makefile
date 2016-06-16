CXX = g++
LDFLAGS = 
CXXFLAGS =
CPPFLAGS = -m32
MAKE = make

all: ex1cpp ex2cpp

lib/csim.cpp.a:
	(cd lib && $(MAKE))

ex1cpp: ex1cpp.cpp lib/csim.cpp.a
	$(CXX) -DCPP -Ilib$(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o ex1cpp ex1cpp.cpp lib/csim.cpp.a -lm
	#$(CXX) -DCPP -Ilib $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o ex1cpp ex1cpp.cpp lib/csim.cpp.a -lm

ex2cpp: ex2cpp.cpp lib/csim.cpp.a
	$(CXX) -DCPP -Ilib $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o ex2cpp ex2cpp.cpp lib/csim.cpp.a -lm

clean:
	rm -f ex1cpp ex2cpp
