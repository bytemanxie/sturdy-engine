#include "pch.h"
#include "EdoyunServer.h"

#pragma warning (disable:4407)


template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped(){
    m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof m_overlapped);
    m_buffer.resize(1024);
    m_server = NULL;
}

template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
    INT lLength = 0, rLength = 0;
    if (*(LPDWORD)*m_client.get() > 0)
    {
        GetAcceptExSockaddrs(*m_client, 0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength,//本地地址
            (sockaddr**)m_client->GetRemoteAddr(), &rLength//远程地址
        );

        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 
           1, *m_client, &m_client->flags(),
            *m_client, NULL);

        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING))
        {

        }

        if (!m_server->NewAccept())
        {
            return -2;
        }
    }
    return -1;
}

EdoyunClient::EdoyunClient() :m_isbusy(false), 
                            m_overlapped(new ACCEPTOVERLAPPED()), m_flags(0),
                            m_recv(new RECVOVERLAPPED()), m_send(new SENDOVERLAPPED())
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof m_laddr);
    memset(&m_raddr, 0, sizeof m_raddr);
}

void EdoyunClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr;
    m_recv->m_client = ptr;
    m_send->m_client = ptr;

}

LPWSABUF EdoyunClient::RecvWSABuffer()
{
    return &m_recv->m_wsabuffer;
}

LPWSABUF EdoyunClient::SendWSABuffer()
{
    return &m_send->m_wsabuffer;
}

bool EdoyunServer::StartService()
{
    CreateSocket();

    if (bind(m_sock, (sockaddr*)&m_addr, sizeof m_addr) == -1)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    if (listen(m_sock, 3) == -1)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);

    if (m_hIOCP == NULL)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }

    CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
    m_pool.Invoke();

    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&EdoyunServer::threaedIocp));

    if (!NewAccept()) return false;

    return true;
}

int EdoyunServer::threaedIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;

    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
    {
        if (transferred > 0 && (CompletionKey != 0))
        {
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);

            switch (pOverlapped->m_operator)
            {
            case EAccept:
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
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
            default:
                break;
            }


        }
        else
        {
            return -1;
        }

    }
    return 0;
}

template<EdoyunOperator op>
inline SendOverlapped<op>::SendOverlapped() {
    m_operator = ESend;
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    memset(&m_overlapped, 0, sizeof m_overlapped);
    m_buffer.resize(1024);
}

template<EdoyunOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
    m_operator = ERecv;
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
    memset(&m_overlapped, 0, sizeof m_overlapped);
    m_buffer.resize(1024 * 256);
}