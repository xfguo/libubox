#ifndef _HASH_H__
#define _HASH_H__

#include <stdint.h>

uint32_t hash_murmur2(const void * key, int len);

#endif
