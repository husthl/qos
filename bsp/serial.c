/*
 * File      : serial.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-11     huanglin     first version
 *
 */


/*
serial for linux

refrence:
1	http://blog.csdn.net/tigerjb/article/details/6179291
	Linux下的串口编程(二） 

2	http://learn.akae.cn/media/ch28s04.html
	清晰的解释了O_NONBLOCK 和 EAGAIN 用法
*/




#include <stdio.h>  
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>      /*文件控制定义*/
#include <errno.h>      /*错误号定义*/
#include <unistd.h>
#include <termios.h>    /*PPSIX终端控制定义*/

#include "inc.h"
#include "vfs.h"
#include "serial.h"
#include "qmem.h"

#define this_file_id   file_id_serial




// 调用这个函数之前，正确配置serial_t结构体
bool serial_open( const char* path, serial_t* serial );
bool serial_close( serial_t* serial );

int serial_write( serial_t* serial, const char* buf, int nbyte );

bool serial_read_char( serial_t * const serial, char* ch, int timeout_ms );
int serial_read( serial_t* serial, char* buf, int nbyte );
int serial_read_timeout_first_char( serial_t* serial, char* buf, int buf_size, int first_char_timeout_ms );
int serial_read_timeout( serial_t* serial, char* buf, int buf_size, int first_char_timeout_ms, int interval_char_timeout_ms );


typedef struct{
    fid_base_t base;
    serial_t serial;
}fid_t;


static bool spath_match( const char* path ){
    const char* const serial_path_pattern = "/dev/ttyS";
    const int nbyte = strlen( serial_path_pattern );
    if( 0 == memcmp( serial_path_pattern, path, nbyte ) ){
        return true;
    }
    return false;
}
static int sopen( const char* path, int oflag, void* config ){
    uverify( NULL != path );
    if( NULL == config ){
        return 0;
    }

    serial_t* const serial = config;
    serial->fd = 0;
    uverify( NULL != serial );

    if( !serial_open( path, serial ) ){
        return 0;
    }

    fid_t* const p = qmalloc( sizeof( fid_t ) );
    return_false_if( NULL == p );

    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_t ) );
    }
    p->serial = *serial;
    
    return (int)p;
}

static bool sclose( int fid ){
    uverify( 0 != fid );

    fid_t* const p = (fid_t*)fid;
    return_false_if( !serial_close( &p->serial ) );
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_t ) );
    }
    qfree( p );
    return true;
}
static int sread( int fid, void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_t* const p = (fid_t*)fid;
    serial_t* const serial = &p->serial;
    return serial_read( serial, buf, nbyte );
}
static int swrite( int fid, const void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_t* const p = (fid_t*)fid;
    serial_t* const serial = &p->serial;
    return serial_write( serial, buf, nbyte );
}
static bool sfstat( int fid, struct qstat* buf ){
    uverify( 0 != fid );
    uverify( NULL != buf );

    return false;
}




bool serial_vfs_open(void){
    const file_operation_t fo = { spath_match, sopen , sclose , sread , swrite, sfstat };
    return_false_if( 0 == file_system_register_vfs( &fo ) );
    return true;
}





/*******************************************************************
* 名称：                UART0_Set
* 功能：                设置串口数据位，停止位和效验位
* 入口参数：        fd         串口文件描述符
*                              speed      串口速度
*                              flow_ctrl  数据流控制
*                           databits   数据位   取值为 7 或者8
*                           stopbits   停止位   取值为 1 或者2
*                           parity     效验类型 取值为N,E,O,,S
*出口参数：              正确返回为1，错误返回为0
*******************************************************************/
// please read: 
// 1    http://blog.163.com/shaohj_1999@126/blog/static/63406851201101410739851/
//      串口编程linux，续（缓冲区处理）  
// 2    http://tldp.org/HOWTO/html_single/Serial-HOWTO/#ss3.3
//      Serial HOWTO
//      very good document about linux serial
static int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity){
	int i;
	const int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
		B38400, B19200, B9600, B4800, B2400, B1200, B300 };
	int   name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,      38400,  19200,  9600, 4800, 2400, 1200,  300 };
	struct termios options;

	// tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数,
	// 还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.
	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");    
		return false; 
	}

	//设置串口输入波特率和输出波特率
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++){
		if  (speed == name_arr[i]){       
			cfsetispeed( &options, speed_arr[i] ); 
			cfsetospeed( &options, speed_arr[i] );
		}
	}     

	// 如果不是开发终端之类的，只是串口传输数据，而不需要串口来处理，那么使用原始模式(Raw Mode)方式来通讯，设置方式如下：
	// 下面是关于raw mode的介绍：
	// 终端简介
    // 历史上真实的 terminal 其实就是一个 "mon + keyborad + 少量内存 + cable" 的设备, 不具备实际的计算功能, "tty" 这个词本身即是 teletype (电传打字机)的意思. 而如今我们用的都是 emulattion. (ref http://tldp.org/HOWTO/Text-Terminal-HOWTO-1.html#ss1.7) 
	// 终端IO
	// 设置
    // 用户进程以及实际的终端驱动之间存在着一个终端行规程( terminal line discipline ), 从终端的输入会先经过此而后才进入用户进程(向终端的输出也会先经过此), 这个模块在不同的模式下会对输入输出进行不同的处理(例如在规范模式下, 判断出用户输入的是 Ctrl+C, 因此给用户进程发送一个中断信号, 用户进程无法获得此字符; 而在原始模式下, 用户进程则可以获得此字符).
    // 模式实际上是一组终端设备特性所组成, 这些特性可通过设置 termios 结构的标志位然后通过 tcgetattr/tcsetattr .. 等终端IO函数进行控制.
    // 常用的模式有三种
    //    以行为单位的, 对特殊字符进行处理的模式 (cooked mode)
    //    原始模式, 不以行为单位, 不对特殊字符进行处理 (raw mode)
    //    cbreak 模式, 类似原始模式, 但某些特殊字符也进行处理 
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   /*Output*/

	//修改控制模式，保证程序不会占用串口
	options.c_cflag |= CLOCAL;
	//修改控制模式，使得能够从串口中读取输入数据
	options.c_cflag |= CREAD;

	//设置数据流控制
	switch(flow_ctrl){
		case 0 ://不使用流控制
			options.c_cflag &= ~CRTSCTS;
			break;   
		case 1 ://使用硬件流控制
			options.c_cflag |= CRTSCTS;
			break;
		case 2 ://使用软件流控制
			options.c_cflag |= IXON | IXOFF | IXANY;
			break;
	}

	//设置数据位
	options.c_cflag &= ~CSIZE; //屏蔽其他标志位
	switch (databits) {  
		case 5    :
			options.c_cflag |= CS5;
			break;
		case 6    :
			options.c_cflag |= CS6;
			break;
		case 7    :    
			options.c_cflag |= CS7;
			break;
		case 8:    
			options.c_cflag |= CS8;
			break;  
		default:   
			fprintf(stderr,"Unsupported data size/n");
			return (false); 
	}

	//设置校验位
	switch (parity){  
		case 'n':
		case 'N': //无奇偶校验位。
			options.c_cflag &= ~PARENB; 
			options.c_iflag &= ~INPCK;    
			break; 
		case 'o':  
		case 'O'://设置为奇校验    
			options.c_cflag |= (PARODD | PARENB); 
			options.c_iflag |= INPCK;             
			break; 
		case 'e': 
		case 'E'://设置为偶校验  
			options.c_cflag |= PARENB;       
			options.c_cflag &= ~PARODD;       
			options.c_iflag |= INPCK;       
			break;
		case 's':
		case 'S': //设置为空格 
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break; 
		default:  
			fprintf(stderr,"Unsupported parity/n");   
			return (false); 
	} 

	// 设置停止位 
	switch (stopbits){  
		case 1:   
			options.c_cflag &= ~CSTOPB; 
			break; 
		case 2:   
			options.c_cflag |= CSTOPB; 
			break;
		default:   
			fprintf(stderr,"Unsupported stop bits/n"); 
			return (false);
	}

	//修改输出模式，原始数据输出
	options.c_oflag &= ~OPOST;

	//设置等待时间和最小接收字符
	//options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */  
    // because I use qos, so NO function will block in function.
	options.c_cc[VTIME] = 0; /* 读取一个字符等待1*(1/10)s */  
	options.c_cc[VMIN] = 0; /* 读取字符的最少个数为1 */


	//added by tianjin 2011-11-24,this is error,in such case,linux serial will do some change on the byte we receive,for example,it will turn 0x0d to 0x0a 
	//put the config at the place because it will not be recovered by other config
	options.c_iflag &= ~ICRNL;
	options.c_iflag &= ~IXON;
	
    fcntl(fd, F_SETFL, FNDELAY); //非阻塞
    //fcntl(fd, F_SETFL, 0); // 阻塞

	//如果发生数据溢出，接收数据，但是不再读取
	tcflush(fd,TCIFLUSH);

	//激活配置 (将修改后的termios数据设置到串口中）
	if (tcsetattr(fd,TCSANOW,&options) != 0){
		perror("com set error!/n");  
		return (false); 
	}

	return (true); 
}




/*
********************************************************************************
* 功  能: 设置串口速度.
* 返  回：0 成功, -1失败
*
********************************************************************************
*/
int SetSpeed( int fd, int speed ){
	int i;
	int status;
	struct termios Opt;
    int baud;

	const int baud_table[8][2] = {
		{115200, B115200},  //zhp 以前为什么没加这个定义？现在加上，蓝牙控制器就是这个速度。
		{38400,	B38400},
 		{19200,	B19200},
 		{9600,	B9600},
 		{4800,	B4800},
 		{2400,	B2400},
 		{1200,	B1200},
 		{300,	B300}
	};

	int iFound = 0;

	for (i = 0; i < (sizeof(baud_table) / sizeof(int)) / 2; i++){
		if (speed == baud_table[i][0]){
			baud = baud_table[i][1];
			iFound = 1;
			break;
		}
	}

	if (iFound == 0){
        uverify( false );
		return -1;
	}

	tcgetattr(fd, &Opt);

	/* 清空数据线，启动新的串口设置 */
    tcflush(fd, TCIOFLUSH);
	cfsetispeed(&Opt, baud);
	cfsetospeed(&Opt, baud);
	status = tcsetattr(fd, TCSANOW, &Opt);
	if(status != 0){
		perror("tcsetattr() failed");
		return -1;
	}
 	return 0;
}

/*
********************************************************************************
* 功  能: 设置串口数据位，停止位和效验位.
********************************************************************************
*/
int SetParity(
	int fd,				/* 打开的串口文件句柄 */
	int databits,		/* 数据位   取值 为 7 或者8 */
	int stopbits,		/* 停止位   取值为 1 或者2 */
	int parity			/* 效验类型 取值为N,E,O,,S */
){
	struct termios options;

	if (tcgetattr(fd,&options) != 0)
	{
		perror("SetupSerial 1");
		return(false);
	}

#if 1	/* 不需要回车结束符控制 */
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  //Input
	options.c_oflag  &= ~OPOST;   //Output
#endif

	options.c_cflag &= ~CSIZE;
	switch (databits) /*设置数据位数*/
	{
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			fprintf(stderr,"Unsupported data size\n");
			return (false);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;

		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported parity\n");
			return (false);
	}

	/* 设置停止位*/
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return (false);
	}
	/* Set input parity option */
	if (parity != 'n')
	{
		options.c_iflag |= INPCK;
	}

	options.c_cc[VTIME] = 1; // 100 ms 超时重发
	options.c_cc[VMIN] = 0;


    options.c_iflag &= ~(ICRNL | INLCR);//zhp解决E R的键值中的0D被自动转成0A的问题。        
	options.c_iflag &= ~(IXOFF | IXON);//zhp解决Y U的键值中的11/13被去掉的问题。
	//把上面两句话加上后就解决了掉字节的问题，但是再屏蔽掉，也不出问题了。奇怪！后来发现重启之后问题重新，说明上两句必须打开。

	tcflush(fd, TCIFLUSH); 	/* Update the options and do it NOW */
	if( tcsetattr( fd,TCSANOW,&options ) != 0)
	{
		perror("SetupSerial 3");
		return (false);
	}
	return (true);
}

/*
********************************************************************************
* 功  能: 打开串口.
********************************************************************************
*/
static int OpenDev( char const *dev ){
	// O_NOCTTY:表示打开的是一个终端设备，程序不会成为该端口的控制终端。如果不使用此标志，任务一个输入(eg:键盘中止信号等)都将影响进程。
	// O_NDELAY:表示不关心DCD信号线所处的状态（端口的另一端是否激活或者停止）。
	const int fd = open( dev, O_RDWR | O_NOCTTY | O_NDELAY );
	if (-1 == fd){
		// log( "Can't Open Serial Port %s", dev );
		uverify( false );
		return -1;
	}
	
	return fd;
}


#if 0
/*
********************************************************************************
*
* 函数名: WaitSerialData
*
* 功  能: 等待串口数据
*
* 输  入：N/A
*
* 输  出：N/A
*
* 返  回：	0 - 有数据
*			其它 - 无数据
*
********************************************************************************
*/
int WaitSerialData(int _fd, int _iTimeOut)
{
//	struct timeval
//	{
//		long tv_sec;		/* seconds */
//		long tv_usec;		/* microseconds */
//	};

	fd_set rdfds;			/* 先申明一个 fd_set 集合来保存我们要检测的 socket句柄 */
	struct timeval tv;		/* 申明一个时间变量来保存时间 */
	int ret;				/* 保存返回值 */
	int fd;


	FD_ZERO(&rdfds); 		/* 用select函数之前先把集合清零 */
	FD_SET(fd, &rdfds);		/* 把要检测的句柄socket加入到集合里 */

	tv.tv_sec = _iTimeOut;
	tv.tv_usec = 0;			/* 设置select等待的最大时间为2秒 */

	/* 检测我们上面设置到集合rdfds里的句柄是否有可读信息 */

	ret = select(fd + 1, &rdfds, 0, (fd_set*)0, &tv);

	if (ret < 0)
	{
		//perror("select");/* 这说明select函数出错 */
		return -1;
	}
	else if(ret == 0)
	{
		/* 说明在我们设定的时间值1秒加500毫秒的时间内，socket的状态没有发生变化 */
		return -1;
	}
	else
	{
		/* 说明等待时间还未到1秒加500毫秒，socket的状态发生了变化 */
		/* ret这个返回值记录了发生状态变化的句柄的数目，由于我们只监视了socket这一个句柄，所以这里一定ret=1，
		如果同时有多个句柄发生变化返回的就是句柄的总和了 */
	    /* 这里我们就应该从socket这个句柄里读取数据了，因为select函数已经告诉我们这个句柄里有数据可读 */
	    if (FD_ISSET(fd, &rdfds))
	    {
	        /* 可以读取数据 */
	        return 0;
	    }
	    else
	    {
	    	return -1;
	    }
	}
}

#endif

/*
* 功  能: 从串口获得一个字节数据
* 返  回：	1 - 有数据
*			0 - 无数据
*/
size_t GetSerialChar(int _fd,unsigned char *ch){
	if (read(_fd, ch, 1) == 1){
		return 1;
	}
	else{
		return 0;
	}
}

/*
* 功  能: 从串口获得一个字符串数据
* 输  入：s：字符串指针，len：长度
* 返  回：	len - 有数据
*			0 - 无数据
*/
size_t GetSerialString(int _fd,unsigned char *s,size_t len)
{
	int il;
	il = read(_fd, s, len); 
	if (il == len){
		return len;
	}
	else{
		return 0;
	}
}

/*
* 功  能: 向串口写一个字符串数据
* 输  入：s：字符串指针，len：长度
* 返  回：	s - 有数据
*			NULL - 无数据
*/
size_t PutString(int _fd,unsigned char *s,size_t len)
{
	if (write(_fd, s, len) == len)
	{
		return len;
	}
	else
	{
		return 0;
	}
}

/*
* 功  能: 向串口发送一串数据.
* 返  回：	0 - 有数据
*			其它 - 无数据
*/
int PutChar(int _fd, char _ch){
	if (write(_fd, &_ch, 1) <= 0){
		return -1;
	}
	else{
		return 0;
	}
}





static bool serial_is_open( const serial_t* serial ){
	uverify( NULL != serial );
	return SERIAL_FD_INVALID != serial->fd;
}





bool serial_open( const char* path, serial_t* serial ){
    uverify( NULL != path );
    uverify( NULL != serial );

	int fd = 0;
	uverify( !serial_is_open( serial ) );

	fd = OpenDev( path );
	if( -1 == fd ){
		uverify( false );
		return false;
	}
	serial->fd = fd;

	if( !UART0_Set( fd, serial->baud, 0, serial->databit, serial->stopbit, serial->parity ) ){
		uverify( false );
		return false;
	}

	serial->rx_cnt = 0;
	serial->tx_cnt = 0;

	return true;
}





bool serial_open_1( const char* path, serial_t* serial ){
	int fd = 0;

	uverify( NULL != serial );
	uverify( !serial_is_open( serial ) );

	fd = OpenDev( path );
	if( -1 == fd ){
		uverify( false );
		return false;
	}
	serial->fd = fd;

	if( 0 != SetSpeed( fd, serial->baud ) ){
		uverify( false );
		return false;
	}
	
	if( !SetParity( fd, serial->databit, serial->stopbit, serial->parity ) ){
		uverify( false );
		return false;
	}

	serial->rx_cnt = 0;
	serial->tx_cnt = 0;

	return true;
}




bool serial_close( serial_t* serial ){
	uverify( NULL != serial );
	uverify( serial_is_open( serial ) );

	if( 0 != close( serial->fd ) ){
		uverify( false );
		return false;
	}

	serial->fd = SERIAL_FD_INVALID;
	return true;
}





// return: 
//		true		receive char
//		false		NOT receive char
bool serial_read_char( serial_t* serial, char* ch, int timeout_ms ){
	uverify( NULL != serial );
	uverify( serial_is_open( serial ) );
	uverify( NULL != ch );

	if( 0 == serial_read_timeout_first_char( serial, ch, 1, timeout_ms ) ){
		return false;
	}

	return true;
}



// return: the byte number write in buf
int serial_read( serial_t * const serial, char* buf, int nbyte ){
	int nread = 0;

	uverify( NULL != serial );
	uverify( serial_is_open( serial ) );
	uverify( NULL != buf );
	uverify( 0 != nbyte );
	
	// 函数说明 read()会把参数fd 所指的文件传送count个字节到buf指针所指的内存中。若参数count为0，则read()不会有作用并返回0。返回值为实际读取到的字节数，如果返回0，表示已到达文件尾或是无可读取的数据，此外文件读写位置会随读取到的字节移动。
	// 附加说明 如果顺利read()会返回实际读到的字节数，最好能将返回值与参数count 作比较，若返回的字节数比要求读取的字节数少，则有可能读到了文件尾、从管道(pipe)或终端机读取，或者是read()被信号中断了读取动作。当有错误发生时则返回-1，错误代码存入errno中，而文件读写位置则无法预期。
	// 错误代码 
	// EINTR 此调用被信号所中断。
	// EAGAIN 当使用不可阻断I/O 时（O_NONBLOCK），若无数据可读取则返回此值。
	// EBADF 参数fd 非有效的文件描述词，或该文件已关闭。
	nread = read( serial->fd, buf, nbyte ); 
	if( -1 == nread ){
		//  When  attempting  to read from a regular file with mandatory
		// file/record locking set  (see  chmod(2)),  and  there  is  a
		// blocking  (i.e.  owned by another process) write lock on the
		// segment of the file to be read:
        //		If O_NDELAY or O_NONBLOCK is set, read returns  -1  and
        //		sets errno to EAGAIN.
        //		If O_NDELAY and O_NONBLOCK are clear, read sleeps until
        //		the blocking record lock is removed.
		if( EAGAIN == errno ){
			// if there are a writing action while you call read(), read() will
			// return -1, and set errno = EAGAIN;
			return 0;
		}
		
		uverify( false );
		return 0;
	}

	serial->rx_cnt += nread;
	return nread;
}



// 本函数的设计目标为：尽量接收串口数据，直到串口设备不发送了，或者缓冲区满了。
// 本函数退出条件如下：
// 1	等待第一个字符，超过first_char_timeout_ms。
//		场景：串口设备很长时间，一个字符没有应答。
// 2	接收字符到达buf_size个，即缓冲区满
// 3	接收最后一个字符，超过interval_char_timeout_ms。
//		场景：串口设备，发送了一些字符后，没有多余的内容发送了。
//		或则，两个字符之间的间隔时间，超过了interval_char_timeout_ms.(这可以理解为串口发送设备故障)
//		也可以理解为，串口字符之间时间间隔超过interval_char_timeout_ms，就返回
// return: the byte number write in buf
int serial_read_timeout( serial_t* serial, char* buf, int buf_size, int first_char_timeout_ms, int interval_char_timeout_ms ){
	int rx_cnt = 0;

	rx_cnt = serial_read_timeout_first_char( serial, buf, buf_size, first_char_timeout_ms );
	if( 0 == rx_cnt ){
		// 场景：串口设备很长时间，一个字符没有应答。
		return rx_cnt;
	}
	if( rx_cnt >= buf_size ){
		// 场景：缓冲区满
		uverify( rx_cnt == buf_size );
		return rx_cnt;
	}

	// 已经接收到一部分字符，继续接收
	while( true ){
		char* const p = buf + rx_cnt;
		const int buf_room = buf_size - rx_cnt;
		const int rx_cnt_loop = serial_read_timeout_first_char( serial, p, buf_room, interval_char_timeout_ms );

		if( 0 == rx_cnt_loop ){
			// 场景：串口设备，发送了一些字符后，没有多余的内容发送了。
			// 或则，两个字符之间的间隔时间，超过了interval_char_timeout_ms.
			return rx_cnt;
		}

		rx_cnt += rx_cnt_loop;
		if( rx_cnt >= buf_size ){
			// 场景：缓冲区满
			uverify( rx_cnt == buf_size );
			return rx_cnt;
		}
	}

	uverify( false );
	return 0;
}




// return: true:	There are something for reading.
static bool wait_fd_have_something_read( int fd, int first_char_timeout_ms ){
	fd_set rdfds;			// 先申明一个 fd_set 集合来保存我们要检测的 socket句柄
	FD_ZERO( &rdfds ); 		// 用select函数之前先把集合清零 
	FD_SET(fd, &rdfds);		// 把要检测的句柄socket加入到集合里 

	// 设置select等待的最大时间
	struct timeval tv;		// 申明一个时间变量来保存时间
	tv.tv_sec = 0;
	tv.tv_usec = MS_TO_US( first_char_timeout_ms );			

	// 检测我们上面设置到集合rdfds里的句柄是否有可读信息
	// Select在Socket编程中还是比较重要的，可是对于初学Socket的人来说都不太爱用Select写程序，他们只是习惯写诸如connect、accept、recv或recvfrom这样的阻塞程序（所谓阻塞方式block，顾名思义，就是进程或是线程执行到这些函数时必须等待某个事件的发生，如果事件没有发生，进程或线程就被阻塞，函数不能立即返回）。可是使用Select就可以完成非阻塞（所谓非阻塞方式non-block，就是进程或线程执行此函数时不必非要等待事件的发生，一旦执行肯定返回，以返回值的不同来反映函数的执行情况，如果事件发生则与阻塞方式相同，若事件没有发生则返回一个代码来告知事件未发生，而进程或线程继续执行，所以效率较高）方式工作的程序，它能够监视我们需要监视的文件描述符的变化情况??????读写或是异常。下面详细介绍一下！
	// 保存返回值
	while(1){
		const int select_ret = select( fd + 1, &rdfds, 0, (fd_set*)0, &tv );

		if( 0 < select_ret ){
			// 0 < select_ret
			uverify( 0 < select_ret );
			// 说明等待时间还未到超时，socket的状态发生了变化 
			// ret这个返回值记录了发生状态变化的句柄的数目，
			// 由于我们只监视了socket这一个句柄，所以这里一定ret=1，
			// 如果同时有多个句柄发生变化返回的就是句柄的总和了 
			uverify( 1 == select_ret );
			
			// 这里我们就应该从socket这个句柄里读取数据了，
			// 因为select函数已经告诉我们这个句柄里有数据可读 
			if( !FD_ISSET( fd, &rdfds ) ){
				uverify( false );
				return false;
			}

			return true;
		}

		if( 0 == select_ret ){
			// 超时
			// 说明在我们设定的时间值内，socket的状态没有发生变化
			return false;
		}

		if( 0 > select_ret ){
			// 这说明select函数出错
			// dodo: huanglin 20111205: 当用power -ulc，连续运行读卡器测试
			// 按ctrl-c时，偶尔，导致测试出错，下面一行错误。
			// 可能原因：因为串口句柄在按ctrl-c时，已经先于应用程序关闭
			// re: huanglin 20111205: please read below topic:
			//		signal SA_RESTART
			//		select EINTR
			//
			// EINTR 此调用被信号所中断。
			// 例如：按了ctrl-c
			if( EINTR == errno ){
				// 再等待一次
				// read: signal SA_RESTART
				continue;
			}
			else{
				uverify( false ); 
				return false;
			}
		}
	}

	uverify( false );
	return false;
}




// 最多等待first_char_timeout_ms，如果还一个字符都没有收到，则返回
// 如果有，则尽量读缓冲区，读到了，就立即返回
// return: the byte number write in buf
int serial_read_timeout_first_char( serial_t* serial, char* buf, int buf_size, int first_char_timeout_ms ){
	uverify( NULL != serial );
	uverify( serial_is_open( serial ) );
	uverify( NULL != buf );
	uverify( 0 != buf_size );

	const int fd = serial->fd;
	if( !wait_fd_have_something_read( fd, first_char_timeout_ms ) ){
		return 0;
	}

	// 可以读取数据
	const int nread = serial_read( serial, buf, buf_size );
	if( 0 == nread ){
		uverify( false );
		return 0;
	}

	return nread;
}



// when 0 == len, it will flush serial write buffer.
// On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
int serial_write( serial_t* serial, const char* buf, int nbyte ){
	uverify( NULL != serial );
	uverify( serial_is_open( serial ) );
	uverify( NULL != buf );

	const int nwrite = write( serial->fd, buf, nbyte );
	serial->tx_cnt += nbyte;
    return nwrite;
}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static int elapse_id = 0;
static serial_t serial = SERIAL_DEFAULT;
static int m_fid = 0;
#define WRITE_MESSAGE		"123abc"

const char* const write_message = WRITE_MESSAGE;
const int data_len = sizeof(WRITE_MESSAGE) - 1;
char buf[sizeof(WRITE_MESSAGE)-1+10];


#define SERIAL_DEVICE_PATH_TEST     SERIAL_DEVICE_PATH_DEAFULT




static bool test_write_read5( void* arg ){
	memset( buf, 0xab, sizeof(buf) );
	test( 2 == qread( m_fid, buf, 2 ) );
	test( 0 == memcmp( "12\xab\xab\xab\xab\xab\xab", buf, 8 ) );
	// if read again, it will retrive remain data
	memset( buf, 0xab, sizeof(buf) );
	test( 3 == qread( m_fid, buf, 3 ) );
	test( 0 == memcmp( "3ab\xab\xab\xab\xab\xab", buf, 8 ) );
	// if read again, it will retrive remain data
	memset( buf, 0xab, sizeof(buf) );
	test( 1 == qread( m_fid, buf, 3 ) );
	test( 0 == memcmp( "c\xab\xab\xab\xab\xab\xab\xab", buf, 8 ) );
	test( qclose( m_fid) );
    m_fid = 0;
	test( MS_TO_US( 3000 ) > elapse_us_close( elapse_id ) );

	return false;
}
static bool test_write_read4( void* arg ){
	{
		memset( buf, 0, sizeof(buf) );
		int nread = qread( m_fid, buf, sizeof(buf) );
		return_false_if( nread != 6 );
	}
	return_false_if( !qclose( m_fid ) );
    m_fid = 0;
	return_false_if( 0 != strcmp( (char*)buf, "123abc" ) );
	return_false_if( MS_TO_US( 2000 ) < elapse_us_close( elapse_id ) );

	// 测试buf长度不够情况
	return_false_if( 0 == ( elapse_id = elapse_us_open() ) );
	test( 0 != ( m_fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial ) ) );
	{
		const char message[] = "123abc";
		return_false_if( !serial_write( &serial, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	qos_task_sleep_us( 0, MS_TO_US( SEC_TO_MS( 1 ) ) );
    qos_task_set_entry( 0, test_write_read5, NULL );
    return true;
}

static bool test_task_write_read3( void* arg ){
	{
		const int nread = qread( m_fid, buf, sizeof(buf) );
		return_false_if( 1 != nread );
	}
	
	test( qclose( m_fid ) );
    m_fid = 0;
	test( buf[0] == 'x' );
	test( MS_TO_US( 2000 ) > elapse_us_close( elapse_id ) );


	// send a string
	test( 0 != ( elapse_id = elapse_us_open() ) );
	test( 0 != ( m_fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial ) ) );
	{
		const char message[] = "123abc";
        const int nbyte = strlen( message );
		return_false_if( nbyte != qwrite( m_fid, message, nbyte ) );
	}
	// sleep make sure linux os have receive all data
	test( qos_task_sleep_us( 0, MS_TO_US( SEC_TO_MS( 1 ) ) ) );
    test( qos_task_set_entry( 0, test_write_read4, NULL ) );
    return true;
}

static bool test_task_write_read2( void* arg ){
	test( 0 == qread( m_fid, buf, sizeof(buf) ) );
	test( qclose( m_fid ) );
    m_fid = 0;

	test( buf[0] == 'x' );
	test( MS_TO_US( 2000 ) > elapse_us_close( elapse_id ) );

	// send one char
	test( 0 != ( elapse_id = elapse_us_open() ) );
	test( 0 != ( m_fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial ) ) );
	{
		const char message[] = "x";
		test( qwrite( m_fid, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	test( qos_task_sleep_us( 0, MS_TO_US( SEC_TO_MS( 1 ) ) ) );
    test( qos_task_set_entry( 0, test_task_write_read3, NULL ) );
    return true;
}
static bool test_task_write_read1( void* arg ){
	{
		int n_read = qread( m_fid, buf, sizeof(buf) );
		test( data_len == n_read );
	}
	test( qclose( m_fid ) );
    m_fid = 0;
	test( MS_TO_US( 2000 ) > elapse_us_close( elapse_id ) );

	// send zero char
	memset( buf, 'x', sizeof(buf) );
	test( 0 != ( elapse_id = elapse_us_open() ) );
	test( 0 != ( m_fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial ) ) );
	{
		const char message[] = "";
		test( 0 == qwrite( m_fid, message, strlen((char*)message) ) );
	}
	// sleep make sure linux os have receive all data
	test( qos_task_sleep_us( 0, MS_TO_US( SEC_TO_MS( 1 ) ) ) );
    test( qos_task_set_entry( 0, test_task_write_read2, NULL ) );
    return true;
}
static bool test_task_write_read( void* arg ){
//	int timer_id = 0;

	uverify( sizeof(buf) > sizeof(WRITE_MESSAGE) );

	// 先测试时间特性

	// !!!ERROR!!! understand:
	// 发送完毕后，再接收，此时，串口电信号已经在
	// 线路上消失。这时，收到的字符数为0，
	// 说明：如果不调用read,linux并不会主动读取串口设备。
	// 也就是说：必须在发送方发送的同时，调用read函数进行接收。
	// correct understand:
	// linux OS will auto read serial port data and save data into system buffer, while you do NOT call read() in your code.
	test( 0 != ( elapse_id = elapse_us_open() ) );
	test( 0 != ( m_fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial ) ) );
	test( qwrite( m_fid, (char*)WRITE_MESSAGE, strlen(WRITE_MESSAGE) ) );
	// wait os read
	// other wise, when you call read(), 
	// it will return -1 and set errno = EAGAIN
	// it mean system buffer havn't data
    test( qos_task_sleep_us( 0, MS_TO_US( 100 ) ) );
    test( qos_task_set_entry( 0, test_task_write_read1, NULL ) );
    return true;
}
static int test_start_write_read(void){
    const int task_id = qos_idle_open( test_task_write_read, NULL );
    return_0_if( 0 == task_id );
    return_0_if( !qos_task_set_name( task_id, "test_task_write_read" ) );
    return task_id;
}
static bool test_write_read(void){
    test( ut_task_open( test_start_write_read ) );
    return true;
}



static bool test_open_close(void){
    test( serial_vfs_open() );

    serial_t serial = SERIAL_DEFAULT;
    int fid = qopen( SERIAL_DEVICE_PATH_TEST, 0, &serial );
    test( 0 != fid );
    test( qclose( fid ) );
    return true;
}


// 如果测试失败，请将串口硬件调整成loop back模式。即将2,3脚短接。

// unit test entry, add entry in file_list.h
bool unit_test_serial(void){
    // 本文件单元测试项表格
    const unit_test_item_t table[] = {
		test_open_close
//		, test_write_read
        , NULL              
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}


