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

bool CClientSocket::SendPacket(const CPacket& pack, 
	std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	if (m_sock == INVALID_SOCKET)
	{
		/*if (InitSocket() == false) return false;*/
		_beginthread(&CClientSocket::threadEntry, 0, this);
	}
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(
		pack.hEvent, std::list<CPacket>()
		));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));

	m_lstSend.push_back(pack);
	WaitForSingleObject(pack.hEvent, INFINITE);
	std::map<HANDLE, std::list<CPacket>>::iterator it;
	it = m_mapAck.find(pack.hEvent);

	if (it != m_mapAck.end())
	{
		std::list<CPacket>::iterator i;
		for (i = it->second.begin(); i != it->second.end(); i++)
		{
			lstPacks.push_back(*i);
		}
		m_mapAck.erase(it);
		return true;
	}
	return false;
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1)return false;
	//Dump((BYTE*)pack.Data(), pack.Size());
	std::string strOut;
	pack.Data(strOut);
	//pack.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	return false;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	_endthread();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();

	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET)
	{
		TRACE("%d %s\r\n", __LINE__, __FUNCTION__);
		if (m_lstSend.size() > 0)
		{
			
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			CPacket& head = m_lstSend.front();
			if (Send(head) == false)
			{
				TRACE("发送失败！\r\n");
				
				continue;
			}
			std::map<HANDLE, std::list<CPacket>>::iterator it;
			it = m_mapAck.find(head.hEvent);
			std::map<HANDLE, bool>::iterator it0 =
				m_mapAutoClosed.find(head.hEvent);

			do {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || index > 0)
				{
					index += length;
					size_t size = (size_t)index;
					CPacket pack((BYTE*)pBuffer, size);
					TRACE("size = %d\r\n", size);
					if (size > 0)
					{
						//TODO: 通知对应的事件
						pack.hEvent = head.hEvent;
						it->second.push_back(pack);
						SetEvent(head.hEvent);
						memmove(pBuffer, pBuffer + size, index - size);
						index -= size;
						if (it0->second == true)
						{
							SetEvent(head.hEvent);
						}
					}
				}
				else if (length <= 0 && index <= 0)
				{
					CloseSocket();
					SetEvent(head.hEvent);//等到服务器关闭命令之后，再通知事情完成
				}
			} while (it0->second == false);
			
			m_lstSend.pop_front();
			InitSocket();
		}
		
	}
	CloseSocket();
}
