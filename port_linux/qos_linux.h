
bool qos_start(void);


// because these function is high related with os system
// so write in port
int elapse_us_open(void);
int elapse_us_reset( int elapse_id );
int elapse_us_update( int elapse_id );
int elapse_us_close( int elapse_id );

// no more
