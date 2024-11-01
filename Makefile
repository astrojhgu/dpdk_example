all: recv send

DPDK_INC=`pkg-config --cflags libdpdk`
DPDK_LIB=`pkg-config --libs libdpdk`
CXX=gcc
OPT=-O3 -g

payload.o: payload.cpp payload.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

send.o: send.cpp utils.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

send: send.o payload.o 
	g++ $^ -o $@ $(DPDK_LIB) $(OPT)

recv.o: recv.cpp utils.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

recv: recv.o payload.o
	g++ $^ -o $@ $(DPDK_LIB) $(OPT)

clean:
	rm -f *.o
