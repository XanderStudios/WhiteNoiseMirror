//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 16:45:54
//

#define _USE_MATH_DEFINES
#include <cstdlib>
#include <cmath>

#include "wn_util.h"

u64 wn_hash(const void *key, u32 len, u64 seed)
{
    const u64 m = 0xc6a4a7935bd1e995ULL;
    const u32 r = 47;

    u64 h = seed ^ (len * m);
    const u64 * data = (const u64 *)key;
    const u64 * end = data + (len/8);
    while (data != end) {
        u64 k = *data++;
        k *= m;
        k ^= k >> r;
        k *= m;
        
        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;
    switch(len & 7) {
        case 7: h ^= u64(data2[6]) << 48;
        case 6: h ^= u64(data2[5]) << 40;
        case 5: h ^= u64(data2[4]) << 32;
        case 4: h ^= u64(data2[3]) << 24;
        case 3: h ^= u64(data2[2]) << 16;
        case 2: h ^= u64(data2[1]) << 8;
        case 1: h ^= u64(data2[0]);
                h *= m;
    };
    
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

u64 wn_uuid()
{
    return std::rand();
}
