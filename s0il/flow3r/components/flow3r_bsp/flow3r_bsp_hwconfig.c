#include "sdkconfig.h"

#if defined(CONFIG_FLOW3R_HW_GEN_P3)
const char *flow3r_bsp_hw_name = "proto3";
#elif defined(CONFIG_FLOW3R_HW_GEN_P4)
const char *flow3r_bsp_hw_name = "proto4";
#elif defined(CONFIG_FLOW3R_HW_GEN_C23)
const char *flow3r_bsp_hw_name = "camp23";
#else
#error "Badge23 Hardware Generation must be set!"
#endif
