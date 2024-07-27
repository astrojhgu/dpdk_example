all: recv send

DPDK_INC=`pkg-config --cflags libdpdk`
DPDK_LIB=`pkg-config --libs libdpdk`
CXX=gcc
OPT=-O3 -g

send.o: send.cpp
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

send: send.o
	g++ $< -o $@ $(DPDK_LIB) $(OPT)

recv.o: recv.cpp
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

recv: recv.o
	g++ $< -o $@ $(DPDK_LIB) $(OPT)

clean:
	rm -f *.o
