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
#include "serial.h"


#define this_file_id   file_id_main


/*

static int elapse_id = 0;
static serial_t serial = SERIAL_DEFAULT;
#define WRITE_MESSAGE		"123abc"

const char* const write_message = WRITE_MESSAGE;
const int data_len = sizeof(WRITE_MESSAGE) - 1;
char buf[sizeof(WRITE_MESSAGE)-1+10];

static bool test_write_read1( void* arg );
static bool test_write_read2( void* arg );
static bool test_write_read3( void* arg );
static bool test_write_read4( void* arg );
static bool test_write_read5( void* arg );

static bool test_write_read( void* arg ){
//	int timer_id = 0;

	uverify( sizeof(buf) > sizeof(WRITE_MESSAGE) );

	// 先测试时间特性

	// !!!ERROR!!! understand:
	// 发送完毕后，再接收，此时，串口电信号已经在
	// 线路上消失。这时，收到的字符数为0，
	// 说明：如果不调用read,linux并不会主动读取串口设备。
	// 也就是说：必须在发送方发送的同时，调用read函数进行接收。
	// correct understand:
	// linux OS will auto read serial port data and save data into system buffer, while you do NOT call read() in your code.
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
	return_false_if( !serial_open( &serial ) );
	return_false_if( !serial_write( &serial, (char*)WRITE_MESSAGE, strlen(WRITE_MESSAGE) ) );
	// wait os read
	// other wise, when you call read(), 
	// it will return -1 and set errno = EAGAIN
	// it mean system buffer havn't data
    return_false_if( !qos_task_sleep_us( MS_TO_US( SEC_TO_MS( 1 ) ) ) );
    return_false_if( !qos_idle_set_entry( 0, test_write_read1, NULL ) );
    return true;
}


static bool test_write_read1( void* arg ){
	{
		int n_read = serial_read( &serial, buf, sizeof(buf) );
		return_false_if( data_len != n_read );
	}
	return_false_if( !serial_close( &serial ) );
	return_false_if( MS_TO_US( 2000 ) < elapse_us_close( elapse_id ) );

	// send zero char
	memset( buf, 'x', sizeof(buf) );
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
	return_false_if( !serial_open( &serial ) );
	{
		const char message[] = "";
		return_false_if( !serial_write( &serial, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	qos_idle_sleep_us( MS_TO_US( SEC_TO_MS( 1 ) ) );
    qos_idle_set_entry( 0, test_write_read2, NULL );
    return true;
}

static bool test_write_read2( void* arg ){
	return_false_if( 0 != serial_read( &serial, buf, sizeof(buf) ) );
	return_false_if( !serial_close( &serial ) );
	return_false_if( buf[0] != 'x' );
	return_false_if( MS_TO_US( 2000 ) < elapse_us_close( elapse_id ) );

	// send one char
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
	return_false_if( !serial_open( &serial ) );
	{
		const char message[] = "x";
		return_false_if( !serial_write( &serial, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	qos_idle_sleep_us( MS_TO_US( SEC_TO_MS( 1 ) ) );
    qos_idle_set_entry( 0, test_write_read3, NULL );
    return true;
}

static bool test_write_read3( void* arg ){
	{
		const int nread = serial_read( &serial, buf, sizeof(buf) );
		return_false_if( 1 != nread );
	}
	
	return_false_if( !serial_close( &serial ) );
	return_false_if( buf[0] != 'x' );
	return_false_if( MS_TO_US( 2000 ) < elapse_us_close( elapse_id ) );


	// send a string
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
    return_false_if( !serial_open( &serial ) );
	{
		const char message[] = "123abc";
		return_false_if( !serial_write( &serial, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	qos_idle_sleep_us( MS_TO_US( SEC_TO_MS( 1 ) ) );
    qos_idle_set_entry( 0, test_write_read4, NULL );
    return true;
}

static bool test_write_read4( void* arg ){
	{
		memset( buf, 0, sizeof(buf) );
		int nread = serial_read( &serial, buf, sizeof(buf) );
		return_false_if( nread != 6 );
	}
	return_false_if( !serial_close( &serial ) );
	return_false_if( 0 != strcmp( (char*)buf, "123abc" ) );
	return_false_if( MS_TO_US( 2000 ) < elapse_us_close( elapse_id ) );

	// 测试buf长度不够情况
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
    return_false_if( !serial_open( &serial ) );
	{
		const char message[] = "123abc";
		return_false_if( !serial_write( &serial, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	qos_idle_sleep_us( MS_TO_US( SEC_TO_MS( 1 ) ) );
    qos_idle_set_entry( 0, test_write_read5, NULL );
    return true;
}

static bool test_write_read5( void* arg ){
	memset( buf, 0xab, sizeof(buf) );
	return_false_if( 2 != serial_read( &serial, buf, 2 ) );
	return_false_if( 0 != memcmp( "12\xab\xab\xab\xab\xab\xab", buf, 8 ) );
	// if read again, it will retrive remain data
	memset( buf, 0xab, sizeof(buf) );
	return_false_if( 3 != serial_read( &serial, buf, 3 ) );
	return_false_if( 0 != memcmp( "3ab\xab\xab\xab\xab\xab", buf, 8 ) );
	// if read again, it will retrive remain data
	memset( buf, 0xab, sizeof(buf) );
	return_false_if( 1 != serial_read( &serial, buf, 3 ) );
	return_false_if( 0 != memcmp( "c\xab\xab\xab\xab\xab\xab\xab", buf, 8 ) );
	return_false_if( !serial_close( &serial ) );
	return_false_if( MS_TO_US( 3000 ) < elapse_us_close( elapse_id ) );
    qos_exit();

	return false;
}




static bool test_open_close( void* arg ){
    serial_t serial = SERIAL_DEFAULT;
    return_false_if( !serial_open( &serial ) );
    return_false_if( !serial_close( &serial ) );

    return_false_if( !qos_idle_set_entry( 0, test_write_read, NULL ) );
    return true;
}

*/


int main(void){
    qos_init();

    //qos_idle_open( test_open_close, NULL );
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

