// MeUdp.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <winsock.h>   
//1
//2


#include "./include/ASMeSDK.h"

#define   RECVLEN  128 // 接收缓冲区大小
#define   RETLEN   10
char buf[RECVLEN];        // 接收缓冲区

#pragma comment(lib, "ws2_32.lib") 

#define SERVERPORT 9200 //服务端口号 


char type;
BOOL bOpen;
DWORD dwMask = 0;
char ipaddr[15];

HANDLE m_hController;//控制器指针，用于返回打开的控制器句柄，供后续开关门接口使用

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA   wWsadata; 
	WORD   wWord; 
	wWord=MAKEWORD(1,1); 
	if(!WSAStartup(wWord,&wWsadata))
	{
		//创建SOCKET
		SOCKET sock;
		if ( (sock = socket(AF_INET, SOCK_DGRAM ,0 /*IPPROTO_UDP*/)) != INVALID_SOCKET )
		{
			printf("start working...\n");
			printf("socket port 9200\n");
			//在本地地址与端口绑定 
			SOCKADDR_IN   addr; 
			addr.sin_addr.s_addr   =   htonl   (INADDR_ANY); 
			addr.sin_family   =   AF_INET; 
			addr.sin_port   =   htons(SERVERPORT); 
			// 绑定
			bind(sock,   (SOCKADDR   *)&addr,sizeof(SOCKADDR));   
			//接收数据 
			SOCKADDR_IN   addrClient;    // 存储有发送端的地址
			int   len=sizeof(SOCKADDR); 
			int   a, n; 
			fd_set   readfds; 
			timeval   timeout={5,0};    // 超时时间为5秒,后面的那个数是微秒值
			while(1)
			{ 
				FD_ZERO(&readfds);            
				FD_SET(sock,&readfds);        
				n   =   select(0,   &readfds,   NULL,   NULL,   &timeout);//接收数据 
				if(n > 0)
				{ 
					a=recvfrom(sock,buf,RECVLEN,0,(SOCKADDR*)&addrClient,&len); 
					printf( "recv %s ,len=%d \n",buf,a);
					if(a   ==   SOCKET_ERROR) //错误
						printf("*** ERROR.  CODE =%d\n",WSAGetLastError());
					else//正确
					{
						type = buf[0];
						dwMask = 0;
						switch(buf[2])
						{
						case '1':
							dwMask  |= 1;break;
						case '2':
							dwMask  |= 2;break;
						case '3':
							dwMask  |= 4;break;
						case '4':
							dwMask  |= 8;break;
						}
						bOpen = (buf[1] == '1')?true:false;

						strcpy(ipaddr,(char *)&buf[3]);  //将数据读入结构体对象

						if(type == '0')//0标示是开关门操作
						{
							printf("%s  open/close door  %s\n", ipaddr,inet_ntoa(addrClient.sin_addr));
							int nRes;
							AS_ME_COMM_ADDRESS com_address;
							com_address.nType = AS_ME_COMM_TYPE_IPV4;
							DWORD dwIP=ntohl(inet_addr(ipaddr)); 
							printf("dwIP:%x", dwIP);
							com_address.RealParam.IPV4.dwIPAddress = dwIP;
							com_address.RealParam.IPV4.wServicePort = (WORD)50000;
							nRes = AS_ME_OpenController(AS_ME_TYPE_UNKOWN,&com_address,AS_ME_NET_ADDRESS,AS_ME_NO_PASSWORD,NULL,&m_hController);
							if(nRes < 0)//Error Return
							{
								char chRet[RETLEN]={0};
								sprintf(chRet, "%d", nRes);
								printf("AS_ME_OpenController ERROR.  CODE =%d\n",nRes);
								sendto(sock,chRet, strlen(chRet)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
							}
							else
							{
								nRes = AS_ME_ToggleDoorState(m_hController,bOpen, dwMask);
								if (nRes < 0)//Error Return
								{
									char chRet[RETLEN]={0};
									sprintf(chRet, "%d", nRes);
									printf("AS_ME_OpenDoor ERROR.  CODE =%d\n",nRes);
									sendto(sock,chRet, strlen(chRet)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
								}
								else
								{
									if (m_hController != NULL)
									{
										AS_ME_CloseController(m_hController);
									}

								}
							}

						}
						else if(type == '1')//获取状态请求
						{
							printf("%s  get state  %s\n", ipaddr,inet_ntoa(addrClient.sin_addr));
							int nRes;
							AS_ME_COMM_ADDRESS com_address;
							com_address.nType = AS_ME_COMM_TYPE_IPV4;
							DWORD dwIP=ntohl(inet_addr(ipaddr)); 
							printf("dwIP:%x", dwIP);
							com_address.RealParam.IPV4.dwIPAddress = dwIP;
							com_address.RealParam.IPV4.wServicePort = (WORD)50000;
							nRes = AS_ME_OpenController(AS_ME_TYPE_UNKOWN,&com_address,AS_ME_NET_ADDRESS,0xFFFF,NULL,&m_hController);
							if(nRes < 0)//Error Return
							{
								char chRet[RETLEN]={0};
								sprintf(chRet, "%d", nRes);
								printf("AS_ME_OpenController ERROR.  CODE =%d\n",nRes);
								sendto(sock,chRet, strlen(chRet)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
							}
							else
							{
 								AS_ME_STATE State;
 								nRes = AS_ME_GetState(m_hController, &State);
								if(nRes < 0)//Error Return
								{
									char chRet[RETLEN]={0};
									sprintf(chRet, "%d", nRes);
									printf("AS_ME_GetState ERROR.  CODE =%d\n",nRes);
									sendto(sock,chRet, strlen(chRet)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
								}
								else
								{
									char chRet[RETLEN]={0};
									sprintf(chRet, "%d", State.aDoor[0]);
									printf("getState  State.aDoor=%s\n",chRet);
									sendto(sock,chRet, strlen(chRet)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));//返回状态

									if (m_hController != NULL)
									{
										AS_ME_CloseController(m_hController);
									}
								}
							}	
						}	
					}
			
				}
				fflush(0);
			} 

		}
		closesocket(sock); 
		WSACleanup(); 
		return 0;
	}
}
//test for git
//just another sentence
//test for git commit -a

