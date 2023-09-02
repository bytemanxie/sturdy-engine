#include "pch.h"
#include "ClientSocket.h"
CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pclient = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIp);
	serv_addr.sin_port = htons(m_nPort);

	if (serv_addr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("指定的IP地址，不存在！"));
		return false;
	}

	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof serv_addr);
	if (ret == -1)
	{
		AfxMessageBox(_T("连接失败"));
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry,
			this, 0, &m_threadID);
		
	}
	/*m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry,
		this, 0, &m_threadID);*/
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);

	size_t nTemp = strOut.size();
	CPacket current((BYTE*)pData->strData.c_str(), nTemp);
	return PostThreadMessage(m_threadID, WM_SEND_PACK, 
		(WPARAM)pData, (LPARAM)hWnd);
}

//bool CClientSocket::SendPacket(const CPacket& pack,
//					std::list<CPacket>& lstPacks, bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
//	{
//		/*if (InitSocket() == false) return false;*/
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
//		TRACE("start thread\r\n");
//	}
//
//	m_lock.lock();
//	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(
//		pack.hEvent, lstPacks));
//	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
//	TRACE("cmd: %d event: %08X thread id: %d\r\n", 
//		pack.sCmd, pack.hEvent, GetCurrentThreadId());
//	m_lstSend.push_back(pack);
//	m_lock.unlock();
//
//	WaitForSingleObject(pack.hEvent, INFINITE);
//	std::map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent);
//
//	if (it != m_mapAck.end())
//	{
//		m_lock.lock();
//		m_mapAck.erase(it);
//		m_lock.unlock();
//		return true;
//	}
//	return false;
//}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock = %d\r\n", m_sock);
	if (m_sock == -1)return false;
	//Dump((BYTE*)pack.Data(), pack.Size());
	std::string strOut;
	pack.Data(strOut);
	//pack.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	return false;
}

CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIp = ss.m_nIp;
	m_nPort = ss.m_nPort;
	
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}
CClientSocket::CClientSocket() :
	m_nIp(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, _T("无法初始化套接字环境, 请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);//text, caption
		exit(0);
	}
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry,
		this, 0, &m_threadID);
	m_buffer.resize(BUFFER_SIZE);
	memset((char*)m_buffer.data(), 0, BUFFER_SIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL}
	};

	for (int i = 0; funcs[i].message != 0; i++)
	{
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>
			(funcs[i].message, funcs[i].func)).second == false)
		{
			TRACE("插入失败， 消息值：%d 函数值：%08X \r\n",
				funcs[i].message, funcs[i].func);
		}
	}
}
unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	
	_endthreadex(0);
	
	return 0;
}

//void CClientSocket::threadFunc()
//{
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET)
//	{
//		TRACE("%d %s\r\n", __LINE__, __FUNCTION__);
//		if (m_lstSend.size() > 0)
//		{
//			
//			TRACE("lstSend size: %d\r\n", m_lstSend.size());
//			m_lock.lock();
//			CPacket& head = m_lstSend.front();
//			m_lock.unlock();
//
//			if (Send(head) == false)
//			{
//				TRACE("发送失败！\r\n");
//				
//				continue;
//			}
//			std::map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end())
//			{
//				std::map<HANDLE, bool>::iterator it0 =
//					m_mapAutoClosed.find(head.hEvent);
//
//				do {
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//					if (length > 0 || index > 0)
//					{
//						index += length;
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size);
//						TRACE("%s\r\n", pack.strData.c_str());
//						if (size > 0)
//						{
//							TODO: 通知对应的事件
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack);
//							memmove(pBuffer, pBuffer + size, index - size);
//							index -= size;
//							if (it0->second == true)
//							{
//								SetEvent(head.hEvent);
//								break;
//							}
//						}
//					}
//					else if (length <= 0 && index <= 0)
//					{
//						CloseSocket();
//						SetEvent(head.hEvent);//等到服务器关闭命令之后，再通知事情完成
//						if (it0 != m_mapAutoClosed.end())
//						{
//							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
//						}
//						else
//						{
//							TRACE("异常情况， 没有对应的pair\r\n");
//						}
//						break;
//					}
//				} while (it0->second == false);
//			}
//			m_lock.lock();
//			m_lstSend.pop_front();
//			m_mapAutoClosed.erase(head.hEvent);
//			m_lock.unlock();
//
//			if (InitSocket() == false)
//			{
//				InitSocket();
//			}
//		}
//		Sleep(1);
//	}
//	CloseSocket();
//}

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//TODO: 定义一个消息的数据结构(数据和数据长度， 模式)， 回调消息的数据结构(HWND MESSAGE))
	//TRACE("m_sock = %d\r\n", m_sock);
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	//先解决有可能内存泄漏的问题
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;

	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);


	if (InitSocket() == true)
	{
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			
			while (m_sock != INVALID_SOCKET)
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || index >= 0)
				{
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0)
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
						
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + nLen, index);
				}
				else
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}

		}
		else
		{
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else
	{
		//TODO:
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}
