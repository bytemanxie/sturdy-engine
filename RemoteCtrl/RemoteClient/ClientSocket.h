#pragma once

#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//nSizeΪ�������ݵĳ���
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
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
	}
	CPacket(const CPacket& pack) {//�������캯��
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
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
		}
		return *this;
	}
	int Size() {//�����ݵĴ�С
		return nLength + 6;
	}
	const char* Data() {
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
	std::string strOut;//������������
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

	bool InitSocket(int nIP, int nPort)
	{
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1)return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof serv_addr);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIP);
		serv_addr.sin_port = htons(nPort);

		if (serv_addr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox(_T("ָ����IP��ַ�������ڣ�"));
			return false;
		}

		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof serv_addr);
		if (ret == -1)
		{
			AfxMessageBox(_T("����ʧ��"));
			TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}


		return true;
	}

#define BUFFER_SIZE 409600
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
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0 && (index == 0)) { // recvû���ļ����ǻ��������ܻ���
				return -1;
			}
			//TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_sock == -1)return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}

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

private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss)
	{

	}

	CClientSocket(const CClientSocket& ss)
	{
		m_sock = ss.m_sock;
	}
	CClientSocket()
	{
		m_sock = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���, �����������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);//text, caption
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset((char*)m_buffer.data(), 0, BUFFER_SIZE);
	};
	~CClientSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data))
		{
			return FALSE;
		}
		return TRUE;
	}
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