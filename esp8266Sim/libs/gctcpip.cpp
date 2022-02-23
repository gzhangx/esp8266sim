//	Program:	gtcpip.cpp
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
//			Feb 19,1999: Seperated header and cpp file, add proxy support
//				proxy_init() for socks


#include "gtcpip.h"
#include <stdio.h>
#include <sstream>
#include "stdafx.h"
#pragma comment(lib, "Ws2_32.lib")

void gCTcpIp::raiseError(const char * msg) {
    printf(msg);
}


	gCTcpIp::~gCTcpIp(){
		close();
	}

	int gCTcpIp::de_init(){
		return WSACleanup();
	}
	//windows specific
	void gCTcpIp::init(void){
		WORD wVersionRequested; 
		WSADATA wsaData; 
		wVersionRequested = MAKEWORD(1, 1); 
		if(WSAStartup(wVersionRequested, &wsaData)!=0)
            raiseError("Can't init windows socket");
	}

	void gCTcpIp::set_socket_timeout(int timeout){
		if(setsockopt(m_sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout))!=0){
            raiseError("Can't set socket timeout opt");
		}
	}

	void gCTcpIp::set_socket_linger(BOOL on,int timeout){
		struct linger lign;
		lign.l_onoff=on;
		//0 for implement depend, default to 6 seconds, 
		lign.l_linger=timeout;
		if(setsockopt(m_sockfd,SOL_SOCKET ,SO_LINGER ,(char*)&lign,sizeof(lign))!=0){
            raiseError("Set linger failed");
		}
	}


    bool gCTcpIp::tcp_client(const char *serv_ip, int port, int time_outs, int time_outus, u_long nblock,
        long connection_break_tm) {
        struct sockaddr_in srv_addr;
        struct sockaddr_in cli_addr;
        struct hostent *host;
        int err;

        if ((m_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
            raiseError("Can't open stream socket");
            return false;
        }

        memset(&srv_addr, 0, sizeof(srv_addr));
        srv_addr.sin_family = AF_INET;

        if ((srv_addr.sin_addr.s_addr = inet_addr(serv_ip)) == INADDR_NONE) {
            host = gethostbyname(serv_ip);

            if (!host) {
                close();
                raiseError("Error get host name for ");
                return false;
            }
            //memcpy(&(srv_addr->sin_addr), host->h_addr, host->h_length);
            srv_addr.sin_addr = *(struct in_addr*)(host->h_addr_list[0]);
            srv_addr.sin_family = host->h_addrtype;
        }

        srv_addr.sin_port = htons(port);

        memset((char*)&cli_addr, 0, sizeof(cli_addr));
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_port = htons(0);

        err = bind(m_sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        if (err<0) {
            //err=last_sock_error();
            close();
            raiseError("Can't bind local address");
            return false;
        }

        err = fd_block(nblock);
        if (err<0) {
            //err=last_sock_error();
            close();
            raiseError("Can't set to non block mode");
            return false;
        }
        err = connect(m_sockfd, (sockaddr *)&srv_addr, sizeof(srv_addr));

        if (nblock == 0) {
            if (err != 0) {
                //err=last_sock_error();
                close();
                raiseError("Can't connect to ");
                return false;
            }
        }
        else {
            if (err != 0) {
                err = last_sock_error();
                if (err != WSAEWOULDBLOCK) {
                    close();
                    //throw gTcp_error_str("Can't connect to ", ERR_SOCKET_CONNECT) + gTcp_error_str(serv_ip);
                    raiseError("can't connect");
                    return false;
                }
            }
            nblock = 0;
            err = ioctlsocket(m_sockfd, FIONBIO, &nblock);
            if (err<0) {
                err = last_sock_error();
                close();
                raiseError("Can't set blocking mode");
                return false;
            }
            do {
                //becaurefull file_write_rdy(s,us), and connection_break_tm is ms
                err = file_write_rdy(0, connection_break_tm);
                if (err != 0)break;
                if (time_outus>0)
                    time_outus -= connection_break_tm;
                else {
                    while (time_outus<0) {
                        time_outs--;
                        time_outus += 1000000;
                        time_outus -= connection_break_tm;
                        if (time_outs<0)break;
                    }
                }
                if (time_outs<0 && time_outus<0) {
                    close();
                    //throw gTcp_error_str("Connect to ") + gTcp_error_str(serv_ip) + gTcp_error_str(" timeout");
                    raiseError("connection timeout");
                    return false;
                }
            } while (1);
            if (err<0) {
                err = last_sock_error();
                close();
                //throw gTcp_error_str("Can't connect to ", ERR_SOCKET_CONNECTTIMEOUT) + gTcp_error_str(serv_ip);
                raiseError("Unable to connect");
                return false;
            }
        }
        return true;

    }



	bool gCTcpIp::sendall(const unsigned char *buf,const int len){
		int res;
		int left=len;
		while(left>0){
			res=send(buf,left);
			//if(res<0)return res;
            if (res < 0) {
                //throw gTcp_error_str("Socket closed", ERR_SOCKET_CLOSED);
                raiseError("sendAll socket closed");
                return false;
            }
			left-=res;
			buf+=res;
		}
        return true;
	}



	bool gCTcpIp::recvall(unsigned char *buf,const int len){
		int res;
		int left=len;
		while(left>0){
			res=recv(buf,left);
            if (res <= 0)	//return res;
            {
                //throw gTcp_error_str("Socket closed",ERR_SOCKET_CLOSED);
                raiseError("receAll socket closed");
                return false;
            }
			left-=res;
			buf+=res;
		}
        return true;
	}


	bool gCTcpIp::recvall_timeout_s(char*buf,const int size,const int timeout){
		int left,writen;
		time_t when,now;
		time(&when);
		left=size;
		while(left>0){
			if(file_rdy(timeout)>0){
				writen=recv(buf,left);
				if(writen<=0){
					//throw gTcp_error_str("Socket err on recv",ERR_SOCKET_CLOSED);
                    raiseError("receAll socket receive error");
                    return false;
				}
				left-=writen;
				buf+=writen;
				if(left==0)break;
				time(&now);
				if( (now-when)>timeout ){
					//throw gTcp_error_str("socket recv timeout",ERR_SOCKET_TIMEOUT);
                    raiseError("receAll timeout");
                    return false;
				}
			}else{
				//throw gTcp_error_str("socket recv timeout",ERR_SOCKET_TIMEOUT);
                raiseError("receAll timeout");
                return false;
			}
		}
        return true;
	}
	
	bool gCTcpIp::recvall_t(char*buf,const int size,const int s,int us){
		int left,writen;
		left=size;
		while(left>0){
			if(file_rdy(s,us)>0){
				writen=recv(buf,left);
				if(writen<=0){
					//throw gTcp_error_str("Socket err on recv",ERR_SOCKET_CLOSED);
                    raiseError("recvAll: can't receive");
                    return false;
				}
				left-=writen;
				buf+=writen;
			}else{
				//throw gTcp_error_str("Socket recv timeout",ERR_SOCKET_TIMEOUT);
                raiseError("Socket recvTimeout");
                return false;
			}
		}
        return true;
	}

	bool gCTcpIp::sendstr(const char *buf){
		return sendall(buf,(int)strlen(buf));
	}
	
	bool gCTcpIp::sendstr(char *buf){
		return sendstr((const char*)buf);
	}

	int gCTcpIp::recvstr(char *buf,int len){
		len=recv(buf,len);
		if(len<=0){
			buf[0]=0;
			raiseError("Socket closed");
            return len;
		}
		buf[len]=0;
		return len;
	}


    


	bool gCTcpIp::tcp_server(int *port,int backlog){
		struct sockaddr_in srv_addr;

		memset((char*)&srv_addr,0, sizeof(srv_addr));
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_port     = htons((short)*port);
		srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

		if ((m_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
		{
			//throw gTcp_error_str("Error get socket",ERR_SOCKET_ALLOC);
			//return -1;
            raiseError("Server: error get socket");
            return false;
		}	

		if (bind(m_sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) != 0) {
			//throw gTcp_error_str("Error bind",ERR_SOCKET_BIND);
			//return -2;
            raiseError("Server: error bind");
            return false;
		}

		if(*port==0){
			int len=sizeof(srv_addr);
			if(getsockname(m_sockfd,(struct sockaddr*)&srv_addr,&len)==0){
				*port=ntohs(srv_addr.sin_port);
            }
            else {
                //throw gTcp_error_str("Init socket: Error get port id", ERR_SOCKET_PORT_ALLOC);
                raiseError("Server: error get port id");
                return false;
            }
		}

		if (listen(m_sockfd, backlog) !=0) {
			//throw gTcp_error_str("Error listen",ERR_SOCKET_LISTEN);
			//return -3;
            raiseError("Server: error listen");
            return false;
		}
        return true;
	}

	bool gCTcpIp::tcp_server(int port,int backlog){
		return tcp_server(&port,backlog);
	}

		//nblock=0 for block, =1 for block
	//return 0 on success
	int gCTcpIp::fd_block(u_long nblock){
		return ioctlsocket (m_sockfd,FIONBIO, &nblock );
	}

	int gCTcpIp::file_write_rdy(int ts,int tus){
		fd_set fdset;
		struct timeval tout;
		tout.tv_sec=ts;
		tout.tv_usec=tus;
		FD_ZERO(&fdset);
		FD_SET(m_sockfd, &fdset);
		return select(FD_SETSIZE,NULL,&fdset,NULL,&tout);
	}
	
	int gCTcpIp::last_sock_error(){
		return WSAGetLastError();
	}

	void gCTcpIp::close(void){
		//m_socket_sem.GWait();
		if(m_sockfd!=INVALID_SOCKET){
			closesocket(m_sockfd);
			m_sockfd=INVALID_SOCKET;
		}
		//m_socket_sem.GSignal();
	}

	int gCTcpIp::wait_file_rdy(){
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(m_sockfd, &fdset);
		return select(FD_SETSIZE,&fdset,NULL,NULL,NULL);
	}

	int gCTcpIp::file_rdy(void){
		fd_set fdset;
		struct timeval tout;
		tout.tv_sec=0;
		tout.tv_usec=0;
		FD_ZERO(&fdset);
		FD_SET(m_sockfd, &fdset);
		return select(FD_SETSIZE,&fdset,NULL,NULL,&tout);
	}

	int gCTcpIp::file_rdy(int ts,int tms){
		fd_set fdset;
		struct timeval tout;
		tout.tv_sec=ts;
		tout.tv_usec=tms;
		FD_ZERO(&fdset);
		FD_SET(m_sockfd, &fdset);
		return select(FD_SETSIZE,&fdset,NULL,NULL,&tout);
	}

	gCTcpIp gCTcpIp::accept_connection(void){
		struct sockaddr_in cli_addr;
  		int len = sizeof(cli_addr);
  		cli_addr.sin_family = AF_INET;
  		return gCTcpIp(accept(m_sockfd, (struct sockaddr*)&cli_addr, &len));
	}
	gCTcpIp gCTcpIp::accept_connection(struct sockaddr_in *cli_addr){
  		int len = sizeof(*cli_addr);
  		cli_addr->sin_family = AF_INET;
  		return gCTcpIp(accept(m_sockfd, (struct sockaddr*)cli_addr, &len));
	}
    /*
	void gCTcpIp::get_peer_name(unsigned char *cip,unsigned short *port){
		struct sockaddr_in addr;
		int len=sizeof(struct sockaddr_in);
		if(getpeername (m_sockfd,(struct sockaddr *)&addr,&len )!=0){
			throw gTcp_error_str("Can't get peer name");
		}
		*(cip  )=addr.sin_addr.S_un.S_un_b.s_b1;
		*(cip+1)=addr.sin_addr.S_un.S_un_b.s_b2;
		*(cip+2)=addr.sin_addr.S_un.S_un_b.s_b3;
		*(cip+3)=addr.sin_addr.S_un.S_un_b.s_b4;
		if(port)*port=addr.sin_port;
	}

	void gCTcpIp::get_sock_name(unsigned char *cip,unsigned short *port){
		struct sockaddr_in addr;
		int len=sizeof(struct sockaddr_in);
		if(getsockname (m_sockfd,(struct sockaddr *)&addr,&len )!=0){
			throw gTcp_error_str("Can't get sock name");
		}
		*(cip  )=addr.sin_addr.S_un.S_un_b.s_b1;
		*(cip+1)=addr.sin_addr.S_un.S_un_b.s_b2;
		*(cip+2)=addr.sin_addr.S_un.S_un_b.s_b3;
		*(cip+3)=addr.sin_addr.S_un.S_un_b.s_b4;
		if(port)*port=addr.sin_port;
	}

    */