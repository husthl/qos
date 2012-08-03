


// virtual file system
typedef struct qstat{
    int size;           // total size, in bytes
}qstat_t;

// return: true, the path is belong to this vfs id.
typedef bool (*path_match_t)( const char* path );
// return: fid
typedef int (*open_t)( const char* path, int oflag, void* config );
typedef bool (*close_t)( int fid );
// return: success read byte number
typedef int (*read_t)( int fid, void* buf, int nbyte );
// return: success write byte number
typedef int (*write_t)( int fid, const void* buf, int nbyte );
// return: ok?
typedef bool (*fstat_t)( int fid, struct qstat *buf );
typedef struct{
    path_match_t path_match;
    open_t open;
    close_t close;
    read_t read;
    write_t write;
    fstat_t fstat;
}file_operation_t;



// file system
bool file_system_init(void);
bool file_system_exit(void);
bool file_system_register_vfs( const file_operation_t* fop );




// because file_id have been use in file_id.h, so name it as fid
// fid_t must include fid_base_t, and must be first element:
// usuage:
// typedef struct{
//      fid_base_t base;
//      other ...
// }fid_t
typedef struct fid_base_s{
    int vfs_id;
}fid_base_t;
int fid_to_vfs_id( const fid_base_t* base );

// because open, close, etc, already use in C standar file operation,
// so add prefix 'q'
int qopen( const char* path, int oflag, void* config );
bool qclose( int fid );
int qread( int fid, void* buf, int nbyte );
int qwrite( int fid, const void* buf, int nbyte );
bool qfstat( int fid, struct qstat* buf );



// no more

