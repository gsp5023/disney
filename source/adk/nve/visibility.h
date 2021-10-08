#ifndef __NVE_VIS__
#define __NVE_VIS__

#ifndef NVE_VISIBLE
#ifdef _MSC_VER
#define NVE_VISIBLE
#else
#define NVE_VISIBLE __attribute__((visibility("default")))
#endif

#endif

#endif
