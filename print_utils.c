#include <stdio.h>
#ifdef WIN32
#include "stdint.h"
#else
#include <stdint.h>
#endif

#include "readonly_set_cfg.h"
#include "print_utils.h"

const char* hex_chars = "0123456789ABCDEF";

DEFINE_PRINT_FUNCS(item_t,)

