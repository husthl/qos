/*
 * File      : udebug.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */

#include <stdlib.h>
#include "malloc.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"

#include "inc.h"

// 本目录

#define this_file_id    file_id_udebug
static const char* const m_file_name = "udebug";

#define log_str( fmt, ... )                 
#define wait_any_key()


#define SLOW_SEC        1

static void show_file_line_message( const char* file_name, int line, const char* message, const char* title, const char* tip ){
    if( NULL == file_name ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == title ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == message ){
        file_name = m_file_name;
        line = __LINE__;
    }

    printf( "%s ", title );
    printf( "%d@%s ", line, file_name );
    printf( "%s ", message );
    printf( "%s\n", tip );
}




bool uverify_real( int file_id, int line, const char* message ){
    const char* file_name = file_id_to_str( file_id );
    if( NULL == message ){
        file_name = m_file_name;
        line = __LINE__;
    }

    static const char title[] = "uverify:";
    show_file_line_message( file_name, line, message, title, "" );
    //sleep_ms( 100 );
    //sleep_sec( SLOW_SEC );

    log_str( "uverify:%d@%s"END_LINE, line, file_name );
    //sleep_sec( SLOW_SEC );

    wait_any_key();
    return true;
}




void here_real( int file_id, int line, const char* message, bool wait_key ){
    char const *file_name = file_id_to_str( file_id );
    if( NULL == file_name ){
        file_name = m_file_name;
        line = __LINE__;
    }

    if( NULL == message ){
        file_name = m_file_name;
        line = __LINE__;
    }

    printf( "here_%s:%d@%s\n", message, line, file_name );
    log_str( "here_%s:%d@%s"END_LINE, message, line, file_name );

    if( wait_key ){
        wait_any_key();
    }
    else{
        //sleep_sec( SLOW_SEC );
    }
    return;
}





void var_real( int file_id, int line, const char* var_name, int n, bool wait_key ){
    char const *file_name = file_id_to_str( file_id );
    if( NULL == file_name ){
        file_name = m_file_name;
        line = __LINE__;
    }

    if( NULL == var_name ){
        file_name = m_file_name;
        line = __LINE__;
    }

    printf( "var:%d@%s\n", line, file_name );
    // 数值放在前面，是因为有些名字很长
    printf( "%d=%s\n", n, var_name  );

    if( wait_key ){
        wait_any_key();
    }
    else{
        //sleep_sec( SLOW_SEC );
    }
    return;
}



void dstr_real( int file_id, int line, const char* str_name, const char* str, bool wait_key ){
    const char* s = "<NULL>";
    if( NULL == str ){
        str = s;
    }

    const char* file_name = file_id_to_str( file_id );
    if( NULL == file_name ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == str_name ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == str ){
        file_name = m_file_name;
        line = __LINE__;
    }

	const int len = strlen( str );

    printf( "str:%d:%s\n", len, str_name );
    // 文件位置
    printf( "%d@%s\n", line, file_name  );

    // 字符串格式
    {
        // str可能不是'\0'结束，所以转存到堆栈内存上
        const int alloca_len = len+1;
        char * const p = alloca( alloca_len );
        int i = 0;
        for( i=0; i<len; i++ ){
            p[i] = str[i];
        }
        p[len] = '\0';
        printf( "%s\n", p );
    }

    // 16进制格式
    {
        const int alloca_len = len*2+10;
        char * const p = alloca( alloca_len );
        int i = 0;
        for( i=0; i<len; i++ ){
            snprintf( p+2*i, 3, "%02x", (unsigned char)(str[i]) );
        }
        p[2*len] = '\0';
        printf( "%s\n", p );
    }


    if( wait_key ){
        wait_any_key();
    }
    else{
        //sleep_sec( SLOW_SEC );
    }

    return;
}





void dmem_real( int file_id, int line, const char* mem_name, const char* mem, int len, bool wait_key ){
    const char* file_name = file_id_to_str( file_id );
    if( NULL == file_name ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == mem_name ){
        file_name = m_file_name;
        line = __LINE__;
    }
    if( NULL == mem ){
        file_name = m_file_name;
        line = __LINE__;
    }

    printf( "mem:%d:%s\n", len, mem_name );
    // 文件位置
    printf( "%d@%s\n", line, file_name  );

    // 字符串格式
    {
        // str可能不是'\0'结束，所以转存到堆栈内存上
        const int alloca_len = len+1;
        char* const p = alloca( alloca_len );
        int i = 0;
        for( i=0; i<len; i++ ){
            p[i] = mem[i];
        }
        p[len] = '\0';
        printf( "%s\n", p );
    }

    // 16进制格式
    {
        const int alloca_len = len*2+10;
        char* const p = alloca( alloca_len );
        int i = 0;
        for( i=0; i<len; i++ ){
            snprintf( p+2*i, 3, "%02x", (unsigned char)(mem[i]) );
        }
        p[2*len] = '\0';
        printf( "%s\n", p );
    }


    if( wait_key ){
        wait_any_key();
    }
    else{
        //sleep_sec( SLOW_SEC );
    }

    return;
}






#if GCFG_UNIT_TEST_EN > 0

/*
static bool fun2_for_test_uverify(void){
    uverify( false );
    return false;
}

static bool fun1_for_test_uverify(void){
    return_false_if( !fun2_for_test_uverify() );
    return true;
}

static bool fun0_for_test_uverify(void){
    return_false_if( !fun1_for_test_uverify() );
    return true;
}


// uverify()按一次键，就应该进入上一级调用的地方
static bool test_uverify(void){
    test( fun0_for_test_uverify() );

    return true;
}
*/


static bool test_nothing(void){
    return true;
}



// 单元测试文件级入口
// 在 ../unit_test/unit_test.c 中 unit_test_file_list[]中加入该单元测试入口函数。
extern bool unit_test_udebug(void);
bool unit_test_udebug(void){
    const unit_test_item_t table[] = {
        test_nothing
        //, test_uverify
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif


// no more------------------------------
