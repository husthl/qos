
#include "stdarg.h"

// 字符串_内存拷贝相关
bool str_to_str( char* dst, int dst_size, const char* src );
bool mem_to_mem( void* dst, int dst_size, const void* src, int src_size );

bool mem_to_str( char* str, int str_size, const void* mem, int n );
bool str_to_mem( void* mem, int mem_size, const char* str );

bool append_str( char* dst, int dst_size, const char* src );
int strsize( const char* str );

// 内存处理
void const *memcpy_src_offset( void* dst, const void* src, int byte_num );
void *memcpy_dst_offset( void* dst, const void* src, int byte_num );
bool memcpy_fill( char* dst, int dst_size, const char* src, int src_len, char fill );
void switch_2_byte( char byte[2] );



// 很多情况下，需要将一个没有0结尾的字符数组，临时转换为0结尾的字符串，用来显示等。 
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

// 一个snprintf()安全版本
// 1.	简化工作，会返回一个内存指针，存放格式化后的结果。
// 2.   可以指定内存在堆/堆栈上。
// 3.	如果结果放在堆栈上。你无需为内存分配，也无需释放内存。
// 4.	对snprintf()结果进行检查。如果不对，返回的NULL
// 5.	为了尽量减少二意性，和sprintf()传入、传出的参数，意义尽量相同
//
// 参数含义：
// return		格式化后的字符串首地址
// 
// usuage: 
//		char const * const p = sprintf_on_mem( alloca, "%s%d", "abc", 123 );
//
// read: g_new()宏定义写法。
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




// 显示相关
//#define printf          u_printf
//int u_printf( const char* fmt, ... );
int str_middle_insert_blank( const char* str, int line_len );

bool printable_char( char ch );
bool printable_mem( const char* mem, int len );

// command line
char **split_command_line( char *lpCmdLine, int *pNumArgs );

// no more------------------------------
