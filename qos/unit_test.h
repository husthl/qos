
#ifndef GCFG_UNIT_TEST_EN
#error "undef GCFG_UNIT_TEST_EN"
#endif


#if GCFG_UNIT_TEST_EN > 0

typedef bool (*unit_test_item_t)(void);

void unit_test_pump(void);
bool unit_test_file_pump_table( const unit_test_item_t table[] );

void unit_test_fail(void);
bool unit_test_is_ok(void);

void _test_fail( int file_id, int line, const char* express );
// in test_in_function, return false to exit test item and stop unit test.
// in test_in_task, return false to stop the task
#define test( a )                                                   \
    do{                                                             \
        if( !(a) ){                                                 \
            _test_fail( this_file_id, __LINE__, #a );               \
            return false;                                           \
        }                                                           \
    }while(0)
    

#else
#define unit_test_pump()                            
#define unit_test_file_pump_table( item_list )		false
#endif


// no more
