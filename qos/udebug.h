
#ifdef GCFG_DEBUG_EN
#else
#error "not define GCFG_DEBUG_EN"
#endif

#if GCFG_DEBUG_EN > 0
// 因为__FILE__会占用很多flash空间，为了避免，可以在每个文件定义一个file_id，用一个函数来映射
// file_id到文件名之间的映射
#define uverify_macro( a, message )                                         \
    do{                                                                     \
        if(!(a)){                                                           \
            do{                                                             \
                uverify_real( this_file_id, __LINE__, message );            \
            }while(0);                                                      \
        }                                                                   \
    }while(0)
#define uverify( a )                uverify_macro( a, #a )
#else
#define uverify( a )
#endif


#if GCFG_DEBUG_EN > 0
// 因为很多情况下，语句为
// if( !a ){
//      uverify( false );           // 在测试状态下，告诉错误的行数
//      return false;               // 退出
// }
// 将其简化为下面的宏定义，含义为：执行，错了就退出
#define return_false_if( a )       \
        do{ if( (a) ){ uverify_macro( false, #a ); return false; } }while(0)
#else
#define return_false_if( a )        \
        do{ if( (a) ){ return false; } }while(0)
#endif



#if GCFG_DEBUG_EN > 0
// 因为很多情况下，语句为
// if( !a ){
//      uverify( false );           // 在测试状态下，告诉错误的行数
//      return 0;                   // 退出
// }
// 将其简化为下面的宏定义，含义为：执行，错了就退出
#define return_0_if( a )            \
        do{ if( (a) ){ uverify_macro( false, #a ); return 0; } }while(0)
#else
#define return_0_if( a )            \
        do{ if( (a) ){ return 0; } }while(0)
#endif


// 调试相关
// usuage:
// here( 1 )
// here( 2 )
#define here( n )	do{ here_real( this_file_id, __LINE__, #n, false ); }while(0)
#define herek( n )	do{ here_real( this_file_id, __LINE__, #n, true ); }while(0)

#define dstr( s )   do{ dstr_real( this_file_id, __LINE__, #s, (char*)s, false ); }while(0)
#define dstrk( s )   do{ dstr_real( this_file_id, __LINE__, #s, (char*)s, true ); }while(0)

#define dmem( m, len )   do{ dmem_real( this_file_id, __LINE__, #m, (char*)m, len, false ); }while(0)
#define dmemk( m, len )   do{ dmem_real( this_file_id, __LINE__, #m, (char*)m, len, true ); }while(0)

#define var( n )   do{ var_real( this_file_id, __LINE__, #n, n, false ); }while(0)
#define vark( n )   do{ var_real( this_file_id, __LINE__, #n, n, true ); }while(0)


bool uverify_real( int fild_id, int line, const char* message );

void var_real( int file_id, int line, const char* var_name, int n, bool wait_key );
void here_real( int file_id, int line, const char* message, bool wait_key );
void dstr_real( int file_id, int line, const char* str_name, const char* str, bool wait_key );
void dmem_real( int file_id, int line, const char* mem_name, const char* mem, int len, bool wait_key );


// no more------------------------------
