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
 * ������landi EPT550�ϣ�__FILE__��ʱΪNULL,
 * ��ˣ��������ļ���������������
 * ��һ����ֵ���ַ�������Ӧ
 * �м����ô���
 * 1    �����˴���__FILE__�����ܻ���ٳ����С��
 * 2    ����������ʱ__FILE__ΪNULL�����磺landi EPT550��������
 * 3    ����ֵ�������ļ����������Ƚϵ�
 * ���������:
 * 1    Ҫ�ֹ�ά��һ�����
 *
 */


#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "inc.h"


#define this_file_id   file_id_file_id

typedef struct{
    const char* const file_name;       // ����Ųλ�ã�NULL��ʾ������
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




// note: û���ҵ��Ļ�������"not_found"����Զ���᷵��NULL
const char* file_id_to_str( int file_id ){
    int ix = 0;
    if( !file_id_in_file_list( file_id, &ix ) ){
        static const char* const not_found = "NOT_found_file_id";
        return not_found;
    }

    // ��Ϊ��������������uverify()��ʵ���У���ˣ�������uverify()
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

    // �����ڵ��ļ�
    int ix = 0;
    test( !file_id_in_file_list( 1000000, &ix ) );

    return true;
}




static bool test_file_id_to_str(void){
    test( 0 == strcmp( "file_id", file_id_to_str( file_id_file_id ) ) );
    test( 0 == strcmp( "NOT_found_file_id", file_id_to_str( 100000 ) ) );

    return true;
}





// ��Ԫ�����ļ������
// �� ../unit_test/unit_test.c �� unit_test_file_list[]�м���õ�Ԫ������ں�����
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
