
#define GCFG_DO_RELEASE                     1
#define GCFG_DO_FREE                        2
// if release, set it as GCFG_DO_RELEASE
// if debug, unit test, or other self define, set it as GCFG_DO_FREE
#define GCFG_DO                             GCFG_DO_FREE    // GCFG_DO_RELEASE 
//#define GCFG_DO                             GCFG_DO_RELEASE 

#if GCFG_DO == GCFG_DO_RELEASE
#include "config_release.h"
#elif GCFG_DO == GCFG_DO_FREE


#define GCFG_DEBUG_EN                       1

#define GCFG_UNIT_TEST_EN                   1
#define GCFG_UNIT_TEST_TIME                 1

// 内存信息记录、边界检查
#define GCFG_MEM_EN                         1


#define GCFG_QOS_TICK_HZ                    (10)       // 100ms a tick
// every tick, first run tick, if the time to next tick more than GCFG_QOS_TICK_FREE_MAX_US,
// then run idle. idle may be run more than one time in one tick. If the time to next tick
// less than GCFG_QOS_TICK_FREE_MAX_US，do nothing, it's free time for cpu.
#define GCFG_QOS_TICK_FREE_MAX_US           (10000)

#else
#error "GCFG_DO error value"
#endif

// no more------------------------------
