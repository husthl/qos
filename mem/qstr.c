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


// ���㣬�����sprintf()����ʽ���ַ�������Ҫ�����ֽڣ�������β'\0'
// �ַ�����ӡ�������ֽڸ�����������β����'\0'
int sprintf_size( const char* fmt, ... ){
    uverify( NULL != fmt );

    va_list argptr;

    va_start( argptr, fmt );
    const int len = vsnprintf( NULL, 0, fmt, argptr );
    // snprintf()������˵��return 0, ��ʾ����ʵ�������snprintf( "%s", "" ), �ͷ���0
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




// ���Ҫ��ӡһ���ַ������У�ǰ�油�Ŀո���
// str          Ҫ��ӡ���ַ���
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






// dst��ԭΪ�ַ�����������������src append��dstβ�������ж�append�Ƿ�ɹ���
// ��֤�������ᳬ��dst_size�����ң�һ������dst�����'\0'
// return: �Ƿ�src��ȫappend��dst��
bool append_str( char *dst, int dst_size, const char* src ){
    const int dst_len = strlen(dst);
    const int src_len = strlen(src);
    char* const p = dst + dst_len;            // ָ��Ҫ��������ʼ��ַ
    const int buff_len_for_copy = dst_size - dst_len;

    uverify( NULL != dst );
    uverify( NULL != src );
    uverify( 0 != dst_size );

    if( (dst_len+1) > dst_size ){
        // ���dst�ַ��������˻��������ȣ���Ϊϵͳ��������
        // ���ջ���������ֵ���нضϡ�
        dst[dst_size-1] = '\0';
        return false;
    }

	str_to_str( p, buff_len_for_copy, src );
    return (dst_len + src_len) == strlen(dst);
}




// ��һ���ڴ棬����ַ�����dst��β����һ����'\0'��β
// return: n���ֽڣ���������dst��
//          ���dst_size�ռ䲻��������false
bool mem_to_str( char* const str, int str_size, const void* mem, int n ){
    uverify( NULL != str );
    uverify( NULL != mem );

	// ���С��1���޷�����'\0'����������ʱ��str����һ���Ϸ��ַ���
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






// ���ַ�����������һ�������У�������'\0'
// return: Ŀ��ռ���ȫ����str
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
// dst_size         Ŀ�껺��������
// �������ȫ����n���ֽڵ��ַ�����Ҫ��dst_size >= n+1
// return: all src byte copy into dst
// ������Σ�dst string������ĩβ��'\0'���������������ַ��������ң�д��dst���ֽ�����������dst_size
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




// �������������Ŀ��ռ䲻��������false
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



// ��Ҫ���ڽ����ַ���
// return: src + byte_num
const void* memcpy_src_offset( void* dst, const void* src, int byte_num ){
    memcpy( dst, (void*)src, byte_num );
    return ((char*)src + byte_num);
}





// ��Ҫ���ڿ���������ַ�����������ʱ��
// ����src��dst������byte_num�ֽڣ�����dst+byte_numָ��
// return: dst+byte_num
void* memcpy_dst_offset( void* dst, const void* src, int byte_num ){
    memcpy( dst, src, byte_num );
    return ((char*)dst + byte_num);
}





// ��src�ַ�������࿽��dst_len���ֽڵ�dst�У����dst�����ֽڣ�β�����fill�ַ���
// dst      Ŀ���ַ�����������'\0'��β
// src      Դ�ַ���
// fill     dstβ�������ֽ���
// return: dst_len�Ƿ��㹻����src�����ֽڣ��㹻������true
// note:
// 1    ���dst_len������С��src�ַ������ȣ�ֻ����ǰdst_len�ֽڡ�
// 2    dst & src can be same pointer
bool memcpy_fill( char* dst, int dst_len, const char* src, int src_len, char fill ){
    uverify( NULL != dst );
    uverify( NULL != src );

    const int copy_len = MIN( dst_len, src_len );

    memcpy( dst, src, copy_len );

    // ���Ȳ���
    if( copy_len < src_len ){
        return false;
    }

    // ���ȸպã��������β��
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
// �������������1�Σ������н���
// ������MSDN���ҵ��������н����Ĺ���MSDN���ӣ�http://msdn2.microsoft.com/en-us/library/aa243471.aspx��
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
// ���ߣ�
// char* p = command_line;
// int len = 0;
// while( NULL != ( p = next_command_line_arg( p, &len ) ) ){
//      do something with p, len
// }
/*
 �����������д�ò����ã���ԭ���ַ��������޸ģ������д��
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

    // ���dst��10���ֽڣ�Ӧ��src 6���ֽڶ�������dst
    memset( dst, 'x', dst_size );
    test( 7 == safe_strcpy( dst, 10, src, 6 ) );
    test( 0 == memcmp( dst, "123456\x00xxx", 10 ) );

    // ���dst��3���ֽڣ�Ӧ��src ǰ2���ֽڿ�����dst
    memset( dst, 'x', dst_size );
    test( 3 == safe_strcpy( dst, 3, src, 6 ) );
    test( 0 == memcmp( dst, "12\x00xxxxxxx", 10 ) );

    // ���dst��6���ֽڣ�Ӧ��src ǰ5���ֽڿ�����dst
    memset( dst, 'x', dst_size );
    test( 6 == safe_strcpy( dst, 6, src, 6 ) );
    test( 0 == memcmp( dst, "12345\x00xxxx", 10 ) );


    // ���dst��10���ֽڣ���src ǰ3���ֽڿ�����dst
    memset( dst, 'x', dst_size );
    test( 4 == safe_strcpy( dst, 10, src, 3 ) );
    test( 0 == memcmp( dst, "123\x00xxxxxx", 10 ) );

    // ���dst��7���ֽڣ���src ǰ6���ֽڿ�����dst
    memset( dst, 'x', dst_size );
    test( 1 == safe_strcpy( dst, 10, src, 0 ) );
    test( 0 == memcmp( dst, "\x00xxxxxxxxx", 10 ) );

    // ���dst��10���ֽڣ�������src�����ֽڿ�����dst
    memset( dst, 'x', dst_size );
    test( 7 == safe_strcpy( dst, 10, src, 8 ) );
    test( 0 == memcmp( dst, "123456\x00xxx", 10 ) );

    return true;
}
*/


static bool test_append_str(void){
    char dst[10];
    const int dst_size = sizeof(dst);
    
    // ����append
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( append_str( dst, dst_size, "123" ) );
    test( 0 == memcmp( dst, "ab123\x00xxxx", 10 ) );

    // �߽�������buff������������
    memset( dst, 'x', dst_size );
	test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( append_str( dst, 6, "123" ) );
    test( 0 == memcmp( dst, "ab123\x00xxxx", 10 ) );

    // �߽������� buff���ȱ�����������һ���ֽ�
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 5, "123" ) );
    test( 0 == memcmp( dst, "ab12\x00xxxxx", 10 ) );

    // �߽�������buff���ȱ�dst�ַ�������С
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 1, "123" ) );
    // 'b' = 0x62
    test( 0 == memcmp( dst, "\x00\x62\x00xxxxxxx", 10 ) );

    // �߽�������buff���ȼ�dst�ַ�������
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 2, "123" ) );
    test( 0 == memcmp( dst, "a\x00\x00xxxxxxx", 10 ) );

    // �߽�������buff���ȼ�dst�ַ�������+1
    memset( dst, 'x', dst_size );
    test( str_to_str( dst, dst_size, "ab" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );
    test( !append_str( dst, 3, "123" ) );
    test( 0 == memcmp( dst, "ab\x00xxxxxxx", 10 ) );

    // ����������src�ַ���Ϊ��
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
    // src���ȣ� С��dst���
    {
        char dst[4];
        const char src[] = "ab";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abxx", 2 ) );
    }

    // src���ȣ�����dst���
    {
        char dst[4];
        const char src[] = "abcd";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abcd", 4 ) );
    }

    // src���ȣ�С��dst���
    {
        char dst[4];
        const char src[] = "abcde";
        test( !memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "abcd", 4 ) );
    }

    // ���������dst����Ϊ0
    {
        char dst[4];
        const char src[] = "abcde";
        test( !memcpy_fill( dst, 0, src, strlen( src ),  'x' ) );
    }

    // ���������src����Ϊ0
    {
        char dst[4];
        const char src[] = "";
        test( memcpy_fill( dst, 4, src, strlen( src ), 'x' ) );
        test( 0 == memcmp( dst, "xxxx", 4 ) );
    }

    return true;
}



static bool test_sprintf_size(void){
    // snprintf()���صĳ��ȣ���������������'\0'
    test( 3 == snprintf( NULL, 0, "%s", "123" ) );

    test( 4 == sprintf_size( "%s", "123" ) );
    test( 8 == sprintf_size( "abc%s\n", "123" ) );
    test( 4 == sprintf_size( "%03x", 0x01 ) );
    test( 5 == sprintf_size( "%d", 1234 ) );

    // �������
    test( 1 == sprintf_size( "" ) );
    test( 1 == sprintf_size( "%s", "" ) );

    // һ���ܳ����ַ���
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
