/*
 * File      : file_id.c
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
 * 发现在landi EPT550上，__FILE__有时为NULL,
 * 因此，创建本文件，来解决这个问题
 * 用一个数值和字符串来对应
 * 有几个好处：
 * 1    减少了大量__FILE__，可能会减少程序大小。
 * 2    编译器，有时__FILE__为NULL。例如：landi EPT550编译器。
 * 3    用数值来代替文件，更好做比较等
 * 不方便的是:
 * 1    要手工维护一个表格
 *
 */


#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "inc.h"


#define this_file_id   file_id_file_id

typedef struct{
    const char* const file_name;       // 不能挪位置，NULL表示表格结束
    int file_id;
}file_t;




#define DEFINE_FUN( f )     { #f, file_id_##f }, 
static const file_t m_file_list[] = {
#include "file_list.h"
};
#undef DEFINE_FUN


bool file_id_in_file_list( int file_id, int* ix ){
    uverify( NULL != ix );

    *ix = 0;
    const file_t* p = NULL;
    for( p=m_file_list; (*p).file_name!=NULL; p++ ){
        if( file_id == p->file_id ){
            return true;
        }
        (*ix)++;
    }

    return false;
}




// note: 没有找到的话，返回"not_found"，永远不会返回NULL
const char* file_id_to_str( int file_id ){
    int ix = 0;
    if( !file_id_in_file_list( file_id, &ix ) ){
        static const char* const not_found = "NOT_found_file_id";
        return not_found;
    }

    // 因为本函数，用用在uverify()的实现中，因此，不能用uverify()
    //uverify( ix < sizeof(m_file_list)/sizeof(m_file_list[0]) );
    //uverify( NULL != m_file_list[ix].file_name );
    return m_file_list[ix].file_name;
}



#if GCFG_UNIT_TEST_EN > 0
static bool test_file_id_in_file_list(void){
    int ix0 = 0;
    test( file_id_in_file_list( file_id_file_id, &ix0 ) );
    int ix1 = 0;
    test( file_id_in_file_list( file_id_udebug, &ix1 ) );
    test( ix0 != ix1 );

    // 不存在的文件
    int ix = 0;
    test( !file_id_in_file_list( 1000000, &ix ) );

    return true;
}




static bool test_file_id_to_str(void){
    test( 0 == strcmp( "file_id", file_id_to_str( file_id_file_id ) ) );
    test( 0 == strcmp( "NOT_found_file_id", file_id_to_str( 100000 ) ) );

    return true;
}





// 单元测试文件级入口
// 在 ../unit_test/unit_test.c 中 unit_test_file_list[]中加入该单元测试入口函数。
extern bool unit_test_file_id(void);
bool unit_test_file_id(void){
    const unit_test_item_t table[] = {
        test_file_id_in_file_list
        , test_file_id_to_str
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif



// no more------------------------------
