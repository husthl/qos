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
	Linux�µĴ��ڱ��(���� 

2	http://learn.akae.cn/media/ch28s04.html
	�����Ľ�����O_NONBLOCK �� EAGAIN �÷�
*/




#include <stdio.h>  
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>      /*�ļ����ƶ���*/
#include <errno.h>      /*����Ŷ���*/
#include <unistd.h>
#include <termios.h>    /*PPSIX�ն˿��ƶ���*/

#include "inc.h"
#include "vfs.h"
#include "serial.h"
#include "qmem.h"

#define this_file_id   file_id_serial




// �����������֮ǰ����ȷ����serial_t�ṹ��
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
* ���ƣ�                UART0_Set
* ���ܣ�                ���ô�������λ��ֹͣλ��Ч��λ
* ��ڲ�����        fd         �����ļ�������
*                              speed      �����ٶ�
*                              flow_ctrl  ����������
*                           databits   ����λ   ȡֵΪ 7 ����8
*                           stopbits   ֹͣλ   ȡֵΪ 1 ����2
*                           parity     Ч������ ȡֵΪN,E,O,,S
*���ڲ�����              ��ȷ����Ϊ1�����󷵻�Ϊ0
*******************************************************************/
// please read: 
// 1    http://blog.163.com/shaohj_1999@126/blog/static/63406851201101410739851/
//      ���ڱ��linux����������������  
// 2    http://tldp.org/HOWTO/html_single/Serial-HOWTO/#ss3.3
//      Serial HOWTO
//      very good document about linux serial
static int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity){
	int i;
	const int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
		B38400, B19200, B9600, B4800, B2400, B1200, B300 };
	int   name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,      38400,  19200,  9600, 4800, 2400, 1200,  300 };
	struct termios options;

	// tcgetattr(fd,&options)�õ���fdָ��������ز������������Ǳ�����options,�ú���,
	// �����Բ��������Ƿ���ȷ���ô����Ƿ���õȡ������óɹ�����������ֵΪ0��������ʧ�ܣ���������ֵΪ1.
	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");    
		return false; 
	}

	//���ô������벨���ʺ����������
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++){
		if  (speed == name_arr[i]){       
			cfsetispeed( &options, speed_arr[i] ); 
			cfsetospeed( &options, speed_arr[i] );
		}
	}     

	// ������ǿ����ն�֮��ģ�ֻ�Ǵ��ڴ������ݣ�������Ҫ������������ôʹ��ԭʼģʽ(Raw Mode)��ʽ��ͨѶ�����÷�ʽ���£�
	// �����ǹ���raw mode�Ľ��ܣ�
	// �ն˼��
    // ��ʷ����ʵ�� terminal ��ʵ����һ�� "mon + keyborad + �����ڴ� + cable" ���豸, ���߱�ʵ�ʵļ��㹦��, "tty" ����ʱ����� teletype (�紫���ֻ�)����˼. ����������õĶ��� emulattion. (ref http://tldp.org/HOWTO/Text-Terminal-HOWTO-1.html#ss1.7) 
	// �ն�IO
	// ����
    // �û������Լ�ʵ�ʵ��ն�����֮�������һ���ն��й��( terminal line discipline ), ���ն˵�������Ⱦ����˶���Ž����û�����(���ն˵����Ҳ���Ⱦ�����), ���ģ���ڲ�ͬ��ģʽ�»������������в�ͬ�Ĵ���(�����ڹ淶ģʽ��, �жϳ��û�������� Ctrl+C, ��˸��û����̷���һ���ж��ź�, �û������޷���ô��ַ�; ����ԭʼģʽ��, �û���������Ի�ô��ַ�).
    // ģʽʵ������һ���ն��豸���������, ��Щ���Կ�ͨ������ termios �ṹ�ı�־λȻ��ͨ�� tcgetattr/tcsetattr .. ���ն�IO�������п���.
    // ���õ�ģʽ������
    //    ����Ϊ��λ��, �������ַ����д����ģʽ (cooked mode)
    //    ԭʼģʽ, ������Ϊ��λ, ���������ַ����д��� (raw mode)
    //    cbreak ģʽ, ����ԭʼģʽ, ��ĳЩ�����ַ�Ҳ���д��� 
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   /*Output*/

	//�޸Ŀ���ģʽ����֤���򲻻�ռ�ô���
	options.c_cflag |= CLOCAL;
	//�޸Ŀ���ģʽ��ʹ���ܹ��Ӵ����ж�ȡ��������
	options.c_cflag |= CREAD;

	//��������������
	switch(flow_ctrl){
		case 0 ://��ʹ��������
			options.c_cflag &= ~CRTSCTS;
			break;   
		case 1 ://ʹ��Ӳ��������
			options.c_cflag |= CRTSCTS;
			break;
		case 2 ://ʹ�����������
			options.c_cflag |= IXON | IXOFF | IXANY;
			break;
	}

	//��������λ
	options.c_cflag &= ~CSIZE; //����������־λ
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

	//����У��λ
	switch (parity){  
		case 'n':
		case 'N': //����żУ��λ��
			options.c_cflag &= ~PARENB; 
			options.c_iflag &= ~INPCK;    
			break; 
		case 'o':  
		case 'O'://����Ϊ��У��    
			options.c_cflag |= (PARODD | PARENB); 
			options.c_iflag |= INPCK;             
			break; 
		case 'e': 
		case 'E'://����ΪżУ��  
			options.c_cflag |= PARENB;       
			options.c_cflag &= ~PARODD;       
			options.c_iflag |= INPCK;       
			break;
		case 's':
		case 'S': //����Ϊ�ո� 
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break; 
		default:  
			fprintf(stderr,"Unsupported parity/n");   
			return (false); 
	} 

	// ����ֹͣλ 
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

	//�޸����ģʽ��ԭʼ�������
	options.c_oflag &= ~OPOST;

	//���õȴ�ʱ�����С�����ַ�
	//options.c_cc[VTIME] = 1; /* ��ȡһ���ַ��ȴ�1*(1/10)s */  
    // because I use qos, so NO function will block in function.
	options.c_cc[VTIME] = 0; /* ��ȡһ���ַ��ȴ�1*(1/10)s */  
	options.c_cc[VMIN] = 0; /* ��ȡ�ַ������ٸ���Ϊ1 */


	//added by tianjin 2011-11-24,this is error,in such case,linux serial will do some change on the byte we receive,for example,it will turn 0x0d to 0x0a 
	//put the config at the place because it will not be recovered by other config
	options.c_iflag &= ~ICRNL;
	options.c_iflag &= ~IXON;
	
    fcntl(fd, F_SETFL, FNDELAY); //������
    //fcntl(fd, F_SETFL, 0); // ����

	//�����������������������ݣ����ǲ��ٶ�ȡ
	tcflush(fd,TCIFLUSH);

	//�������� (���޸ĺ��termios�������õ������У�
	if (tcsetattr(fd,TCSANOW,&options) != 0){
		perror("com set error!/n");  
		return (false); 
	}

	return (true); 
}




/*
********************************************************************************
* ��  ��: ���ô����ٶ�.
* ��  �أ�0 �ɹ�, -1ʧ��
*
********************************************************************************
*/
int SetSpeed( int fd, int speed ){
	int i;
	int status;
	struct termios Opt;
    int baud;

	const int baud_table[8][2] = {
		{115200, B115200},  //zhp ��ǰΪʲôû��������壿���ڼ��ϣ�������������������ٶȡ�
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

	/* ��������ߣ������µĴ������� */
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
* ��  ��: ���ô�������λ��ֹͣλ��Ч��λ.
********************************************************************************
*/
int SetParity(
	int fd,				/* �򿪵Ĵ����ļ���� */
	int databits,		/* ����λ   ȡֵ Ϊ 7 ����8 */
	int stopbits,		/* ֹͣλ   ȡֵΪ 1 ����2 */
	int parity			/* Ч������ ȡֵΪN,E,O,,S */
){
	struct termios options;

	if (tcgetattr(fd,&options) != 0)
	{
		perror("SetupSerial 1");
		return(false);
	}

#if 1	/* ����Ҫ�س����������� */
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  //Input
	options.c_oflag  &= ~OPOST;   //Output
#endif

	options.c_cflag &= ~CSIZE;
	switch (databits) /*��������λ��*/
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
			options.c_cflag |= (PARODD | PARENB);  /* ����Ϊ��Ч��*/
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;   /* ת��ΪżЧ��*/
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

	/* ����ֹͣλ*/
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

	options.c_cc[VTIME] = 1; // 100 ms ��ʱ�ط�
	options.c_cc[VMIN] = 0;


    options.c_iflag &= ~(ICRNL | INLCR);//zhp���E R�ļ�ֵ�е�0D���Զ�ת��0A�����⡣        
	options.c_iflag &= ~(IXOFF | IXON);//zhp���Y U�ļ�ֵ�е�11/13��ȥ�������⡣
	//���������仰���Ϻ�ͽ���˵��ֽڵ����⣬���������ε���Ҳ���������ˡ���֣�������������֮���������£�˵�����������򿪡�

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
* ��  ��: �򿪴���.
********************************************************************************
*/
static int OpenDev( char const *dev ){
	// O_NOCTTY:��ʾ�򿪵���һ���ն��豸�����򲻻��Ϊ�ö˿ڵĿ����նˡ������ʹ�ô˱�־������һ������(eg:������ֹ�źŵ�)����Ӱ����̡�
	// O_NDELAY:��ʾ������DCD�ź���������״̬���˿ڵ���һ���Ƿ񼤻����ֹͣ����
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
* ������: WaitSerialData
*
* ��  ��: �ȴ���������
*
* ��  �룺N/A
*
* ��  ����N/A
*
* ��  �أ�	0 - ������
*			���� - ������
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

	fd_set rdfds;			/* ������һ�� fd_set ��������������Ҫ���� socket��� */
	struct timeval tv;		/* ����һ��ʱ�����������ʱ�� */
	int ret;				/* ���淵��ֵ */
	int fd;


	FD_ZERO(&rdfds); 		/* ��select����֮ǰ�ȰѼ������� */
	FD_SET(fd, &rdfds);		/* ��Ҫ���ľ��socket���뵽������ */

	tv.tv_sec = _iTimeOut;
	tv.tv_usec = 0;			/* ����select�ȴ������ʱ��Ϊ2�� */

	/* ��������������õ�����rdfds��ľ���Ƿ��пɶ���Ϣ */

	ret = select(fd + 1, &rdfds, 0, (fd_set*)0, &tv);

	if (ret < 0)
	{
		//perror("select");/* ��˵��select�������� */
		return -1;
	}
	else if(ret == 0)
	{
		/* ˵���������趨��ʱ��ֵ1���500�����ʱ���ڣ�socket��״̬û�з����仯 */
		return -1;
	}
	else
	{
		/* ˵���ȴ�ʱ�仹δ��1���500���룬socket��״̬�����˱仯 */
		/* ret�������ֵ��¼�˷���״̬�仯�ľ������Ŀ����������ֻ������socket��һ���������������һ��ret=1��
		���ͬʱ�ж����������仯���صľ��Ǿ�����ܺ��� */
	    /* �������Ǿ�Ӧ�ô�socket���������ȡ�����ˣ���Ϊselect�����Ѿ����������������������ݿɶ� */
	    if (FD_ISSET(fd, &rdfds))
	    {
	        /* ���Զ�ȡ���� */
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
* ��  ��: �Ӵ��ڻ��һ���ֽ�����
* ��  �أ�	1 - ������
*			0 - ������
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
* ��  ��: �Ӵ��ڻ��һ���ַ�������
* ��  �룺s���ַ���ָ�룬len������
* ��  �أ�	len - ������
*			0 - ������
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
* ��  ��: �򴮿�дһ���ַ�������
* ��  �룺s���ַ���ָ�룬len������
* ��  �أ�	s - ������
*			NULL - ������
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
* ��  ��: �򴮿ڷ���һ������.
* ��  �أ�	0 - ������
*			���� - ������
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
	
	// ����˵�� read()��Ѳ���fd ��ָ���ļ�����count���ֽڵ�bufָ����ָ���ڴ��С�������countΪ0����read()���������ò�����0������ֵΪʵ�ʶ�ȡ�����ֽ������������0����ʾ�ѵ����ļ�β�����޿ɶ�ȡ�����ݣ������ļ���дλ�û����ȡ�����ֽ��ƶ���
	// ����˵�� ���˳��read()�᷵��ʵ�ʶ������ֽ���������ܽ�����ֵ�����count ���Ƚϣ������ص��ֽ�����Ҫ���ȡ���ֽ����٣����п��ܶ������ļ�β���ӹܵ�(pipe)���ն˻���ȡ��������read()���ź��ж��˶�ȡ���������д�����ʱ�򷵻�-1������������errno�У����ļ���дλ�����޷�Ԥ�ڡ�
	// ������� 
	// EINTR �˵��ñ��ź����жϡ�
	// EAGAIN ��ʹ�ò������I/O ʱ��O_NONBLOCK�����������ݿɶ�ȡ�򷵻ش�ֵ��
	// EBADF ����fd ����Ч���ļ������ʣ�����ļ��ѹرա�
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



// �����������Ŀ��Ϊ���������մ������ݣ�ֱ�������豸�������ˣ����߻��������ˡ�
// �������˳��������£�
// 1	�ȴ���һ���ַ�������first_char_timeout_ms��
//		�����������豸�ܳ�ʱ�䣬һ���ַ�û��Ӧ��
// 2	�����ַ�����buf_size��������������
// 3	�������һ���ַ�������interval_char_timeout_ms��
//		�����������豸��������һЩ�ַ���û�ж�������ݷ����ˡ�
//		���������ַ�֮��ļ��ʱ�䣬������interval_char_timeout_ms.(��������Ϊ���ڷ����豸����)
//		Ҳ�������Ϊ�������ַ�֮��ʱ��������interval_char_timeout_ms���ͷ���
// return: the byte number write in buf
int serial_read_timeout( serial_t* serial, char* buf, int buf_size, int first_char_timeout_ms, int interval_char_timeout_ms ){
	int rx_cnt = 0;

	rx_cnt = serial_read_timeout_first_char( serial, buf, buf_size, first_char_timeout_ms );
	if( 0 == rx_cnt ){
		// �����������豸�ܳ�ʱ�䣬һ���ַ�û��Ӧ��
		return rx_cnt;
	}
	if( rx_cnt >= buf_size ){
		// ��������������
		uverify( rx_cnt == buf_size );
		return rx_cnt;
	}

	// �Ѿ����յ�һ�����ַ�����������
	while( true ){
		char* const p = buf + rx_cnt;
		const int buf_room = buf_size - rx_cnt;
		const int rx_cnt_loop = serial_read_timeout_first_char( serial, p, buf_room, interval_char_timeout_ms );

		if( 0 == rx_cnt_loop ){
			// �����������豸��������һЩ�ַ���û�ж�������ݷ����ˡ�
			// ���������ַ�֮��ļ��ʱ�䣬������interval_char_timeout_ms.
			return rx_cnt;
		}

		rx_cnt += rx_cnt_loop;
		if( rx_cnt >= buf_size ){
			// ��������������
			uverify( rx_cnt == buf_size );
			return rx_cnt;
		}
	}

	uverify( false );
	return 0;
}




// return: true:	There are something for reading.
static bool wait_fd_have_something_read( int fd, int first_char_timeout_ms ){
	fd_set rdfds;			// ������һ�� fd_set ��������������Ҫ���� socket���
	FD_ZERO( &rdfds ); 		// ��select����֮ǰ�ȰѼ������� 
	FD_SET(fd, &rdfds);		// ��Ҫ���ľ��socket���뵽������ 

	// ����select�ȴ������ʱ��
	struct timeval tv;		// ����һ��ʱ�����������ʱ��
	tv.tv_sec = 0;
	tv.tv_usec = MS_TO_US( first_char_timeout_ms );			

	// ��������������õ�����rdfds��ľ���Ƿ��пɶ���Ϣ
	// Select��Socket����л��ǱȽ���Ҫ�ģ����Ƕ��ڳ�ѧSocket������˵����̫����Selectд��������ֻ��ϰ��д����connect��accept��recv��recvfrom����������������ν������ʽblock������˼�壬���ǽ��̻����߳�ִ�е���Щ����ʱ����ȴ�ĳ���¼��ķ���������¼�û�з��������̻��߳̾ͱ����������������������أ�������ʹ��Select�Ϳ�����ɷ���������ν��������ʽnon-block�����ǽ��̻��߳�ִ�д˺���ʱ���ط�Ҫ�ȴ��¼��ķ�����һ��ִ�п϶����أ��Է���ֵ�Ĳ�ͬ����ӳ������ִ�����������¼���������������ʽ��ͬ�����¼�û�з����򷵻�һ����������֪�¼�δ�����������̻��̼߳���ִ�У�����Ч�ʽϸߣ���ʽ�����ĳ������ܹ�����������Ҫ���ӵ��ļ��������ı仯���??????��д�����쳣��������ϸ����һ�£�
	// ���淵��ֵ
	while(1){
		const int select_ret = select( fd + 1, &rdfds, 0, (fd_set*)0, &tv );

		if( 0 < select_ret ){
			// 0 < select_ret
			uverify( 0 < select_ret );
			// ˵���ȴ�ʱ�仹δ����ʱ��socket��״̬�����˱仯 
			// ret�������ֵ��¼�˷���״̬�仯�ľ������Ŀ��
			// ��������ֻ������socket��һ���������������һ��ret=1��
			// ���ͬʱ�ж����������仯���صľ��Ǿ�����ܺ��� 
			uverify( 1 == select_ret );
			
			// �������Ǿ�Ӧ�ô�socket���������ȡ�����ˣ�
			// ��Ϊselect�����Ѿ����������������������ݿɶ� 
			if( !FD_ISSET( fd, &rdfds ) ){
				uverify( false );
				return false;
			}

			return true;
		}

		if( 0 == select_ret ){
			// ��ʱ
			// ˵���������趨��ʱ��ֵ�ڣ�socket��״̬û�з����仯
			return false;
		}

		if( 0 > select_ret ){
			// ��˵��select��������
			// dodo: huanglin 20111205: ����power -ulc���������ж���������
			// ��ctrl-cʱ��ż�������²��Գ�������һ�д���
			// ����ԭ����Ϊ���ھ���ڰ�ctrl-cʱ���Ѿ�����Ӧ�ó���ر�
			// re: huanglin 20111205: please read below topic:
			//		signal SA_RESTART
			//		select EINTR
			//
			// EINTR �˵��ñ��ź����жϡ�
			// ���磺����ctrl-c
			if( EINTR == errno ){
				// �ٵȴ�һ��
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




// ���ȴ�first_char_timeout_ms�������һ���ַ���û���յ����򷵻�
// ����У��������������������ˣ�����������
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

	// ���Զ�ȡ����
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

	// ����buf���Ȳ������
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

	// �Ȳ���ʱ������

	// !!!ERROR!!! understand:
	// ������Ϻ��ٽ��գ���ʱ�����ڵ��ź��Ѿ���
	// ��·����ʧ����ʱ���յ����ַ���Ϊ0��
	// ˵�������������read,linux������������ȡ�����豸��
	// Ҳ����˵�������ڷ��ͷ����͵�ͬʱ������read�������н��ա�
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


// �������ʧ�ܣ��뽫����Ӳ��������loop backģʽ������2,3�Ŷ̽ӡ�

// unit test entry, add entry in file_list.h
bool unit_test_serial(void){
    // ���ļ���Ԫ��������
    const unit_test_item_t table[] = {
		test_open_close
//		, test_write_read
        , NULL              
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}


