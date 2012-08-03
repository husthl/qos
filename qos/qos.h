
bool qos_init(void);
// return: tick again. true, continue. false, stop tick run, exit qos.
bool qos_tick(void);
bool qos_exit(void);
const char* qos_version(void);

// port---------------------------------
bool qos_start(void);
// because these function is high related with os system
// so write in port
int elapse_us_open(void);
int elapse_us_reset( int elapse_id );
int elapse_us_update( int elapse_id );
int elapse_us_close( int elapse_id );

// tick---------------------------------
#define MS_TO_US( ms )              ( (ms) * 1000 )
#define SEC_TO_MS( sec )            ( (sec) * 1000 )
#define MINUTE_TO_SEC( minute )     ( (minute) * 60  )
#define HOUR_TO_MINUTE( hour )      ( (hour) * 60  )

#define SEC_TO_US( sec )            ( SEC_TO_MS( MS_TO_US( sec ) ) )
#define MS_TO_SEC( ms )				((unsigned int)( (ms) / 1000 ))

#define LOOP_TIMES( timeout_total, timeout_once )				\
	( (timeout_total) / (timeout_once) )						\
	+ ( ( (timeout_total) % (timeout_once) ) == 0 ? 0 : 1 )



// qos base timer tick, times every second
#define TICK_PER_SEC                GCFG_QOS_TICK_HZ

// 所有计时单位为：us
#define US_PER_TICK                 ( MS_TO_US( SEC_TO_MS( 1 ) ) / TICK_PER_SEC )
#define US_TO_TICK( us )            ( (us) / US_PER_TICK )
#define MS_TO_TICK( ms )            ( US_TO_TICK( MS_TO_US( ms ) ) )
#define SEC_TO_TICK( sec )          ( MS_TO_TICK( SEC_TO_MS( sec ) ) )

#if US_PER_TICK <= GCFG_QOS_TICK_FREE_MAX_US
#error "There is no time to run idle"
#endif


#define QOS_TICK_STATE_UNKNOW       0
#define QOS_TICK_STATE_TIMER        1
#define QOS_TICK_STATE_IDLE         2
#define QOS_TICK_STATE_FREE         3
int qos_tick_status(void);
int qos_tick_cnt(void);
int qos_last_task_id(void);

// a tick:
// |  timer_time   idle_time   free_time  |
typedef struct{
    int timer_us;
    int idle_us;
    int free_us;
}tick_time_t;
bool qos_last_tick_time( tick_time_t* last_tick_time );
int qos_tick_run_us(void);


// qos busy reason
// tick too busy, when after run all timer, there is no time for idle, and reach to next tick.
// you can: 
// 1. reduce timer number. 
// 2. increase cpu frequence.
// 3. split one tick timer's operation into server ticks.
#define QOS_BUSY_TOO_MUCH_TIMER                 1       
// last idle run too much time. after last idle runned, time reach to next tick.
// you can:
// 1. increase cpu frequence.
// 2. increase GCFG_QOS_TICK_FREE_MAX_US to run idle more time every tick.
// 3. split one idle's operation into server function.
#define QOS_BUSY_LAST_IDLE_TOO_LONG             2
// when the cpu too busy, below callback will be called.
// busy_reason          busy reason
typedef void (*qos_busy_t)( int busy_reason );
// if you want to get warnnig in qos busy, you can qos_set_busy( qos_busy_default_warnning );
void qos_busy_default_warnning( int busy_reason );
bool qos_set_busy( qos_busy_t busy );


// task---------------------------------
// return: run again? 
//      true, the callback will be invoked again. 
//      false, stop called the callback.
typedef bool (*qos_entry_t)( void* arg );
bool qos_task_close_real( int task_id );
#define qos_task_close( task_id )   (bool)({ const bool b = qos_task_close_real( task_id ); b; })
bool qos_task_set_entry( int task_id, qos_entry_t entry, void* arg );
// if task is idle, will sleep once.
// if task is timer, will change it's interval_us_ori. the next time timer called is after us.
bool qos_task_sleep_us( int task_id, int us );
bool qos_task_set_name( int task_id, const char* task_name );
const char* qos_task_name( int task_id );


// task notify event
#define QOS_TASK_EVNET_SET_ENTRY    1
#define QOS_TASK_EVNET_SLEEP_US     2
#define QOS_TASK_EVNET_CLOSE        3       // at task close, task is ready to quit
#define QOS_TASK_EVNET_DELETE       4       // before task memory free, it's the last event
// event        
//              you can use as: if( event == QOS_TASK_EVNET_CLOSE ){ do sth... }
// return: notify again?    false, stop notify; true, notify again.
typedef bool (*qos_task_notify_t)( int task_id, int event, void* arg );
bool qos_task_set_notify( int task_id, qos_task_notify_t notify, void* arg );


// return: task id
int qos_timer_open( qos_entry_t entry, void* arg, int interval_us );
// return: task id
int qos_idle_open( qos_entry_t entry, void* arg );


// no more
