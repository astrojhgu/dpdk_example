all: recv send

DPDK_INC=`pkg-config --cflags libdpdk`
DPDK_LIB=`pkg-config --libs libdpdk`
YAML_LIB=`pkg-config --libs yaml-cpp`
CXX=gcc
OPT=-O3 -g

payload.o: payload.cpp config.h  payload.h  utils.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

send.o: send.cpp config.h  payload.h  utils.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT) #-fopenmp

send: send.o payload.o 
	g++ $^ -o $@ $(DPDK_LIB) $(YAML_LIB) $(OPT) #-fopenmp

recv.o: recv.cpp config.h  payload.h  utils.h
	g++ -c $< -o $@ $(DPDK_INC) $(OPT)

recv: recv.o payload.o
	g++ $^ -o $@ $(DPDK_LIB) $(YAML_LIB) $(OPT)

clean:
	rm -f *.o
