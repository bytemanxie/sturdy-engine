#pragma once

#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#define WM_SEND_PACK (WM_USER + 1) // ���Ͱ�����

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//nSizeΪ�������ݵĳ���
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent) {
		sHead = 0xFEFF;
		nLength = nSize + 4;//���Ͽ��������У�鳤��
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
		this->hEvent = hEvent;
	}
	CPacket(const CPacket& pack) {//�������캯��
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		hEvent = pack.hEvent;
	}
	CPacket(const BYTE* pData, size_t& nSize): hEvent(INVALID_HANDLE_VALUE) {
		size_t i = 0;
		for (; i < nSize; i++) {//�Ұ�ͷ
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {//�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {//��δ��ȫ���յ����ͷ��أ�����ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;//head2 length4 data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
			hEvent = pack.hEvent;
		}
		return *this;
	}
	int Size() {//�����ݵĴ�С
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead;//�̶�λ 0xFEFF
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
	//std::string strOut;//������������
	HANDLE hEvent;
};

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	BOOL IsDirectory;//�Ƿ�ΪĿ¼ 0 �� 1 ��
	BOOL HasNext;//�Ƿ��к��� 0 û�� 1 ��
	char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;

//������ƶ���˫��
//������Ҽ����м�
//����
typedef struct MouseEvent {
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����м�
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;

std::string GetErrInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL)//��̬��Ա����û��thisָ�룬�޷����ʳ�Ա����
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket();
	

#define BUFFER_SIZE 4096000
	//����������packet�����ؽ�������ֵ
	int DealCommand() {//��������
		if (m_sock == -1)return -1;
		//char buffer[1024] = "";
		char* buffer = m_buffer.data();
		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		//memset(buffer, 0, BUFFER_SIZE);
		static int index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ((int)len <= 0 && ((int)index == 0)) { // recvû���ļ����ǻ��������ܻ���
				return -1;
			}
			TRACE("recv len = %d(0x%08X) index = %d(0x%08x)\r\n", len, len, index, index);
			index += len;
			len = index;
			TRACE("recv len = %d(0x%08X) index = %d(0x%08x)\r\n", len, len, index, index);
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,
		bool isAutoClosed = true);
	
	
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(const MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy((void*)&mouse, m_packet.strData.c_str(), sizeof MOUSEEV);
			return true;
		}
		return 0;
	}

	//����m_packet
	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	void UpdataAddress(int nIP, int nPort)
	{
		if ((m_nIp != nIP) || (m_nPort != nPort))
		{
			m_nIp = nIP;
			m_nPort = nPort;
	
		}
	
	}

private:
	typedef void(CClientSocket::* MSGFUNC) (UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;

	int m_nIp, m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& pack);
	CClientSocket& operator=(const CClientSocket& ss)
	{

	}

	CClientSocket(const CClientSocket& ss)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_bAutoClose = ss.m_bAutoClose;
		m_sock = ss.m_sock;
		m_nIp = ss.m_nIp;
		m_nPort = ss.m_nPort;
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
				TRACE("����ʧ�ܣ� ��Ϣֵ��%d ����ֵ��%08X \r\n", 
					funcs[i].message, funcs[i].func);
			}
		}
		
	}
	CClientSocket():
		m_nIp(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
		m_hThread(INVALID_HANDLE_VALUE)
	{
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���, �����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);//text, caption
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset((char*)m_buffer.data(), 0, BUFFER_SIZE);
		
	}
	~CClientSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	static void threadEntry(void* arg);
	void threadFunc();
	void threadFunc2();

	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data))
		{
			return FALSE;
		}
		return TRUE;
	}

	void SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);
	static CClientSocket* m_instance;

	static void releaseInstance()
	{
		if (m_instance)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

//extern CServerSocket server;