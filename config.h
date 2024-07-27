#ifndef CFG_H
#define CFG_H
#include <cstddef>

constexpr size_t MTU=9600;
constexpr size_t MBUF_SIZE=MTU+18+128;
constexpr size_t PKT_LEN=8500;
constexpr size_t N_PT_PER_FRAME=4096;
#endif
