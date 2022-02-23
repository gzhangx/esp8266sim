//	Program:	gtcpip.h
//	DESC:		implement tcpip protocal functions
//	Author:		Gang Zhang
//	E-mail:		gzhang_ocx@yahoo.com or gazhang@geocities.com
//	HTTP:		http://members.xoom.com/gzhang
//	Date:		Apr 16, 1998
//	Revisions:
//			July 16, 1998: add wait_file_rdy() 
//				to wait for a ready signal
//			Aug 27,1998: tcp_client now do a nonblock 
//				connect() call 
//			Aug 28,1998: add GANG_DLL_EXPORT,
//				#define GANG_DLL_EXPORT __declspec( dllexport )
//			and the whole class will be exported in dll
//			Feb 10,1999: protect m_sockfd with sem on close, get, set....
//				Note: didn't protect all of them (send....)


#ifndef gCTcpIpGANG_ZHANGHEADERFILE
#define gCTcpIpGANG_ZHANGHEADERFILE

#include <stdlib.h>
#include <time.h>
#include <winsock.h>
#include <string>
//#include "locksem.h"

#ifndef GANG_DLL_EXPORT
#define GANG_DLL_EXPORT
#endif



class GANG_DLL_EXPORT gCStreamFile{
public:
	virtual bool sendall(const unsigned char *buf,const int len)=0;
	virtual bool recvall(unsigned char *buf,const int len)=0;
};

class GANG_DLL_EXPORT gCTcpIp:public gCStreamFile{
protected:
	SOCKET m_sockfd;
	//gCLockSem m_socket_sem;	//protect against socket open and close

	static char proxy_addr[64];	//for proxy use
	static int proxy_port;

    static void raiseError(const char * msg);
public:
	gCTcpIp(SOCKET skt):m_sockfd(skt){
	}

	void operator=(gCTcpIp &tcp){
		//m_socket_sem.GWait();
		m_sockfd=tcp;
		//m_socket_sem.GSignal();
	}

	operator SOCKET(){
		//m_socket_sem.GWait();
		SOCKET skt=m_sockfd;
		m_sockfd=INVALID_SOCKET;
		//m_socket_sem.GSignal();
		return skt;
	}

	//transfer fp to other program, while not close it
	inline SOCKET transfer_fd(void){
		return (*this);
	}
	inline SOCKET get_fd(void){
		//m_socket_sem.GWait();
		SOCKET skt=m_sockfd;
		//m_socket_sem.GSignal();
		return skt;
	}
	inline void set_fd(SOCKET fp){
		//m_socket_sem.GWait();
		if(m_sockfd!=INVALID_SOCKET){
			closesocket(m_sockfd);
			m_sockfd=INVALID_SOCKET;
		}
		m_sockfd=fp;
		//m_socket_sem.GSignal();
	}

	BOOL valid() const {return m_sockfd!=INVALID_SOCKET;}

	gCTcpIp():m_sockfd(INVALID_SOCKET){
	}

	virtual ~gCTcpIp();

	static int de_init();
	//windows specific
	static void init(void);
	

	void set_socket_timeout(int timeout);

	void set_socket_linger(BOOL on,int timeout=6);

	inline int recv(char *buf,int len){
		return ::recv(m_sockfd,buf,len,0);
	}

	inline int recv(unsigned char *buf,int len){
		return recv((char*)buf,len);
	}

	inline int send(const unsigned char *buf,const int len){
		return ::send(m_sockfd,(const char*)buf,len,0);
	}

	inline int send(const char *buf,const int len){
		return ::send(m_sockfd,buf,len,0);
	}

	inline bool send_t(const char *buf,int len){
        if (::send(m_sockfd, buf, len, 0) < 0) {
            raiseError("Socket closed");
            return false;
        }
        return true;
	}

	inline bool send_t(unsigned char *buf,int len){
		return send_t((const char*)buf,len);
	}

	inline bool send_t(char *buf,int len){
		return send_t((const char*)buf,len);
	}

	inline int recv_t(char *buf,int len){
		int res= ::recv(m_sockfd,buf,len,0);
        if (res <= 0) {
            raiseError("Socket closed");
            return -1;
        }
		return res;
	}

	inline int recv_t(unsigned char *buf,int len){
		return recv_t((char *)buf,len);
	}

	inline void sendbyte(unsigned char thebyte)
	{
		send_t((const char*)&thebyte,1);
	}

	bool sendall(const unsigned char *buf,const int len);
	bool recvall(unsigned char *buf,const int len);

	inline bool sendall(const char *buf,const int len){
		return sendall((const unsigned char *)buf,len);
	}

	inline void recvall(char *buf,const int len){
		recvall((unsigned char*)buf,len);
	}


	bool gCTcpIp::recvall_t(char*buf,const int size,const int s,const int us=0);
	bool gCTcpIp::recvall_timeout_s(char*buf,const int size,const int timeout);

	bool sendstr(const char *buf);
	
	bool sendstr(char *buf);

	int recvstr(char *buf,int len);

	//struct hostent *name2addr(char *ip,const int iplen,const char *hostname);
  ///static std::string name2IpAddr(const std::string hostname);

	bool tcp_server(int *port,int backlog=5);

	bool tcp_server(int port,int backlog=5);

		//nblock=0 for block, =1 for block
	//return 0 on success
	int fd_block(u_long nblock);

	int file_write_rdy(int ts,int tms=0);
	
	int last_sock_error();
	
	bool tcp_client(const char *ip,const int port,int time_outs=10,int time_outus=0,u_long nblock=1,
	   long connection_break_tm=100000);


	void close(void);

	int wait_file_rdy();

	int file_rdy(void);

	int file_rdy(int ts,int tms=0);

	gCTcpIp accept_connection(void);
	gCTcpIp accept_connection(struct sockaddr_in *cli_addr);

	void get_peer_name(unsigned char *cip,unsigned short *port=NULL);

	void get_sock_name(unsigned char *cip,unsigned short *port=NULL);
};



class gTcpBuf:public gCTcpIp{
	int bufsize;
	char *tcpbuf;
	int numbytes;	//bytes available in tcpbuf
	int bytesout;	//bytes been got by use but not flashed out from buf yet
public:
	gTcpBuf(){
		numbytes=bytesout=0;

		bufsize=512;
		tcpbuf=new char[bufsize];		
	}
	
	BOOL set_size(int newsize){
		char *tmpbuf=new char[newsize];
		if(tmpbuf==NULL)return FALSE;
		delete tcpbuf;
		tcpbuf=tmpbuf;
		bufsize=newsize;
		return TRUE;
	}
	~gTcpBuf(){
		if(tcpbuf)delete tcpbuf;
	}

	int get_tcp_buf_size(){
		return numbytes-bytesout;
	}

	int bufd_recv(char *buf,int size){
		//clear out previous buffer content
		if(bytesout)numbytes-=bytesout;
        if (numbytes < 0) return -1;
		if(bytesout){
			for(int i=0;i<numbytes;i++){
				tcpbuf[i]=tcpbuf[bytesout+i];
			}
			bytesout=0;
		}
		//at this point tcpbuf contain only bytes that
		//has not go to the user yet

		//if required size is less than cached bytes
		if(size<=numbytes){
			memcpy(buf,tcpbuf,size);
			bytesout=size;
			return size;
		}

		//make sure we are not out of bound
		if(size>bufsize)size=bufsize;
		int recvbytes=recv(tcpbuf+numbytes,size-numbytes);
		if(recvbytes>0){
			numbytes+=recvbytes;
			bytesout=(size<=numbytes)?size:numbytes;
			memcpy(buf,tcpbuf,bytesout);
			return bytesout;
		}else if(numbytes){
			//on disconnect, flash the rest if any
			memcpy(buf,tcpbuf,numbytes);
			bytesout=numbytes;
			return numbytes;
		}
		return recvbytes;
	}
	int unrecv(int size){
		if(bytesout>=size){
			bytesout-=size;
			return size;
		}else{
			size=bytesout;
			bytesout=0;
		}
		return size;	//if size is too  big, return bytesout
				//see else part!!
	}
};

#endif

