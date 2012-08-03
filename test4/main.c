/*
 * File      : main.c
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
#include <string.h>
#include "inc.h"


#define this_file_id   file_id_main




static bool idle( void* arg ){
    qos_exit();
    return false;
}



int main(void){
    qos_init();
    qos_idle_open( idle, NULL );
    qos_start();
    return 0;
}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_main(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}

