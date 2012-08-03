
#ifndef GCFG_UNIT_TEST_EN
#error "undef GCFG_UNIT_TEST_EN"
#endif


#if GCFG_UNIT_TEST_EN > 0

bool ut_init(void);

// invoke a task to start a unit test.
// return: test task id
typedef int (*ut_task_start_t)(void);
bool ut_task_open( ut_task_start_t start );
void ut_task_pump(void);

bool ut_task_running(void);

#else
#define ut_task_pump()

#endif

// no more
