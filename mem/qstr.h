
#include "stdarg.h"

// �ַ���_�ڴ濽�����
bool str_to_str( char* dst, int dst_size, const char* src );
bool mem_to_mem( void* dst, int dst_size, const void* src, int src_size );

bool mem_to_str( char* str, int str_size, const void* mem, int n );
bool str_to_mem( void* mem, int mem_size, const char* str );

bool append_str( char* dst, int dst_size, const char* src );
int strsize( const char* str );

// �ڴ洦��
void const *memcpy_src_offset( void* dst, const void* src, int byte_num );
void *memcpy_dst_offset( void* dst, const void* src, int byte_num );
bool memcpy_fill( char* dst, int dst_size, const char* src, int src_len, char fill );
void switch_2_byte( char byte[2] );



// �ܶ�����£���Ҫ��һ��û��0��β���ַ����飬��ʱת��Ϊ0��β���ַ�����������ʾ�ȡ� 
#define mem_to_str_on_mem( mem_alloc_fun, src, src_len )             \
	(char*)({											\
    uverify( NULL != src );                             \
    uverify( 0 <= src_len );                            \
                                                        \
    const int _dst_size = src_len + 1;                  \
    char* _dst = mem_alloc_fun( _dst_size );            \
    uverify( NULL != _dst );                            \
                                                        \
	memcpy( _dst, _src, src_len );                      \
    _dst[src_len] = '\0';                               \
                                                        \
    _dst;                                               \
	})

#define str_dup_on_mem( mem_alloc_fun, src )            \
	(char*)({											\
    uverify( NULL != src );                             \
                                                        \
    const int _dst_size = strlen( src ) + 1;            \
    char* _dst = mem_alloc_fun( _dst_size );            \
    uverify( NULL != _dst );                            \
                                                        \
	memcpy( _dst, src, _dst_size );                     \
                                                        \
    _dst;                                               \
	})

#define mem_dup_on_mem( mem_alloc_fun, src, len )       \
	(char*)({											\
    uverify( NULL != src );                             \
    uverify( 0 <= len );                                \
                                                        \
    char* _dst = mem_alloc_fun( len );                  \
    uverify( NULL != _dst );                            \
                                                        \
	memcpy( _dst, src, len );                           \
                                                        \
    _dst;                                               \
	})

void printf_section( const char* prompt, const char* section, int section_len );
void printf_section_hex( const char* prompt, const char* section, int section_len );
int sprintf_size( const char* const fmt, ... );
int vsprintf_size( const char* fmt, va_list ap );

// һ��snprintf()��ȫ�汾
// 1.	�򻯹������᷵��һ���ڴ�ָ�룬��Ÿ�ʽ����Ľ����
// 2.   ����ָ���ڴ��ڶ�/��ջ�ϡ�
// 3.	���������ڶ�ջ�ϡ�������Ϊ�ڴ���䣬Ҳ�����ͷ��ڴ档
// 4.	��snprintf()������м�顣������ԣ����ص�NULL
// 5.	Ϊ�˾������ٶ����ԣ���sprintf()���롢�����Ĳ��������御����ͬ
//
// �������壺
// return		��ʽ������ַ����׵�ַ
// 
// usuage: 
//		char const * const p = sprintf_on_mem( alloca, "%s%d", "abc", 123 );
//
// read: g_new()�궨��д����
#define sprintf_on_mem( mem_alloc_fun, fmt, ... )		                    \
	(char*)({											                    \
    const int _t_size = sprintf_size( fmt, __VA_ARGS__ );                   \
    uverify( _t_size > 0 );                                                 \
    char *_t_p = mem_alloc_fun( _t_size );                                  \
    uverify( NULL != _t_p );                                                \
                                                                            \
	const int _t_result_len = snprintf( _t_p, _t_size, fmt, __VA_ARGS__ );  \
    if( _t_result_len+1 != _t_size ){                                       \
        uverify( false );                                                   \
    }                                                                       \
    uverify( _t_p[_t_size-1] == '\0' );                                     \
                                                                            \
    _t_p;                                                                   \
	})

#define sprintf_on_stack( fmt, ... )    sprintf_on_mem( alloca, fmt, __VA_ARGS__ )




// ��ʾ���
//#define printf          u_printf
//int u_printf( const char* fmt, ... );
int str_middle_insert_blank( const char* str, int line_len );

bool printable_char( char ch );
bool printable_mem( const char* mem, int len );

// command line
char **split_command_line( char *lpCmdLine, int *pNumArgs );

// no more------------------------------
