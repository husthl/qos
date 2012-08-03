


int xmodem_open( int fid );
bool xmodem_close( int xmodem_id );
bool xmodem_start_receive( int xmodem_id, int dst_file_fid );
bool xmodem_start_transmit( int xmodem_id, int src_file_fid );

// set sync can change waiting transmit start time.
// transmit_start_time = sync_timeout_cnt_max * sync_timeout_sec (second)
bool xmodem_sync_set( int id, int sync_timeout_cnt_max, int sync_timeout_sec );

// return: ok?
typedef bool (*xmodem_end_t)( void* arg );
bool xmodem_set_end_callback( int id, xmodem_end_t end_callback, void* arg );

typedef struct{
    // if last tx or rx success.
    bool ok;

    bool rxing;
    int rx_nbyte;
    int dst_file_fid;

    bool txing;
    int tx_nbyte;

    // sync timeout interval second & count max, 
    // so the max waiting transmit start time is 
    // sync_timeout_cnt_max * sync_timeout_sec (second)
    int sync_timeout_cnt_max;
    int sync_timeout_sec;
}xmodem_info_t;
bool xmodem_info( int id, xmodem_info_t* info );



// no more

