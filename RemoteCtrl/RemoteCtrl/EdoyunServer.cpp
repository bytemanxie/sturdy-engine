#include "pch.h"
#include "EdoyunServer.h"
#include "EdoyunTool.h"
#pragma warning(disable:4407)
template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_operator = EAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	TRACE("AcceptWorker this %08X\r\n", this);
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocalAddr, &lLength,//本地地址
			(sockaddr**)&pRemoteAddr, &rLength//远程地址
		);

		memcpy(m_client->GetLocalAddr(), pLocalAddr, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client, (ULONG_PTR)m_client);

		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:报错
			TRACE("WSARecv failed %d\r\n", ret);
		}
		if (!m_server->NewAccept())
		{
			return -2;
		}
	}
	return -1;//必须返回-1，否则循环不会终止
}

template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}


EdoyunClient::EdoyunClient()
	:m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData)
{
	TRACE("m_overlapped %08X\r\n", &m_overlapped);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

EdoyunClient::~EdoyunClient()
{
	m_buffer.clear();
	closesocket(m_sock);
}

void EdoyunClient::SetOverlapped(EdoyunClient* ptr) {
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}

EdoyunClient::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

LPWSABUF EdoyunClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPOVERLAPPED EdoyunClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

LPWSABUF EdoyunClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

LPOVERLAPPED EdoyunClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}

int EdoyunClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0) return -1;
	m_used += (size_t)ret;
	//TODO:解析数据
	CEdoyunTool::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

int EdoyunClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && WSAGetLastError() != WSA_IO_PENDING) {
			CEdoyunTool::ShowError();
			return ret;
		}
	}
	return 0;
}

EdoyunServer::~EdoyunServer()
{
	closesocket(m_sock);
	std::map<SOCKET, EdoyunClient*>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++) {
		delete it->second;
		it->second = NULL;
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool EdoyunServer::StartService()
{
	CreateSocket();
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	if (!NewAccept())return false;
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	//m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threadIocp));
	return true;
}

bool EdoyunServer::NewAccept()
{
	//PCLIENT pClient(new EdoyunClient());
	EdoyunClient* pClient = new EdoyunClient();
	pClient->SetOverlapped(pClient);
	m_client.insert(std::pair<SOCKET, EdoyunClient*>(*pClient, pClient));
	if (!AcceptEx(m_sock,
		*pClient,
		*pClient,
		0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		*pClient, *pClient))
	{
		if (WSAGetLastError() != ERROR_SUCCESS && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TRACE("连接失败：%d %s\r\n", WSAGetLastError(), CEdoyunTool::GetErrInfo(WSAGetLastError()).c_str());
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void EdoyunServer::BindNewSocket(SOCKET s, ULONG_PTR nKey)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, nKey, 0);
}

void EdoyunServer::CreateSocket()
{
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		return;
	}
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int EdoyunServer::threadIocp()
{
	DWORD tranferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &tranferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if (CompletionKey != 0) {
			EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
			pOverlapped->m_server = this;
			TRACE("Operator is %d\r\n", pOverlapped->m_operator);
			switch (pOverlapped->m_operator) {
			case EAccept:
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				TRACE("pOver %08X\r\n", pOver);
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ERecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case ESend:
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case EError:
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}
