/*
 * File      : qstr.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-12     huanglin     first version
 *
 */


#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"

#include "inc.h"
#include "qmem.h"
#include "qstr.h"

#define this_file_id   file_id_qstr
//#define PRINTF_BUF_SIZE		1024

#define MIN(X,Y)        ( ( (X) < (Y) ) ? (X) : (Y) )


// 计算，如果用sprintf()来格式化字符串，需要多少字节，包括结尾'\0'
// 字符串打印出来后，字节个数，包括结尾处的'\0'
int sprintf_size( const char* fmt, ... ){
    uverify( NULL != fmt );

    va_list argptr;

    va_start( argptr, fmt );
    const int len = vsnprintf( NULL, 0, fmt, argptr );
    // snprintf()帮助上说：return 0, 表示错误，实际情况：snprintf( "%s", "" ), 就返回0
    va_end( argptr );

    uverify( 0 <= len );
    return (len+1);
}


int vsprintf_size( const char* fmt, va_list ap ){
    uverify( NULL != fmt );

    const int len = vsnprintf( NULL, 0, fmt, ap );
    uverify( 0 <= len );
    return (len+1);
}



/*
int u_printf( const char* fmt, ... ){
    va_list argptr;
    int len = 0;

    char buffer[PRINTF_BUF_SIZE];

    va_start( argptr, fmt );
    len = vsprintf( buffer, fmt, argptr );
    va_end(argptr);

    if( len >= sizeof(buffer) ){
        uverify( false );
        return 0;
    }

    log_str( "%s", buffer );
    log_display();
    
    return len;
}
*/




// 如果要打印一个字符串居中，前面补的空格数
// str          要打印的字符串
// line_len     a line char num that can print 
// ____xxxxx____
int str_middle_insert_blank( const char* str, int line_len ){
    uverify( NULL != str );
    uverify( 0 < line_len );

    const int len = strlen( str );
    if( len >= line_len ){
        return 0;
    }

    const int total_blank_len = line_len - len;
    const int half_blank_len = total_blank_len / 2;
    return half_blank_len;
}






static void printf_a_hex( char n ){
    char str[10];

	sprintf( str, "%02x ", n );
	printf( str );
}



static int printf_hex( const char* buf, int nbyte ){
    int i = 0;

	for( i=0; i<nbyte; i++ ){
		printf_a_hex( buf[i] );
	}

	return nbyte;
}



 

void printf_section_hex( const char* prompt, const char* section, int section_len ){
    printf( "%s", prompt );
	printf_hex( section, section_len );		
	printf( " (len=%u)", section_len );
    printf( "\n" );
}


 


void printf_section( const char* prompt, const char* section, int section_len ){
    int i = 0;

    printf( "%s", prompt );

    for( i=0; i<section_len; i++ ){
        printf( "%c", section[i] );
    }

	printf( "(len=%u)", section_len );
    printf( "\n" );
}





int strsize( const char* str ){
    uverify( NULL != str );
    return strlen( str ) + 1;
}






// dst中原为字符串，本函数尽量将src append在dst尾部，并判断append是否成功。
// 保证操作不会超过dst_size，而且，一定会在dst后添加'\0'
// return: 是否src完全append进dst中
bool append_str( char *dst, int dst_size, const char* src ){
    const int dst_len = strlen(dst);
    const int src_len = strlen(src);
    char* const p = dst + dst_len;            // 指向要拷贝的起始地址
    const int buff_len_for_copy = dst_size - dst_len;

    uverify( NULL != dst );
    uverify( NULL != src );
    uverify( 0 != dst_size );

    if( (dst_len+1) > dst_size ){
        // 如果dst字符串超过了缓冲区长度，认为系统发生错误，
        // 按照缓冲区长度值进行截断。
        dst[dst_size-1] = '\0';
        return false;
    }

	str_to_str( p, buff_len_for_copy, src );
    return (dst_len + src_len) == strlen(dst);
}




// 将一段内存，变成字符串，dst结尾处，一定是'\0'结尾
// return: n个字节，都拷贝到dst中
//          如果dst_size空间不够，返回false
bool mem_to_str( char* const str, int str_size, const void* mem, int n ){
    uverify( NULL != str );
    uverify( NULL != mem );

	// 如果小于1，无法容纳'\0'，函数返回时，str不是一个合法字符串
    if( 1 > str_size ){
        uverify( false );
        return false;
    }

    const int str_byte_num = str_size - 1;
    const int copy_byte_num = MIN( str_byte_num, n );

    int i = 0;
	const char* const src = (char*)mem; 
    for( i=0; i<copy_byte_num; i++ ){
        str[i] = src[i];
    }

    str[copy_byte_num] = '\0';
    if( copy_byte_num == n ){
        return true;
    }

    return false;
}






// 将字符串，拷贝到一段内容中，不拷贝'\0'
// return: 目标空间完全容纳str
bool str_to_mem( void* mem, int mem_size, const char* str ){
    uverify( NULL != mem );
    uverify( 0 <= mem_size );
    uverify( NULL != str );

    const int str_len = strlen( str );
    const int copy_byte_num = MIN( mem_size, str_len );

    int i = 0;
	char* const dst = (char*)mem;
    for( i=0; i<copy_byte_num; i++ ){
        dst[i] = str[i];
    }

    if( copy_byte_num < str_len ){
        return false;
    }

    return true;
}




// copy n char from src to dst, and add '\0'
// dst_size         目标缓冲区长度
// 如果想完全拷贝n个字节的字符串，要求dst_size >= n+1
// return: all src byte copy into dst
// 无论如何，dst string都会在末尾加'\0'，构建成完整的字符串；而且，写入dst的字节数，不超过dst_size
bool str_to_str( char* dst, int dst_size, const char* src ){
    uverify( NULL != dst );
    uverify( 1 <= dst_size );
    uverify( NULL != src );

    if( 1 > dst_size ){
        return false;
    }

    const int src_len = strlen( src );
    const int dst_len = dst_size - 1;
    const int copy_byte_num = MIN( src_len, dst_len );

    int i = 0;
    for( i=0; i<copy_byte_num; i++ ){
        dst[i] = src[i];
    }
    dst[copy_byte_num] = '\0';

    if( copy_byte_num == src_len ){
        return true;
    }

    return false;
}




// 尽量拷贝，如果目标空间不够，返回false
// return: dst_size >= src_size
bool mem_to_mem( void* dst, int dst_size, const void* src, int src_size ){
    uverify( NULL != dst );
    uverify( 0 <= dst_size );
    uverify( NULL != src );
    if( 0 > src_size ){
        var( src_size );
        return false;
    }
    uverify( 0 <= src_size );

    const int copy_byte_num = MIN( dst_size, src_size );
    memcpy( dst, src, copy_byte_num );

    if( copy_byte_num < src_size ){
        vark( src_size );
        vark( copy_byte_num );
        return false;
    }

    return true;
}



// 主要用于解析字符串
// return: src + byte_num
const void* memcpy_src_offset( void* dst, const void* src, int byte_num ){
    memcpy( dst, (void*)src, byte_num );
    return ((char*)src + byte_num);
}





// 主要用于拷贝分离的字符串到缓冲区时用
// 拷贝src到dst，拷贝byte_num字节，返回dst+byte_num指针
// return: dst+byte_num
void* memcpy_dst_offset( void* dst, const void* src, int byte_num ){
    memcpy( dst, src, byte_num );
    return ((char*)dst + byte_num);
}





// 将src字符串，最多拷贝dst_len个字节到dst中，如果dst多余字节，尾部填充fill字符。
// dst      目标字符串，并非以'\0'结尾
// src      源字符串
// fill     dst尾部填充的字节数
// return: dst_len是否足够容纳src所有字节，足够，返回true
// note:
// 1    如果dst_len，长度小于src字符串长度，只拷贝前dst_len字节。
// 2    dst & src can be same pointer
bool memcpy_fill( char* dst, int dst_len, const char* src, int src_len, char fill ){
    uverify( NULL != dst );
    uverify( NULL != src );

    const int copy_len = MIN( dst_len, src_len );

    memcpy( dst, src, copy_len );

    // 长度不够
    if( copy_len < src_len ){
        return false;
    }

    // 长度刚好，无需填充尾部
    //if( copy_len == src_len ){
    //    return true;
    //}

    uverify( copy_len >= src_len );
    uverify( copy_len - src_len <= dst_len );
    uverify( copy_len <= dst_len );
    const int fill_len = dst_len - copy_len;
    memset( dst + copy_len, fill, fill_len );

    return true;
}





void switch_2_byte( char byte[2] ){
    const char b0 = byte[0];
    byte[0] = byte[1];
    byte[1] = b0;
}




bool printable_char( char ch ){
    // todo...
    return true;
}
bool printable_mem( const char* mem, int len ){
    uverify( NULL != mem );
    uverify( 0 <= len );

    int i = 0;
    for( i=0; i<len; i++ ){
        if( !printable_char( mem[i] ) ){
            return false;
        }
    }

    return true;
}




// http://www.jxust3jia1.com/bbs/viewthread.php?tid=10619&sid=42LNCM
// 编程能力提升第1课：命令行解析
// 求助于MSDN，找到了命令行解析的规则，MSDN链接：http://msdn2.microsoft.com/en-us/library/aa243471.aspx。
/*
Microsoft C startup code uses the following rules when interpreting arguments given on the operating system command line:
Arguments are delimited by white space, which is either a space or a tab.
A string surrounded by double quotation marks is interpreted as a single argument, regardless of white space contained within. A quoted string can be embedded in an argument. Note that the caret (^) is not recognized as an escape character or delimiter. 
A double quotation mark preceded by a backslash, \", is interpreted as a literal double quotation mark (").
Backslashes are interpreted literally, unless they immediately precede a double quotation mark.
If an even number of backslashes is followed by a double quotation mark, then one backslash (\) is placed in the argv array for every pair of backslashes (\\), and the double quotation mark (") is interpreted as a string delimiter.
If an odd number of backslashes is followed by a double quotation mark, then one backslash (\) is placed in the argv array for every pair of backslashes (\\) and the double quotation mark is interpreted as an escape sequence by the remaining backslash, causing a literal double quotation mark (") to be placed in argv.
This list illustrates the rules above by showing the interpreted result passed to argv for several examples of command-line arguments. The output listed in the second, third, and fourth columns is from the ARGS.C program that follows the list.
Command-Line Input	argv[1]	argv[2]	argv[3]
"a b c" d e	        a b c	d	    e
"ab\"c" "\\" d	    ab"c	\	    d
a\\\b d"e f"g h	    a\\\b	de fg	h
a\\\"b c d	        a\"b	c	    d
a\\\\"b c" d e	    a\\b c	d	    e
*/
// usuage:
// const char* cmdlinestr = "a b c";
// int nArgs = 0;
// char** szArglist = SplitCommandLine( cmdlinestr, &nArgs );
// do something with szArglist and nArgs
// qfree( szArglist )

// command_line: a bb ccc
// char* p_a = command_line;
// int len = 0;
// char* p_b = next_command_line_arg( p_a, &len );
// uverify( len == 2 );
// char* p_c = next_command_line_arg( p_b, &len );
// uverify( len == 3 );
// uverify( NULL == next_command_line_arg( p_c, &len ) );
//
// 或者：
// char* p = command_line;
// int len = 0;
// while( NULL != ( p = next_command_line_arg( p, &len ) ) ){
//      do something with p, len
// }
/*
 下面这个函数写得并不好，对原有字符串做了修改，最好重写。
char **split_command_line( char *lpCmdLine, int *pNumArgs ){
    char *psrc = lpCmdLine;
    int nch = (int)strlen(lpCmdLine);
    int cbAlloc = (nch + 1) * sizeof(char*) + sizeof(char) * (nch + 1);
    char *pAlloc = (char*)qmalloc(cbAlloc / sizeof(char));
    
    char **argv = (char**) pAlloc;
    char *pdst = (char*)(((char*)pAlloc) + sizeof(char*)*(nch + 1));
    int inquote = 0;
    int copychar = 1;
    int numslash = 0;

    if (!pAlloc)
        return NULL;
    argv[(*pNumArgs)] = pdst;
    *pNumArgs = 0;
    while(1)
    {
        if(*psrc)
        {
            while(*psrc == ' ' || *psrc == '\t')
            {
                ++psrc;
            }
        }
        if(*psrc == '\0')
            break;
        argv[(*pNumArgs)++] = pdst;
        while(1)
        {
            copychar = 1;
            numslash = 0;
            while(*psrc == '\\')
            {
                ++psrc;
                ++numslash;
            }
            if(*psrc == '\"')
            {
                if(numslash % 2 == 0)
                {
                    if(inquote)
                    {
                        if(psrc[1] == '"')
                        {
                            psrc++;
                        }
                        else
                        {
                            copychar = 0;
                        }
                    }
                    else
                    {
                        copychar = 0;
                    }
                    inquote = !inquote;
                }
                numslash /= 2;
            }
            while(numslash--)
            {
                *pdst++ = '\\';
            }
    
            if(*psrc == '\0' || (!inquote && (*psrc == ' ' || *psrc == '\t')))
                break;
    
            if(copychar)
            {
                *pdst++ = *psrc;
            }
            ++psrc;
        }
        *pdst++ = '\0';
    }
    argv[(*pNumArgs)] = NULL;
    uverify((char*)pdst <= (char*)pAlloc + cbAlloc);
    return argv;
}
*/




#if GCFG_UNIT_TEST_EN > 0
/*
static bool test_safe_strcpy(void){
    const char src[] = "123456";
    const int src_len = strlen( src );
    char dst[10];
    const int dst_size = sizeof(dst);

    uverify( 6 == src_len );
    uverify( 10 == dst_size );

    // 如果dst有10个字节，应该src 6个字节都拷贝进dst
    memset( dst, 'x', dst_size );
    test( 7 == safe_strcpy( dst, 10, src, 6 ) );
    test( 0 == memcmp( dst, "123456\x00xxx", 10 ) );

    // 如果dst有3个字节，应该src 前2个字节拷贝进dst
    memset( dst, 'x', dst_size );
    test( 3 == safe_strcpy( dst, 3, src, 6 ) );
    test( 0 == memcmp( dst, "12\x00xxxxxxx", 10 ) );

    // 如果dst有6个字节，应该src 前5个字节拷贝进dst
    memset( dst, 'x', dst_size );
    test( 6 == safe_strcpy( dst, 6, src, 6 ) );
    test( 0 == memcmp( dst, "12345\x00xxxx", 10 ) );


    // 如果dst有10个字节，将src 前3个字节拷贝进dst
    memset( dst, 'x', dst_size );
    test( 4 == safe_strcpy( dst, 10, src, 3 ) );
    test( 0 == memcmp( dst, "123\x00xxxxxx", 10 ) );

    // 如果dst有7个字节，将src 前6个字节拷贝进dst
    memset( dst, 'x', dst_size );
    test( 1 == safe_strcpy( dst, 10, src, 0 ) );
    test( 0 == memcmp( dst, "\x00xxxxxxxxx", 10 ) );

    // 如果dst有10个字节，将超出src长度字节拷贝进dst
    memset( dst, 'x', dst_size );
    test( 7 == safe_strcpy( dst, 10, src, 8 ) );
    test( 0 == memcmp( dst, "123456\x00xxx", 10 ) );

    return true;
}
*/


static bool test_append_str(void){
    char dst[10];
    const int dst_size = sizeof(dst);
    
    // 正常append
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( append_str( dst, dst_size, "123" ) );
    test( 0 == memcmp( dst, "ab123\x00xxxx", 10 ) );

    // 边界条件，buff长度正好容纳
    memset( dst, 'x', dst_size );
	test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( append_str( dst, 6, "123" ) );
    test( 0 == memcmp( dst, "ab123\x00xxxx", 10 ) );

    // 边界条件， buff长度比完整拷贝少一个字节
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 5, "123" ) );
    test( 0 == memcmp( dst, "ab12\x00xxxxx", 10 ) );

    // 边界条件，buff长度比dst字符串长度小
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 1, "123" ) );
    // 'b' = 0x62
    test( 0 == memcmp( dst, "\x00\x62\x00xxxxxxx", 10 ) );

    // 边界条件，buff长度即dst字符串长度
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 2, "123" ) );
    test( 0 == memcmp( dst, "a\x00\x00xxxxxxx", 10 ) );

    // 边界条件，buff长度即dst字符串长度+1
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 3, "123" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );

    // 特殊条件，src字符串为空
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( append_str( dst, 5, "" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );

    memset( dst, 'x', dst_size );
    test( str_to_str( dst, 3, "" ) );
    test( 0 == memcmp( dst, "\x00xx", 3 ) );

    return true;
}






static bool test_memcpy_fill(void){
    // src长度， 小于dst情况
    {
        char dst[4];
        const char src[] = "ab";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abxx", 2 ) );
    }

    // src长度，等于dst情况
    {
        char dst[4];
        const char src[] = "abcd";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abcd", 4 ) );
    }

    // src长度，小于dst情况
    {
        char dst[4];
        const char src[] = "abcde";
        test( !memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abcd", 4 ) );
    }

    // 极限情况，dst长度为0
    {
        char dst[4];
        const char src[] = "abcde";
        test( !memcpy_fill( dst, 0, src, strlen( src ),  'x' ) );
    }

    // 极限情况，src长度为0
    {
        char dst[4];
        const char src[] = "";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "xxxx", 4 ) );
    }

    return true;
}



static bool test_sprintf_size(void){
    // snprintf()返回的长度，不包括结束处的'\0'
    test( 3 == snprintf( NULL, 0, "%s", "123" ) );

    test( 4 == sprintf_size( "%s", "123" ) );
    test( 8 == sprintf_size( "abc%s\n", "123" ) );
    test( 4 == sprintf_size( "%03x", 0x01 ) );
    test( 5 == sprintf_size( "%d", 1234 ) );

    // 极限情况
    test( 1 == sprintf_size( "" ) );
    test( 1 == sprintf_size( "%s", "" ) );

    // 一个很长的字符串
    char buff[5000];
    const int len = sizeof(buff);
    memset( buff, 'a', len );
    buff[len-1] = '\0';
    test( 5001 == sprintf_size( "%s\n", buff ) );

    return true;
}



static bool test_mem_to_str(void){
    char str[3];

    // normal
    memset( str, 0, 3 );
    test( mem_to_str( str, 3, "12", 2 ) );
    test( 0 == memcmp( str, "12\x00", 3 ) );

    // dst size is NOT enought
    memset( str, 0, 3 );
    test( !mem_to_str( str, 3, "123", 3 ) );
    test( 0 == memcmp( str, "12\x00", 3 ) );

    // dst size is 1
    memset( str, 0, 3 );
    test( !mem_to_str( str, 1, "123", 3 ) );
    test( 0 == memcmp( str, "\x00\x00\x00", 3 ) );

    // src n is 0
    memset( str, 0, 3 );
    test( mem_to_str( str, 3, "123", 0 ) );
    test( 0 == memcmp( str, "\x00\x00\x00", 3 ) );

    return true;
}




static bool test_str_to_mem(void){
    char mem[3];

    // normal
    memset( mem, 0, 3 );
    test( str_to_mem( mem, 3, "12" ) );
    test( 0 == memcmp( mem, "12\x00", 3 ) );

    // str len = 0
    memset( mem, 0, 3 );
    test( str_to_mem( mem, 3, "" ) );
    test( 0 == memcmp( mem, "\x00\x00\x00", 3 ) );

    // str len = mem size
    memset( mem, 0, 3 );
    test( str_to_mem( mem, 3, "123" ) );
    test( 0 == memcmp( mem, "123", 3 ) );

    // str len > mem size
    memset( mem, 0, 3 );
    test( !str_to_mem( mem, 3, "1234" ) );
    test( 0 == memcmp( mem, "123", 3 ) );

    // mem size = 0, str len = 0
    memset( mem, 0, 3 );
    test( str_to_mem( mem, 0, "" ) );
    test( 0 == memcmp( mem, "\x00\x00\x00", 3 ) );

    // mem size = 0, str len != 0
    memset( mem, 0, 3 );
    test( !str_to_mem( mem, 0, "12" ) );
    test( 0 == memcmp( mem, "\x00\x00\x00", 3 ) );

    return true;
}



static bool test_str_to_str(void){
    char dst[3];

    // normal
    memset( dst, 'x', 3 );
    test( str_to_str( dst, 3, "12" ) );
    test( 0 == memcmp( dst, "12\x00", 3 ) );

    // dst size NOT enought
    memset( dst, 'x', 3 );
    test( !str_to_str( dst, 3, "123" ) );
    test( 0 == memcmp( dst, "12\x00", 3 ) );

    // dst size is 1
    memset( dst, 'x', 3 );
    test( !str_to_str( dst, 1, "12" ) );
    test( 0 == memcmp( dst, "\x00xx", 3 ) );

    // src len is 0
    memset( dst, 'x', 3 );
    test( str_to_str( dst, 3, "" ) );
    test( 0 == memcmp( dst, "\x00xx", 3 ) );

    return true;
}



// unit test entry, add entry in file_list.h
bool unit_test_qstr(void){
    const unit_test_item_t table[] = {
  //      test_safe_strcpy
        test_append_str
        , test_memcpy_fill
        , test_sprintf_size
        , test_mem_to_str
        , test_str_to_mem
        , test_str_to_str
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif


// no more------------------------------
