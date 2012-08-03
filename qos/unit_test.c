/*
 * File      : unit_test.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */


/* 
* 
* 以文件为单位，进行测试
* 文件内的单元测试函数，用链表的形式串起来，所以，不需要维护
* 单元测试函数表，非常简洁。
* 加单元测试的步骤：
* 1 在主程序中，调用unit_test_pump()。这是单元测试的启动函数。
* 2 每个文件中，建立一个文件测试函数入口，例如unit_test_password_keyboard
*   其中调用unit_test_file_pump()，指明第一个调用的单元测试函数
* 3 在每个单元测试函数中，在函数的第一行调用unit_test_set_next_test()
*   来指明下一个测试函数。形成一个测试函数链表，避免维护单元测试函数表。
*   传入NULL，则指明本文件的单元测试停止。
* 4 当任何一个单元测试失败，整个测试停止。
* 
* 单元测试的例子，见本文件后面
*
* 文件历史:
* 
* modify history:
* 20110401  去掉了链表形式的相关函数。目的是保持最简化。
*
*
*/
 


#include <stdio.h>
#include "inc.h"


#define this_file_id   file_id_unit_test
 

#if GCFG_UNIT_TEST_EN > 0

typedef bool (*unit_test_file_t)(void);


#define DEFINE_FUN( f ) extern bool unit_test_##f(void);
#include "file_list.h"
#undef DEFINE_FUN


//static bool unit_test_1st(void){ return true; };
#define DEFINE_FUN( f )     unit_test_##f, 
static const unit_test_file_t m_file_list[] = {
#include "file_list.h"
    NULL
};
#undef DEFINE_FUN



#define wait_any_key()  
#define beep( beep_ms ) 
#define sleep_ms( inter_ms )

static int test_time_cnt = 0;

static bool m_ok = true;
void unit_test_fail(void){
    m_ok = false;
}
bool unit_test_is_ok(void){
    return m_ok;
}




//extern bool keyer_enable( bool enable );
//extern int key_read_once_base(void);
//extern bool key_dump(void);
//extern void wait_any_key(void);
#define KEY_NULL    0
void _test_fail( int file_id, int line, const char* express ){
    unit_test_fail();

    const char* file_name = file_id_to_str( file_id );
    if( NULL == express ){
        file_name = this_file_name();
        line = __LINE__;
    }

    static const char title[] = "test:";
    printf( "%s ", title );
    printf( "%d@%s ", line, file_name );
    printf( "%s\n", express );


    /*
    // 等待按键，pass掉key.c 和 keyer.c模块
    while( 1 ){
        const int key_code = key_read_once_base();
        EA_vDisplay( 4, "test key_code=%d", key_code );
        if( key_code != KEY_NULL ){
            return;
        }
        sleep_ms( 10 );
    }
    */
    
    wait_any_key();
    return;
}



static void end_bell(void){
    //const int beep_ms = 500;
    //const int inter_ms = 1000;

    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
}



static void unit_test_success_end(void){
    char message[100];
    snprintf( message, sizeof(message), "unit test ok: %d/%d", test_time_cnt+1, GCFG_UNIT_TEST_TIME );
    printf( "%s\n", message );                              

    end_bell();
}




static void unit_test_pump_once(void){
    unit_test_file_t const *p = NULL;
    for( p=m_file_list; *p!=NULL; p++ ){
        printf( "." );
        if( !(*p)() ){
            printf( "\n" );
            // test fail
            return;
        }
    }


    printf( "\n" );
    // all test success
    unit_test_success_end();
    return;
}




void unit_test_pump(void){
    for( test_time_cnt=0; test_time_cnt<GCFG_UNIT_TEST_TIME; test_time_cnt++ ){
        char message[100];
        snprintf( message, sizeof(message), "unit test: %d/%d", test_time_cnt+1, GCFG_UNIT_TEST_TIME );
        printf( "%s\n", message );                              
        unit_test_pump_once();
    }

    test_time_cnt = 0;
    return;
}



bool unit_test_file_pump_table( const unit_test_item_t table[] ){
    unit_test_item_t const *unit_test_item = table;

    while( NULL != *unit_test_item ){
        if( !((*unit_test_item)()) ){
            // test fail
            return false;
        }
        
        unit_test_item++;
    }

    return true;
}


#endif


// 单元测试例子及模板
#if GCFG_UNIT_TEST_EN > 0

// 单元测试项1
static bool test_item_1(void){
    return true;
}


// 单元测试文件级入口
// 在 ../unit_test/unit_test.c 中 unit_test_file_list[]中加入该单元测试入口函数。
bool unit_test_unit_test(void){
    const unit_test_item_t table[] = {
        test_item_1
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif



// no more------------------------------
