/*
********************************************************************************
*
* Copyright(C),1994-2008,Routon Electronic Co.,Ltd.
*
* 文件名: serial.h
*
* 内容描述: 串口模块.
*
* 文件修改历史:
*
* 版本号    日期                作者          	说明
* 01a       2008-10-30          huanglin        创建文件
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
	int databit;	// 数据位   取值 为 7 或者8 
	int stopbit;	// 停止位   取值为 1 或者2 
	char parity;	// 效验类型 取值为N,E,O,,S 

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
