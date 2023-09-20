#pragma once

#include <list>

#include "Packet.h"



typedef void (*SOCKET_CALLBACK)(void* , int , std::list<CPacket>&, CPacket&);

class CServerSocket
{
public:
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL)//静态成员函数没有this指针，无法访问成员变量
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
	{
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false)
			{
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();
			if (ret > 0)
			{
				m_callback(m_arg, ret, lstPackets, m_packet);
				while (lstPackets.size() > 0)
				{
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}

protected:

	bool InitSocket(short port)
	{

		if (m_sock == -1)return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof serv_addr);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof serv_addr) == -1) return false;

		if (listen(m_sock, 1)) return false;

		return true;
	}

	bool AcceptClient()
	{
		sockaddr_in client_addr;
		int cli_sz = sizeof client_addr;
		m_client = accept(m_sock, (sockaddr*)&client_addr, &cli_sz);
		//TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1) return false;
		return true;
	}

#define BUFFER_SIZE 4096
	//处理命令，并将m_packet打包,里面的内容即为客户端发来的命令
	//返回接收到的命令号
	int DealCommand() {//处理命令
		if (m_client == -1)return -1;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("内存不足！\r\n");
			return -2;
		}
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[]buffer;
				return -1;
			}
			//TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_client == -1)return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	//从m_packet中获取文件的路径
	/*bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4) || (m_packet.sCmd == 9))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}*/

	/*bool GetMouseEvent(const MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy((void*) & mouse, m_packet.strData.c_str(), sizeof MOUSEEV);
			return true;
		}
		return 0;
	}*/

	/*CPacket& GetPacket()
	{
		return m_packet;
	}*/
	void CloseClient()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
		
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_sock, m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss)
	{

	}

	CServerSocket(const CServerSocket& ss)
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() 
	{
		m_sock = INVALID_SOCKET;
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境, 请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);//text, caption
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	};
	~CServerSocket() 
	{
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data))
		{
			return FALSE;
		}
		return TRUE;
	}
	static CServerSocket* m_instance;
	static void releaseInstance()
	{
		if (m_instance)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

//extern CServerSocket server;