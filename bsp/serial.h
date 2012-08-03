/*
********************************************************************************
*
* Copyright(C),1994-2008,Routon Electronic Co.,Ltd.
*
* �ļ���: serial.h
*
* ��������: ����ģ��.
*
* �ļ��޸���ʷ:
*
* �汾��    ����                ����          	˵��
* 01a       2008-10-30          huanglin        �����ļ�
*
********************************************************************************
*/

#define BAUD_115200		(115200)
#define BAUD_38400		(38400)
#define BAUD_19200		(19200)
#define	BAUD_9600		(9600)
#define BAUD_4800		(4800)
#define BAUD_2400		(2400)
#define BAUD_1200		(1200)
#define BAUD_300		(300)

typedef struct {
	int fd;

	int baud;
	int databit;	// ����λ   ȡֵ Ϊ 7 ����8 
	int stopbit;	// ֹͣλ   ȡֵΪ 1 ����2 
	char parity;	// Ч������ ȡֵΪN,E,O,,S 

	int tx_cnt;
	int rx_cnt;
}serial_t;
#define SERIAL_DEVICE_PATH_DEAFULT	"/dev/ttyS0"
#define SERIAL_FD_INVALID			0
#define SERIAL_DEFAULT {			\
	SERIAL_FD_INVALID				\
	, BAUD_9600, 8, 1, 'n'			\
	, 0, 0							\
}




bool serial_vfs_open(void);
// usuage:
// serial_t serial_config = { 0, 9600, 8, 1, 'n', 0, 0 };
// const int file_id = qopen( "/dev/ttyS0", &serial_config );
// const int nwrite = qwrite( file_id, "123", 3 );
// char buf[10];
// const int nread = qread( file_id, buf, 10 );
// qclose( file_id );



// no more
