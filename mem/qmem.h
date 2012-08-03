

#define qfreez( p )      do{ if( NULL != p ){ qfree((void*)p); p = NULL; } }while(0)

#if 0
#define offsetof(st, m) \
     ((size_t) ( (char *)&((st *)0)->m - (char *)0 ))
#endif


#define container_of(ptr, type, member) ({                              \
                const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
                (type *)( (char *)__mptr - offsetof(type,member) );})




void* qmalloc_real( int file_id, int line, size_t size );
#define qmalloc( size )	                                                        \
	(void*)({											                        \
    void* _ret = qmalloc_real( this_file_id, __LINE__, size );                  \
    _ret;                                                                       \
	})

void* qrealloc_real( int file_id, int line, void *ptr, int newsize );
#define qrealloc( ptr, newsize )                                                \
    (void*)({                                                                   \
    void* _ret = qrealloc_real( this_file_id, __LINE__, ptr, newsize );         \
    _ret;                                                                       \
    })

bool qfree_real( int file_id, int line, void* ptr );
#define qfree( ptr )                                                            \
    do{                                                                         \
        if( !qfree_real( this_file_id, __LINE__, ptr ) ){                       \
            uverify( false );                                                   \
        }                                                                       \
    }while(0)



typedef struct mem_info_s{
    int malloc_exec_ix;         // 为了分配该内存，调用malloc()的序号，可以作为内存的索引号。
                                // 该索引号可以用来配对malloc和free。
                                // 因为同一个地址，释放后可以被再次分配，因此，用内存地址
                                // 配对malloc和free，不合适。
                                // 而且，内存地址形如0x12345678这样的大数，很难阅读。
    int file_id;
    int line;

    void* ptr;
    int size;
}mem_info_t;
// return: true, callback for next. Otherwise, false, stop for each.
typedef bool (*qmem_for_eack_t)( const mem_info_t* mem_info, int ix, void* arg );
int qmem_for_each( qmem_for_eack_t callback, void* arg );

bool qmem_info( mem_info_t* mem_info, void* ptr );
int qmem_count(void);

typedef struct{
    char* left;
    const char* left_pattern;
    int left_len;

    char* right;
    const char* right_pattern;
    int right_len;
}qmem_edge_t;
bool qmem_edge( qmem_edge_t* mem_edge, void* ptr );
bool qmem_check( void* ptr );
bool qmem_check_all( mem_info_t* mem_info_fail );

// no more------------------------------
