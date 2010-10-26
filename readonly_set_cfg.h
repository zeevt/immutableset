#ifndef READONLY_SET_CFG_H_
#define READONLY_SET_CFG_H_

#ifdef WIN32
#include "inttypes.h"
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 1 /* Microsoft doesn't support C99 */
#else
#include <inttypes.h>
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 
#endif

#define CACHE_LINE_BYTES 64
#define item_bits        32
#if item_bits == 16
#define item_t           uint16_t
#define PR_item_t        PRIu16
#define ITEM_T_C(n)      INT16_C(n)
#elif item_bits == 32
#define item_t           uint32_t
#define PR_item_t        PRIu32
#define ITEM_T_C(n)      INT32_C(n)
#elif item_bits == 64
#define item_t           uint64_t
#define PR_item_t        PRIu64
#define ITEM_T_C(n)      INT64_C(n)
#else
#error item_bits has to be one of: 16, 32, 64.
#endif

#endif /* READONLY_SET_CFG_H_ */
