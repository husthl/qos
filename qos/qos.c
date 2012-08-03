/*
 * File      : qos.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>


#include "inc.h"
#include "qemdlink.h"
#include "qmem.h"
#include "qmem_task.h"
#include "vfs.h"


#define this_file_id   file_id_qos


/*
qos任务调度原理。



tick
    qos将cpu的时间，划分为一个个的tick，每个tick，占用固定长的时间。qos用频率（每秒tick数，单位赫兹hz）来
设置qos的tick长度。


task
qos任务有两种：定时任务timer，和空闲任务idle。
所有timer优先级相同，idle优先级相同。
timer在每个tick都会被调用，按照注册的先后次序。
idle则在每个tick中，运行timer后，尽可能多次数运行idle，同时，尽量不占用下一个tick时间。每个idle，在所有时间内，运行机会平等。


tick的时间划分
    一次tick，时间被划分为3个部分：
    timer_time   idle_time    free_time
1   运行timer的时间timer_time；
2   运行idle的时间idle_time；
3   空闲时间free_time。空闲时间是为了在一个tick内，保证cpu尽可能多的运行idle；同时，尽量不占用下一tick周期时间。


free_time
    在每个tick尾部，有一小段空闲（free）时间，这一段时间，什么也不做。这是为了保证系统每个timer都能按时被调用。
free时间随着任务的闲、忙，free时间长度会发生变化，但其最长时间，不超过GCFG_QOS_TICK_FREE_MAX_US。
即每个tick距离下个tick的最大空闲时间，不会大于GCFG_QOS_TICK_FREE_MAX_US。
    这段时间，可以用一个很短的任务代替，或者让cpu进入省电状态。


example:
    下面举例说明：
    假设有3个timer，记为t1,t2,t3；3个idle，记为i1,i2,i3;cpu的空闲时间为f( 意为free )。
    下面是个典型的时序：
    |           tick1            |          tick2          |           tick3            |          tick4         |
    | t1   t2   t3   i1    i2  f | t1   t2   t3    i3      |  t1   t2   i1  i2 i3 f     | t1 t2 i1 i2 i3 i1 i2 f |
    每个tick任务调度如下：
    先说timer任务的调度：
1   定时任务在每个tick都会被调度。
2   在tick2，t3任务结束，于是在tick3,tick4不再出现t3。
    再说idle任务调度和free时间:
1   tick1，i1,i2被调用后，free时间小于GCFG_QOS_TICK_FREE_MAX_US，即下一个tick已经快到了，因此，i3没有机会运行。
2   tick2，轮到i3被调用了。但i3运行的时间很长，运行完后，下一个tick已经到了，连free时间都没有，因此，没有运行i1,i2。
    可能会占用tick3的一小段时间，这种情况，应该尽量避免，可以通过分解i3来使每个tick的时间都能够控制在规定时间内。
3   tick3，i1,i2,i3都被调用了，tick尾部，有一段free时间。
4   tick4，i1,i2,i3依次运行完后，又运行了i1,i2，但没有时间运行i3了。可以看到，一个tick内，会尽可能多的运行idle；
    可以预见，下一个tick来时，第一个运行的idle任务应该是i3。


qos用int来存储us(微秒)变量，计量tick，是否合适?
    Answer: int最大值为2 147 483 648，1秒=1 000 000us。
因此，int型最大可以存储2147.48365秒=35分钟=半小时，
我们平常接触的cpu，一般一个tick频率范围为1Hz~1 000 000Hz
 即1秒到1us范围内变化，一般这些us计量值，都是
 一个tick就会清零，偶尔会超过一些，但不会溢出到半个小时。
所以，用int型来计量us，完全足够。


任务状态的切换
启动 -> tick状态 <->  运行状态 -> dead -> delete
tick状态：tick状态，对于定时任务、处于休眠的idle任务，都需要对任务进行计时，决定什么时候再次调用
运行状态：即任务调用任务入口函数状态。
dead：任务处于准备消亡的状态。
delete:任务正在消亡，删除相关的运行空间。

链表：
1   task    串接所有task.
2   tick    串接所有需要tick的task，包括所有timer，以及正在sleep的idle
3   dead    串接所有已经dead的task
4   timer   串接需要run的timer
5   idle    串接需要run的idle
有如下事实：
1   进入dead状态后，就不能返回到其他状态。
2   因为timer和idle是用不同的策略进行调度的，因此，用不同的链表timer和idle串接。
3   timer可以在tick和timer两个链表之间切换。
    idle可以在tick和idle两个链表之间切换。

*/
 
//{{{ data




typedef struct{
    bool tick_again;

    int this_task_id;
    int last_task_id;         // if last task close, it will be 0.

    tick_time_t last_tick_time;
    tick_time_t this_tick_time;

    int tick_status;
    int tick_cnt;
}tick_t;
static tick_t m_tick = { 
    true, 
    0, 0,
    { 0, 0, 0 }, { 0, 0, 0 },
    QOS_TICK_STATE_UNKNOW, 0
};



static bool qos_at_exit(void);

// There are serval link:
// 1    task
// 2    timer
// 3    idle
// 4    notify


typedef bool (*task_delete_t)( int task_id );
typedef bool (*task_dead_t)( int task_id );
typedef bool (*task_sleep_us_t)( int task_id, int us );
// return: true, continue tick. false, no more tick, remove the node from tick link.
typedef bool (*task_tick_t)( int task_id );
typedef struct task_s{
    qos_entry_t entry;
    void* arg;

    const char* name;

    task_delete_t delete;
    task_dead_t dead;
    task_sleep_us_t sleep_us;
    task_tick_t tick;

    int notify_link;

    qemdlink_node_t node_dead;      // link all dead task.
    qemdlink_node_t node_tick;      // some task needed tick link with this node
    qemdlink_node_t node;           // all task link with this node
}task_t;
static bool task_in_dead_link( int task_id );
static bool task_in_task_link( int task_id );
static bool task_in_tick_link( int task_id );

typedef struct{
    task_t task;

    int interval_us_ori;
    int interval_us;

    qemdlink_node_t node;
}qtimer_t;

typedef struct{
    task_t task;

    int delay_us;

    qemdlink_node_t node;
}idle_t;


//}}}



//{{{ notify
typedef struct{
    qos_task_notify_t task_notify;
    void* arg;

    qemdlink_node_t node;
}notify_t;
static qemdlink_node_t* notify_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    notify_t* const p = (notify_t*)ptr;
    return &(p->node);
}
static void* notify_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, notify_t, node );
    return p;
}
// return: link id
static int notify_link_open(void){
    const int link = qemdlink_open( notify_ptr_to_node, notify_node_to_ptr );
    return_0_if( 0 == link );
    return link;
}



static bool notify_open( int link, qos_task_notify_t task_notify, void* arg ){
    uverify( 0 != link );
    uverify( NULL != task_notify );

    notify_t* const notify = qmalloc( sizeof( notify_t ) );
    return_false_if( NULL == notify );

    if( GCFG_DEBUG_EN ){
        memset( notify, 0, sizeof( notify_t ) );
    }
    return_false_if( !qemdlink_node_init( &notify->node ) );
    notify->task_notify = task_notify;
    notify->arg = arg;

    return_false_if( !qemdlink_append_last( link, notify ) );
    return true;
}
static bool notify_close( int link, notify_t* notify ){
    uverify( 0 != link );
    uverify( NULL != notify );

    return_false_if( !qemdlink_remove( link, notify ) );
    qfree( notify );
    return true;
}



static bool notify_link_close( int link ){
    uverify( 0 != link );
    
    for(;;){
        notify_t* const notify = qemdlink_first( link );
        if( NULL == notify ){
            return_false_if( !qemdlink_close( link ) );
            link = 0;
            return true;
        }

        return_false_if( !notify_close( link, notify ) );
    }

    uverify( false );
    return false;
}



static bool notify_one( int link, notify_t* notify, int task_id, int event ){
    uverify( 0 != link );
    uverify( NULL != notify );
    uverify( 0 != task_id );
    uverify( 0 != event );
    
    const qos_task_notify_t task_notify = notify->task_notify;
    void* const arg = notify->arg;
    uverify( NULL != task_notify );

    if( !task_notify( task_id, event, arg ) ){
        // delete notify
        return_false_if( !notify_close( link, notify ) );
    }
    return true;
}
typedef struct{
    int task_id;
    int event;
}notify_task_callback_t;
static bool notify_task_callback( int link, void* ptr, int ix, void* void_arg ){
    uverify( 0 != link );
    notify_t* const notify = (notify_t*)ptr;
    uverify( NULL != notify );
    uverify( 0 <= ix );
    const notify_task_callback_t* const arg = (notify_task_callback_t*)void_arg;
    uverify( NULL != arg );
    const int task_id = arg->task_id;
    const int event = arg->event;

    return_false_if( !notify_one( link, notify, task_id, event ) );
    return true;
}
static bool notify_task( int link, int task_id, int event ){
    uverify( 0 != link );
    uverify( 0 != task_id );
    uverify( 0 != event );

    notify_task_callback_t arg = { task_id, event };
    qemdlink_for_each( link, notify_task_callback, &arg );
    return true;
}




//}}}



//{{{ tick
// tick是个链表，将所有需要tick的task，链接在一起。这样，遍历该链表，即可tick所有需要tick的task，
// 在有很多idle时，节省cpu开销。
// tick串接的，就是task，无需qmalloc新的空间。
// 但同时带来问题：因为一个task是被两个链表共享，所以，在task被del之前，要将其从tick链表中remove，带来同步问题。
static qemdlink_node_t* tick_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    task_t* const p = (task_t*)ptr;
    return &(p->node_tick);
}
static void* tick_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, task_t, node_tick );
    return p;
}
// tick link, link all task need tick.
static int m_tick_link = 0;
static bool tick_init(void){
    m_tick_link = qemdlink_open( tick_ptr_to_node, tick_node_to_ptr );
    return_false_if( 0 == m_tick_link );
    return true;
}




static bool task_in_tick_link( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    
    return qemdlink_in( m_tick_link, task );
}
// return: ok?
static bool tick_open( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    uverify( 0 != m_tick_link );

    // if task in dead status, cannot tick
    /*
    if( task_in_dead_link( task_id ) ){
        return true;
    }
    */

    uverify( task_in_task_link( task_id ) );
    // if task in dead link, task cannot put in tick link
    uverify( !task_in_dead_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );

    return_false_if( !qemdlink_append_last( m_tick_link, task ) );
    return true;
}
static bool tick_close( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    uverify( 0 != m_tick_link );

    if( !task_in_tick_link( task_id ) ){
        return true;
    }

    return_false_if( !qemdlink_remove( m_tick_link, task ) );
    return true;
}




static bool tick_task_dead_all(void){
    for(;;){
        task_t* const task = qemdlink_first( m_tick_link );
        if( NULL == task ){
            return true;
        }
        const int task_id = (int)task;
        return_false_if( !qos_task_close( task_id ) );
    }

    uverify( false );
    return false;
}
static bool tick_exit(void){
    uverify( 0 != m_tick_link );
    return_false_if( !tick_task_dead_all() );

    return true;

            /*
    for(;;){
        task_t* const task = qemdlink_first( m_tick_link );
        if( NULL == task ){
            return_false_if( !qemdlink_close( m_tick_link ) );
            m_tick_link = 0;
            return true;
        }

        const int task_id = (int)task;
        return_false_if( !tick_close( task_id ) );
    }

    uverify( false );
    return false;
    */
}
static bool tick_link_close(void){
    uverify( 0 != m_tick_link );
    uverify( qemdlink_empty( m_tick_link ) );
    return_false_if( !qemdlink_close( m_tick_link ) );
    m_tick_link = 0;
    return true;
}







static bool tick_loop_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_tick_link == link_id );
    task_t* const task = ptr;
    uverify( NULL != task );
    uverify( 0 <= ix );
    uverify( NULL == void_arg );

    const task_tick_t task_tick = task->tick;
    const int task_id = (int)task;
    // if task put in dead link, cannot tick again.
    uverify( !task_in_dead_link( task_id ) );
    //printf( "qos tick task: %s\n", qos_task_name( task_id ) );
    if( !task_tick( task_id ) ){
        // detach from the tick link
        return_false_if( !tick_close( task_id ) );
    }
    if( !m_tick.tick_again ){
        return false;
    }
    return true;
}
static bool tick_loop(void){
    qemdlink_for_each( m_tick_link, tick_loop_callback, NULL );
    return true;
}



//}}}



//{{{ cpu
// cpu是用来测量cpu消耗的程序段，包括tick消耗的时间测量，cpu busy告警。

static int m_cpu_elapse_id = 0;
static bool cpu_init(void){
    uverify( 0 == m_cpu_elapse_id );

    m_cpu_elapse_id = elapse_us_open();
    return_false_if( 0 == m_cpu_elapse_id );
    return true;
}

static bool cpu_exit(void){
    uverify( 0 != m_cpu_elapse_id );
    return_false_if( 0 > elapse_us_close( m_cpu_elapse_id ) );
    m_cpu_elapse_id = 0;
    return true;
}



static bool tick_start(void){
    uverify( 0 != m_cpu_elapse_id );
    return_false_if( 0 > elapse_us_reset( m_cpu_elapse_id ) );
    return true;
}
int qos_tick_run_us(void){
    uverify( 0 != m_cpu_elapse_id );
    const int tick_run_us = elapse_us_update( m_cpu_elapse_id );
    return tick_run_us;
}
/*
static bool qos_in_tick(void){
    const int pass_us = qos_tick_run_us();
    if( US_PER_TICK > pass_us ){
        //printf( "qos_in_tick(): pass_us=%d\n", pass_us );
        return true;
    }

    return false;
}
*/



// a tick:
// |  timer   idle   free |
static bool qos_in_idle(void){
    const int pass_us = qos_tick_run_us();
    if( US_PER_TICK <= pass_us ){
        return false;
    }

    const int free_us = US_PER_TICK - pass_us;
    // 为了运行idle，需要的最少空闲时间
    const int free_us_max = GCFG_QOS_TICK_FREE_MAX_US;
    //printf( "idle_us=%d\n", idle_us );
    if( free_us <= free_us_max ){
        return false;
    }

    return true;
}



// 缺省的qos_busy callback
// when this function called, this tick already exceed to next tick.
static void qos_busy_default( int busy_reason ){
    return;
}
void qos_busy_default_warnning( int busy_reason ){
    const int tick_run_us = qos_tick_run_us();
    uverify( tick_run_us > US_PER_TICK );

    if( QOS_BUSY_TOO_MUCH_TIMER == busy_reason ){
        const int tick_cnt = qos_tick_cnt();
        const int occupy_next_tick_us = tick_run_us - US_PER_TICK ;
        const int us_per_tick = US_PER_TICK;
        printf( "WARNNING: qos too much timer in tick_cnt %d: no time to run idle, \n"
                "tick hold %d us, occupy next tick %d us, setting %d us/tick\n"
                , tick_cnt
                , tick_run_us, occupy_next_tick_us, us_per_tick );
        uverify( false );
        return;
    }
    if( QOS_BUSY_LAST_IDLE_TOO_LONG == busy_reason ){
        const int tick_cnt = qos_tick_cnt();
        const int occupy_next_tick_us = tick_run_us - US_PER_TICK;
        const int us_per_tick = US_PER_TICK;
        const int task_id = qos_last_task_id();
        const char* const task_name_already_close = "task_already_close";
        const char* task_name = (0 == task_id) ?  task_name_already_close : qos_task_name( task_id );
        printf( "WARNING: qos last idle run too much time in tick_cnt %d, idle name: %s, task_id 0x%08x,\n"
                "tick hold %d us, occupy next tick %d us, setting %d us/tick\n"
                , tick_cnt, task_name, task_id
                , tick_run_us, occupy_next_tick_us, us_per_tick );
        uverify( false );
        return;
    }
    uverify( false );
    return;
}
static qos_busy_t m_qos_busy = qos_busy_default;
// busy         when cpu is busy, it will be called. If NULL, set as default.
// return: ok?
bool qos_set_busy( qos_busy_t busy ){
    if( NULL == busy ){
        busy = qos_busy_default;
    }

    m_qos_busy = busy;
    return true;
}




static void qos_tick_too_much_timer(void){
    m_qos_busy( QOS_BUSY_TOO_MUCH_TIMER );
}
static void qos_last_idle_too_long(void){
    m_qos_busy( QOS_BUSY_LAST_IDLE_TOO_LONG );
}


//}}}



//{{{ dead
// dead是个链表，将所有已经dead、等待释放内存的task，链接在一起。这样，遍历该链表，即可delete所有需要delete的task，
// 节省宝贵的timer_time开销。
// dead link串接的，就是task，无需qmalloc新的空间。
// 但同时带来问题：因为一个task是被两个链表共享，所以，在task被del之前，要将其从tick链表中remove，否则带来同步问题。
static qemdlink_node_t* dead_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    task_t* const p = (task_t*)ptr;
    return &(p->node_dead);
}
static void* dead_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, task_t, node_dead );
    return p;
}
// dead link, link all task need delete.
static int m_dead_link = 0;
static bool dead_init(void){
    m_dead_link = qemdlink_open( dead_ptr_to_node, dead_node_to_ptr );
    return_false_if( 0 == m_dead_link );
    return true;
}



static bool task_in_dead_link( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    return qemdlink_in( m_dead_link, task );
}
// return: ok?
static bool dead_open( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    uverify( 0 != m_dead_link );

    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    return_false_if( !qemdlink_append_last( m_dead_link, task ) );

    return true;
}
static bool dead_close( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    uverify( 0 != m_dead_link );

    uverify( task_in_dead_link( task_id ) );
    return_false_if( !qemdlink_remove( m_dead_link, task ) );
    return true;
}




static bool qos_task_delete( int task_id );
static bool dead_delete_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_dead_link == link_id );
    task_t* const task = ptr;
    uverify( NULL != task );
    const int task_id = (int)task;
    uverify( 0 <= ix );
    uverify( NULL == void_arg );

    uverify( task_in_task_link( task_id ) );
    uverify( task_in_dead_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );

    return_false_if( !qos_task_delete( task_id ) );
    return true;
}
static bool dead_delete(void){
    qemdlink_for_each( m_dead_link, dead_delete_callback, NULL );
    return true;
}



static bool dead_exit(void){
    uverify( 0 != m_dead_link );
    return_false_if( !dead_delete() );
    return true;
}
static bool dead_link_close(void){
    uverify( 0 != m_dead_link );
    uverify( qemdlink_empty( m_dead_link ) );
    return_false_if( !qemdlink_close( m_dead_link ) );
    m_dead_link = 0;
    return true;
}





//}}}



//{{{ task
static qemdlink_node_t* task_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    task_t* const p = (task_t*)ptr;
    return &(p->node);
}
static void* task_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, task_t, node );
    return p;
}
static int m_task_link = 0;
static bool task_init(void){
    m_task_link = qemdlink_open( task_ptr_to_node, task_node_to_ptr );
    return_false_if( 0 == m_task_link );
    return true;
}


static bool task_in_task_link( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    
    return qemdlink_in( m_task_link, task );
}




static bool task_id_valid( int task_id ){
    if( 0 == task_id ){
        return false;
    }
    return true;
}


/*
static int task_to_id( task_t* task ){
    uverify( NULL != task );
    const int task_id = (int)task;
    uverify( task_id_valid( task_id ) );
    return task_id;
}
*/




static bool task_event_notify( int task_id, int event ){
    uverify( task_id_valid( task_id ) );
    uverify( 0 != event );

    task_t* const task = (task_t*)task_id;
    if( 0 == task->notify_link ){
        return true;
    }

    return_false_if( !notify_task( task->notify_link, task_id, event ) );
    return true;
}




// return: task_id
static int task_open( int size, qos_entry_t entry, void* arg
        , task_delete_t delete, task_sleep_us_t sleep_us, task_tick_t tick, task_dead_t dead ){
    uverify( size >= sizeof( task_t ) );
    uverify( NULL != entry );
    uverify( NULL != delete );
    uverify( NULL != sleep_us );
    uverify( NULL != dead );

    task_t* const task = qmalloc( size );
    return_0_if( NULL == task );

    if( GCFG_DEBUG_EN ){
        memset( task, 0, size );
    }
    return_0_if( !qemdlink_node_init( &task->node ) );
    return_0_if( !qemdlink_node_init( &task->node_tick ) );
    return_0_if( !qemdlink_node_init( &task->node_dead ) );
    task->entry = entry;
    task->arg = arg;
    task->delete = delete;
    task->dead = dead;
    task->sleep_us = sleep_us;
    task->tick = tick;
    task->notify_link = 0;

    return_0_if( !qemdlink_append_last( m_task_link, task ) );
    const int task_id = (int)task;
    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    return task_id;
}
static bool task_notify_close( int task_id ){
    uverify( task_id_valid( task_id ) );

    task_t* const task = (task_t*)task_id;
    if( 0 == task->notify_link ){
        return true;
    }

    return_false_if( !notify_link_close( task->notify_link ) );
    task->notify_link = 0;
    return true;
}
// 原来是在qos_task_close()中，直接将任务从链表中去除，然后将相关内存free，
// 但运行中存在一些问题，主要有两个问题：
// 1    任务相关内存正在使用。例如：假设任务A正在运行，调用qos_task_close()将
// 自己close掉，然后相应的自己的内存全部free，但任务可能并没有退出，
// 任务相关代码还没有返回，因此，其可能调用一些系统函数，可能导致访问已经
// 释放的内存。
// 2    浪费运行时间。在timer_time释放内存，会占用宝贵的timer_time时间，
// 最好只做个标记，然后在idle_time进行释放。
// 所以，这里的做法为：qos_task_close()仅仅将任务做标记，要删除。实际做法是，
// 将其从tick link移除，这样，其不会被调用。将其放入dead link。待到空闲状态时，
// 统一真正的delete掉。
bool qos_task_close_real( int task_id ){
    //printf( "qos_task_close(): 0x%08x\n", task_id );
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );

    // 如果任务已经进入dead状态，就不能返回到其他状态。
    if( task_in_dead_link( task_id ) ){
        return true;
    }

    task_t* const task = (task_t*)task_id;
    const task_dead_t dead = task->dead;
    uverify( NULL != dead );
    // remove from tick link, so it will not be called again
    return_false_if( !dead( task_id ) );
    return_false_if( !task_event_notify( task_id, QOS_TASK_EVNET_CLOSE ) );
    return true;
}
static bool task_close( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;
    uverify( 0 != m_task_link );

    uverify( task_in_task_link( task_id ) );
    return_false_if( !qemdlink_remove( m_task_link, task ) );
    return true;
}

static bool qos_task_delete( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_dead_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( task_in_task_link( task_id ) );

    return_false_if( !task_event_notify( task_id, QOS_TASK_EVNET_DELETE ) );
    return_false_if( !task_notify_close( task_id ) );

    return_false_if( !dead_close( task_id ) );
    return_false_if( !task_close( task_id ) );

    task_t* const task = (task_t*)task_id;
    return_false_if( !(task->delete)( task_id ) );
    if( NULL != task->name ){
        qfreez( task->name );
    }

    if( GCFG_DEBUG_EN ){
        memset( task, 0, sizeof(task_t) );
    }
    qfree( task );
    return true;
}



static bool task_exit(void){
    uverify( 0 != m_task_link );
    return true;
}
static bool task_link_close(void){
    uverify( 0 != m_task_link );
    uverify( qemdlink_empty( m_task_link ) );

    return_false_if( !qemdlink_close( m_task_link ) );
    m_task_link = 0;
    return true;
}


// return: task run again?
static bool task_run( int task_id ){
    uverify( task_id_valid( task_id ) );

    task_t* const task = (task_t*)task_id;
    const qos_entry_t entry = task->entry;
    void* const arg = task->arg;

    uverify( 0 == m_tick.this_task_id );
    m_tick.this_task_id = task_id;
    //printf( "run task start: %s, 0x%08x, tick pass_us=%d\n", qos_task_name( task_id ), task_id, qos_tick_run_us() );
    if( !entry( arg ) ){
        // close task
        return_false_if( !qos_task_close( task_id ) );

        //printf( "close task: 0x%08x, tick pass_us=%d\n", task_id, qos_tick_run_us() );
        m_tick.last_task_id = 0;
        m_tick.this_task_id = 0;
        return false;
    }

#if 0
    if( !qos_in_tick() ){
        printf( "task_run: %s, 0x%08x qos_tick_run_us=%d\n", qos_task_name( task_id ), task_id, qos_tick_run_us() );
        uverify( false );
    }
#endif
    //printf( "run task end: %s, 0x%08x, tick pass_us=%d\n", qos_task_name( task_id ), task_id, qos_tick_run_us() );
    m_tick.last_task_id = task_id;
    m_tick.this_task_id = 0;

    return true;
}




// task_id         if 0, means current run task id
bool qos_task_set_entry( int task_id, qos_entry_t entry, void* arg ){
    if( 0 == task_id ){
        task_id = m_tick.this_task_id;
    }
    return_false_if( !task_id_valid( task_id ) );
    uverify( NULL != entry );
    
    task_t* const task = (task_t*)task_id;
    task->entry = entry;
    task->arg = arg;

    return_false_if( !task_event_notify( task_id, QOS_TASK_EVNET_SET_ENTRY ) );
    return true;
}





bool qos_task_set_name( int task_id, const char* task_name ){
    uverify( NULL != task_name );


    if( 0 == task_id ){
        task_id = m_tick.this_task_id;
    }
    return_false_if( !task_id_valid( task_id ) );

    task_t* const task = (task_t*)task_id;
    const int size = strlen( task_name ) + 1;
    char* const p = qmalloc( size );
    return_false_if( NULL == p );
    memcpy( p, task_name, size );
    // place after qmalloc(), if qmalloc() fail, task still have old name.
    if( NULL != task->name ){
        qfreez( task->name );
    }
    task->name = p;
    return true;
}
const char* qos_task_name( int task_id ){
    uverify( 0 != task_id );
    task_t* const task = (task_t*)task_id;

    if( NULL != task->name ){
        return task->name;
    }
    
    static const char* const unknow_name_task = "<task_no_name>";
    return unknow_name_task;
}



// for timer, it will delay timer us, then timer reload interval_us in open argument.
// for idle, it will delay idle us one time.
// task_id         if 0, means current run task id
// return: ok?
bool qos_task_sleep_us( int task_id, int us ){
    if( 0 == task_id ){
        task_id = m_tick.this_task_id;
    }
    uverify( task_id_valid( task_id ) );
    uverify( 0 <= us );

    uverify( task_in_task_link( task_id ) );
    if( task_in_dead_link( task_id ) ){
        return false;
    }

    task_t* const task = (task_t*)task_id;
    return_false_if( !(task->sleep_us)( task_id, us ) );
    return_false_if( !task_event_notify( task_id, QOS_TASK_EVNET_SLEEP_US ) );
    return true;
}




bool qos_task_set_notify( int task_id, qos_task_notify_t task_notify, void* arg ){
    if( 0 == task_id ){
        task_id = m_tick.this_task_id;
    }
    uverify( task_id_valid( task_id ) );
    uverify( NULL != task_notify );

    uverify( task_in_task_link( task_id ) );
    
    task_t* const task = (task_t*)task_id;
    if( 0 == task->notify_link ){
        task->notify_link = notify_link_open();
        return_false_if( 0 == task->notify_link );
    }
    uverify( 0 != task->notify_link );

    return_false_if( !notify_open( task->notify_link, task_notify, arg ) );
    return true;
}





//}}}



//{{{ timer
static qemdlink_node_t* timer_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    qtimer_t* const p = (qtimer_t*)ptr;
    return &(p->node);
}
static void* timer_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, qtimer_t, node );
    return p;
}
static int m_timer_link = 0;
static bool timer_init(void){
    m_timer_link = qemdlink_open( timer_ptr_to_node, timer_node_to_ptr );
    return_false_if( 0 == m_timer_link );
    return true;
}



static int timer_to_task_id( qtimer_t* timer ){
    uverify( NULL != timer );
    const int task_id = (int)(&timer->task);
    uverify( task_id_valid( task_id ) );
    return task_id;
}
static qtimer_t* task_id_to_timer( int task_id ){
    uverify( 0 != task_id );
    return (qtimer_t*)task_id;
}


static bool task_in_timer_link( int task_id ){
    uverify( 0 != task_id );
    qtimer_t* const timer = task_id_to_timer( task_id );
    return qemdlink_in( m_timer_link, timer );
}
static bool timer_open( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( !task_in_timer_link( task_id ) );
    qtimer_t* const timer = task_id_to_timer( task_id );
    return_false_if( !qemdlink_append_last( m_timer_link, timer ) );
    return true;
}
static bool timer_close( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_timer_link( task_id ) );
    qtimer_t* const timer = task_id_to_timer( task_id );
    return_false_if( !qemdlink_remove( m_timer_link, timer ) );
    return true;
}



static bool timer_move_from_timer_to_tick_link( int task_id ){
    uverify( 0 != task_id );
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( task_in_timer_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    
    return_false_if( !timer_close( task_id ) );
    return_false_if( !tick_open( task_id ) );
    return true;
}
static bool timer_move_from_tick_to_timer_link( int task_id ){
    uverify( 0 != task_id );
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_timer_link( task_id ) );
    uverify( task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !tick_close( task_id ) );
    return_false_if( !timer_open( task_id ) );
    return true;
}
static bool timer_move_from_timer_to_dead_link( int task_id ){
    uverify( 0 != task_id );
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( task_in_timer_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !timer_close( task_id ) );
    return_false_if( !dead_open( task_id ) );
    return true;
}
static bool timer_move_from_tick_to_dead_link( int task_id ){
    uverify( 0 != task_id );
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_timer_link( task_id ) );
    uverify( task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !timer_move_from_tick_to_timer_link( task_id ) );
    return_false_if( !timer_move_from_timer_to_dead_link( task_id ) );
    return true;
}



static bool timer_sleep_us( int task_id, int us ){
    uverify( task_id_valid( task_id ) );
    uverify( 0 <= us );

    qtimer_t* const timer = task_id_to_timer( task_id );
    timer->interval_us_ori = us;
    timer->interval_us = us;

    if( task_in_timer_link( task_id ) ){
        return_false_if( !timer_move_from_timer_to_tick_link( task_id ) );
    }

    return true;
}
// return: false, remove from tick link
static bool timer_task_tick( int task_id ){
    uverify( task_id_valid( task_id ) );
    qtimer_t* const timer = task_id_to_timer( task_id );

    if( US_PER_TICK < timer->interval_us ){
        timer->interval_us -= US_PER_TICK;
        return true;
    }
    
    // time reach
    timer->interval_us = timer->interval_us_ori;
    return_false_if( !timer_move_from_tick_to_timer_link( task_id ) );
    //printf( "run timer: 0x%08x, tick pass_us=%d\n", task_id, qos_tick_run_us() );
    //return_false_if( !task_run( task_id ) );
    // because it's timer, never return false, never detach from tick link
    return true;
}
static bool timer_dead( int task_id ){
    uverify( task_id_valid( task_id ) );
    if( task_in_timer_link( task_id ) ){
        return_false_if( !timer_move_from_timer_to_dead_link( task_id ) );
        return true;
    }
    if( task_in_tick_link( task_id ) ){
        return_false_if( !timer_move_from_tick_to_dead_link( task_id ) );
        return true;
    }
    uverify( false );
    return false;
}
static bool timer_task_delete_callback( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( !task_in_timer_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    uverify( !task_in_task_link( task_id ) );
    return true;
}
// interval_us      if interval_us <= US_PER_TICK, the task will be call every tick
// return: task id
int qos_timer_open( qos_entry_t entry, void* arg, int interval_us ){
    uverify( NULL != entry );
    uverify( 0 <= interval_us );
    uverify( 0 <= interval_us );
    //uverify( US_PER_TICK <= interval_us );

    // at least delay 1 tick
    if( US_PER_TICK > interval_us ){
        interval_us = US_PER_TICK;
    }

    const int task_id = task_open( sizeof(qtimer_t), entry, arg
            , timer_task_delete_callback, timer_sleep_us, timer_task_tick, timer_dead );
    return_0_if( 0 == task_id );

    qtimer_t* const timer = task_id_to_timer( task_id );
    timer->interval_us_ori = interval_us;
    timer->interval_us = interval_us;
    // at first timer place in tick link, so it will count down interval_us before be called.
    return_0_if( !tick_open( task_id ) );

    //return_0_if( !timer_link_append( timer, interval_us ) );
    uverify( !task_in_dead_link( task_id ) );

    return task_id;
}




static bool timer_task_dead_all(void){
    for(;;){
        qtimer_t* const timer = qemdlink_first( m_timer_link );
        if( NULL == timer ){
            return true;
        }
        const int task_id = timer_to_task_id( timer );
        return_false_if( !qos_task_close( task_id ) );
    }

    uverify( false );
    return false;
}
static bool timer_exit(void){
    uverify( 0 != m_timer_link );
    return_false_if( !timer_task_dead_all() );
    return true;
}
static bool timer_link_close(void){
    uverify( 0 != m_timer_link );
    uverify( qemdlink_empty( m_timer_link ) );
    return_false_if( !qemdlink_close( m_timer_link ) );
    m_timer_link = 0;
    return true;
}



// return: ok?
static bool timer_run_one( qtimer_t* timer ){
    uverify( NULL != timer );
    const int task_id = timer_to_task_id( timer );

    uverify( timer->interval_us_ori == timer->interval_us );
    if( task_run( task_id ) ){
        return_false_if( !timer_move_from_timer_to_tick_link( task_id ) );
    }

    return true;
}
static bool timer_link_run_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_timer_link == link_id );
    qtimer_t* const timer = (qtimer_t*)ptr;
    uverify( NULL != timer );
    uverify( 0 <= ix );
    uverify( NULL == void_arg );

    return_false_if( !timer_run_one( timer ) );
    if( !m_tick.tick_again ){
        return false;
    }
    return true;
}
// because not all idle run in every tick, so, idle must rember idle pointer next will run
// return: ok?
static bool timer_link_run(void){
    qemdlink_for_each( m_timer_link, timer_link_run_callback, NULL );
    return true;
}

/*
// return: ok?
static bool timer_tick_one( qtimer_t* timer ){
    uverify( NULL != timer );

    if( US_PER_TICK < timer->interval_us ){
        timer->interval_us -= US_PER_TICK;
        return true;
    }
    
    // time reach
    timer->interval_us = timer->interval_us_ori;
    const int task_id = timer_to_task_id( timer );
    return_false_if( !task_run( task_id ) );
    return true;
}
static bool timer_tick_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_timer_link == link_id );
    qtimer_t* const timer = (qtimer_t*)ptr;
    uverify( NULL != timer );
    uverify( 0 <= ix );
    uverify( NULL == void_arg );

    return_false_if( !timer_tick_one( timer ) );
    if( !m_tick.tick_again ){
        return false;
    }
    return true;
}
static bool timer_tick(void){
    qemdlink_for_each( m_timer_link, timer_tick_callback, NULL );
    return true;
}
*/



// }}}



//{{{ idle
static qemdlink_node_t* idle_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    idle_t* const p = (idle_t*)ptr;
    return &(p->node);
}
static void* idle_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, idle_t, node );
    return p;
}
// this idle always exist, so idle_link never empty. Default value pattern.
static bool default_idle_entry( void* arg ){
    if( !dead_delete() ){
        uverify( false );
    }
    return true;
}
static int m_idle_link = 0;
static idle_t* m_idle_next = NULL;
static bool idle_init(void){
    m_idle_link = qemdlink_open( idle_ptr_to_node, idle_node_to_ptr );
    return_false_if( 0 == m_idle_link );
    {
        const int task_id = qos_idle_open( default_idle_entry, NULL );
        return_false_if( 0 == task_id );
        return_false_if( !qos_task_set_name( task_id, "default_idle_entry" ) );
    }
    m_idle_next = qemdlink_first( m_idle_link );
    return true;
}





static int idle_to_task_id( idle_t* idle ){
    uverify( NULL != idle );
    const int task_id = (int)idle;
    uverify( task_id_valid( task_id ) );
    return task_id;
}
static idle_t* task_id_to_idle( int task_id ){
    uverify( task_id_valid( task_id ) );
    idle_t* const idle = (idle_t*)task_id;
    return idle;
}
static bool idle_in_idle_link( idle_t* idle ){
    uverify( NULL != idle );
    return qemdlink_in( m_idle_link, idle );
}
static bool task_in_idle_link( int task_id ){
    uverify( 0 != task_id );
    idle_t* const idle = task_id_to_idle( task_id );
    return idle_in_idle_link( idle );
}
// because default_idle_next exist, never return NULL
// return: next idle pointer
idle_t* const idle_link_loop_next( idle_t* idle ){
    uverify( NULL != idle );
    uverify( 0 != m_idle_link );

    uverify( idle_in_idle_link( idle ) );
    idle_t* next = qemdlink_next( m_idle_link, idle );
    if( NULL == next ){
        next = qemdlink_first( m_idle_link );
    }
    uverify( NULL != next );
    return next;
}



static bool idle_open( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( !task_in_idle_link( task_id ) );
    idle_t* const idle = task_id_to_idle( task_id );
    return_false_if( !qemdlink_append_last( m_idle_link, idle ) );
    return true;
}
static bool idle_close( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_idle_link( task_id ) );
    idle_t* const idle = task_id_to_idle( task_id );
    if( m_idle_next == idle ){
        m_idle_next = idle_link_loop_next( idle );
    }
    /*
    if( task_in_tick_link( task_id ) ){
        return_false_if( !tick_close( task_id ) );
    }
    */
    return_false_if( !qemdlink_remove( m_idle_link, idle ) );
    return true;
}



static bool idle_move_from_idle_to_tick_link( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( task_in_idle_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    
    return_false_if( !idle_close( task_id ) );
    return_false_if( !tick_open( task_id ) );
    return true;
}
static bool idle_move_from_tick_to_idle_link( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_idle_link( task_id ) );
    uverify( task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !tick_close( task_id ) );
    return_false_if( !idle_open( task_id ) );
    return true;
}
static bool idle_move_from_idle_to_dead_link( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( task_in_idle_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !idle_close( task_id ) );
    return_false_if( !dead_open( task_id ) );
    return true;
}
static bool idle_move_from_tick_to_dead_link( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( task_in_task_link( task_id ) );
    uverify( !task_in_idle_link( task_id ) );
    uverify( task_in_tick_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );

    return_false_if( !idle_move_from_tick_to_idle_link( task_id ) );
    return_false_if( !idle_move_from_idle_to_dead_link( task_id ) );

    //return_false_if( !tick_close( task_id ) );
    //return_false_if( !dead_open( task_id ) );
    return true;
}





static bool idle_sleep_us( int task_id, int us ){
    uverify( task_id_valid( task_id ) );
    uverify( 0 <= us );

    idle_t* const idle = task_id_to_idle( task_id );
    idle->delay_us = us;
    
    if( task_in_idle_link( task_id ) ){
        return_false_if( !idle_move_from_idle_to_tick_link( task_id ) );
    }
    //return_false_if( !tick_open( task_id ) );
    return true;
}
static bool idle_task_tick( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( !task_in_idle_link( task_id ) );
    uverify( task_in_tick_link( task_id ) );

    idle_t* const idle = task_id_to_idle( task_id );

    if( US_PER_TICK < idle->delay_us ){
        idle->delay_us -= US_PER_TICK;
        // tick again
        return true;
    }

    idle->delay_us = 0;
    // here is in tick, do not execute idle in tick
    // remove tick from tick link
    return_false_if( !idle_move_from_tick_to_idle_link( task_id ) );
    return false;
}
static bool idle_dead( int task_id ){
    uverify( 0 != task_id );
    if( task_in_idle_link( task_id ) ){
        return_false_if( !idle_move_from_idle_to_dead_link( task_id ) );
        return true;
    }

    if( task_in_tick_link( task_id ) ){
        return_false_if( !idle_move_from_tick_to_dead_link( task_id ) );
        return true;
    }
    uverify( false );
    return false;
}
static bool idle_task_delete_callback( int task_id ){
    uverify( task_id_valid( task_id ) );
    uverify( !task_in_idle_link( task_id ) );
    uverify( !task_in_tick_link( task_id ) );
    uverify( !task_in_task_link( task_id ) );
    uverify( !task_in_dead_link( task_id ) );
    return true;
}
static bool idle_link_append( idle_t* const idle ){
    uverify( NULL != idle );

    return_false_if( !qemdlink_node_init( &idle->node ) );
    idle->delay_us = 0;
    return_false_if( !qemdlink_append_last( m_idle_link, idle ) );
    return true;
}
// return: task id
int qos_idle_open( qos_entry_t entry, void* arg ){
    uverify( NULL != entry );

    const int task_id = task_open( sizeof(idle_t), entry, arg
            , idle_task_delete_callback, idle_sleep_us, idle_task_tick, idle_dead );
    return_0_if( 0 == task_id );

    idle_t* const idle = task_id_to_idle( task_id );
    return_0_if( !idle_link_append( idle ) );

    return task_id;
}




static bool idle_task_dead_all(void){
    for(;;){
        idle_t* const idle = qemdlink_first( m_idle_link );
        if( NULL == idle ){
            return true;
        }
        const int task_id = idle_to_task_id( idle );
        return_false_if( !qos_task_close( task_id ) );
    }

    uverify( false );
    return false;
}
static bool idle_exit(void){
    uverify( 0 != m_idle_link );
    return_false_if( !idle_task_dead_all() );
    return true;
}
static bool idle_link_close(void){
    uverify( 0 != m_idle_link );
    uverify( qemdlink_empty( m_idle_link ) );
    return_false_if( !qemdlink_close( m_idle_link ) );
    m_idle_link = 0;

    return true;
}





// return: ok?
static bool idle_run( idle_t* idle ){
    uverify( NULL != idle );

    // in idle delay?
    uverify( 0 == idle->delay_us );
    /*
    if( 0 < idle->delay_us ){
        return true;
    }
    */

    const int task_id = idle_to_task_id( idle );
    if( task_run( task_id ) ){
    }
    return true;
}
// return: ok?
static bool idle_one(void){
    uverify( 0 != m_idle_link );
    // because default_idle_entry always exist in idle link, 
    // so idle_next nerver is NULL
    uverify( NULL != m_idle_next );

    idle_t* const idle_will_run = m_idle_next;
    // because when exec idle_run(), the idle_next may be remove from the link, 
    // so qemdlink_next() cannot be place below idle_run()
    m_idle_next = idle_link_loop_next( m_idle_next );
    uverify( NULL != m_idle_next );

    return_false_if( !idle_run( idle_will_run ) );
    return true;
}
// because not all idle run in every tick, so, idle must rember idle pointer next will run
// return: ok?
static bool idle_till_next_tick_near(void){
    for(;;){
        if( !qos_in_idle() ){
            return true;
        }

        return_false_if( !idle_one() );
        //printf( "qos_tick_run_us()=%d\n", qos_tick_run_us() );

        if( !m_tick.tick_again ){
            return true;
        }
    }

    return true;
}




//}}}



//{{{ other

const char* qos_version(void){
    //static const char* const version = "qOS1.0_" # __DATE__ # __TIME__;
    static const char* const version = "qOS1.0";
    return version;
}


int qos_tick_status(void){
    return m_tick.tick_status;
}
int qos_tick_cnt(void){
    return m_tick.tick_cnt;
}
int qos_last_task_id(void){
    return m_tick.last_task_id;
}
// return: ok?
bool qos_last_tick_time( tick_time_t* tick_time ){
    uverify( NULL != tick_time );
    *tick_time = m_tick.last_tick_time;
    return true;
}


bool qos_init(void){
    return_false_if( !file_system_init() );

    return_false_if( !dead_init() );
    return_false_if( !tick_init() );
    return_false_if( !task_init() );
    return_false_if( !timer_init() );
    return_false_if( !idle_init() );

    return_false_if( !qmem_task_init() );

    unit_test_pump();
    ut_task_pump();

    return_false_if( !cpu_init() );
    return true;
    
}


static bool qos_at_exit(void){
    return_false_if( !tick_exit() );
    return_false_if( !idle_exit() );
    return_false_if( !timer_exit() );
    return_false_if( !dead_exit() );
    return_false_if( !task_exit() );
    return_false_if( !cpu_exit() );

    return_false_if( !tick_link_close() );
    return_false_if( !idle_link_close() );
    return_false_if( !timer_link_close() );
    return_false_if( !dead_link_close() );
    return_false_if( !task_link_close() );

    return_false_if( !file_system_exit() );
    return_false_if( !qmem_task_at_exit() );
    return true;
}



bool qos_exit(void){
    m_tick.tick_again = false;
    return true;
}



// return: ok?
static bool qos_idle(void){
    return_false_if( !idle_till_next_tick_near() );
    return true;
}
// return: tick again?
static bool tick_timer_status(void){
    m_tick.tick_status = QOS_TICK_STATE_TIMER;
    m_tick.last_tick_time = m_tick.this_tick_time;

    return_false_if( !tick_loop() );
    if( m_tick.tick_again ){
        return_false_if( !timer_link_run() );
    }

    const int pass_us = qos_tick_run_us();
    m_tick.this_tick_time.timer_us = pass_us;
    if( US_PER_TICK <= pass_us ){
        qos_tick_too_much_timer();
    }

    if( !m_tick.tick_again ){
        // exit qos
        return false;
    }

    return true;
}
// return: tick again?
static bool tick_idle_status(void){
    m_tick.tick_status = QOS_TICK_STATE_IDLE;
    return_false_if( !qos_idle() );

    const int pass_us = qos_tick_run_us();
    const int idle_us = pass_us - m_tick.this_tick_time.timer_us;
    uverify( 0 <= idle_us );
    m_tick.this_tick_time.idle_us = idle_us;
    if( US_PER_TICK <= pass_us ){
        qos_last_idle_too_long();
    }
    m_tick.this_tick_time.free_us = US_PER_TICK - pass_us;
    m_tick.tick_status = QOS_TICK_STATE_FREE;

    if( !m_tick.tick_again ){
        // exit qos
        return false;
    }

    return true;
}
// return: tick again?
bool qos_tick_real(void){
    return_false_if( !tick_start() );

    if( !tick_timer_status() ){
        return false;
    }

    if( !tick_idle_status() ){
        return false;
    }

    // tick again
    return true;
}
// return: tick again?
bool qos_tick(void){
    if( !qos_tick_real() ){
        qos_at_exit();
        return false;
    }

    ++m_tick.tick_cnt;
    return true;
}




//}}}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0

static bool task1( void* arg ){
    return true;
}



static bool test_timer(void){
    const int task_id = qos_timer_open( task1, NULL, 2*US_PER_TICK );
    test( qos_task_close( task_id ) );
    return true;
}


static bool test_idle(void){
    const int task_id = qos_idle_open( task1, NULL );
    test( qos_task_close( task_id ) );
    return true;
}


// unit test entry, add entry in file_list.h
bool unit_test_qos(void){
    const unit_test_item_t table[] = {
        test_timer
        , test_idle
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
