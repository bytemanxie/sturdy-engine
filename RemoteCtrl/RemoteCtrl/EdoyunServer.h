#pragma once
#include <MSWSock.h>
#include "EdoyunThread.h"
#include <map>
#include "CEdoyunQueue.h"

enum EdoyunOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class EdoyunServer;
class EdoyunClient;

typedef std::shared_ptr<EdoyunClient> PCLIENT;

class EdoyunOverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;//操作 参见EdoyunOperator
    std::vector<char> m_buffer;//缓冲区
    ThreadWorker m_worker;//处理函数
    EdoyunServer* m_server;//服务器对象
    PCLIENT m_client;//对应的客户端
    WSABUF m_wsabuffer;
};



template<EdoyunOperator op>
class AcceptOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
    bool A();
public:
    AcceptOverlapped();

    int AcceptWorker();

    ~AcceptOverlapped() {}

    
};

typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;

template<EdoyunOperator>
class RecvOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();

    int RecvWorker()
    {
        int ret = m_client->Recv();
        return ret;
    }
    PCLIENT m_client;
};

typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

template<EdoyunOperator>
class SendOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    SendOverlapped();

    int SendWorker()
    {
        return -1;
    }
};

typedef SendOverlapped<ESend> SENDOVERLAPPED;

class EdoyunClient {
public:
    EdoyunClient();
    ~EdoyunClient() {
        closesocket(m_sock);
    }

    void SetOverlapped(PCLIENT& ptr);

    operator SOCKET() {
        return m_sock;
    }

    operator PVOID()
    {
        return &m_buffer[0];
    }

    operator LPOVERLAPPED()
    {
        return &m_overlapped->m_overlapped;
    }

    operator LPDWORD()
    {
        return &m_received;
    }

    LPWSABUF RecvWSABuffer();

    LPWSABUF SendWSABuffer();

    DWORD& flags()
    {
        return m_flags;
    }

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }

    size_t GetBufferSize() const
    {
        return m_buffer.size();
    }

    bool Recv()
    {
        int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
        if (ret <= 0) return -1;
        m_used += (size_t)ret;
        return 0;
    }

private:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;

    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;
    std::vector<char> m_buffer;

    size_t m_used;//已经使用的缓冲区大小
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
};



template<EdoyunOperator>
class ErrorOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof m_overlapped);
        m_buffer.resize(1024);
    }

    int ErrorWorker()
    {
        return -1;
    }
};

typedef ErrorOverlapped<EError> ERROROVERLAPPED;



class EdoyunServer :
    public ThreadFuncBase
{
public:
    EdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = port;
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        
    }

    bool StartService();
    
    ~EdoyunServer(){
        DWORD transferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* lpOverlapped;
        if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
        {

        }
    }

    int AcceptClient()
    {

    }

    bool NewAccept()
    {
        PCLIENT pClient(new EdoyunClient());
        pClient->SetOverlapped(pClient);

        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));

        if (!AcceptEx(
            m_sock, *pClient, *pClient, 0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            *pClient, *pClient))
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

private:
    void CreateSocket()
    {
        m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof opt);
    }

    

    //EdoyunClient& GetFree;
    int threaedIocp();
    

private:
    EdoyunThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;

    std::map<SOCKET, std::shared_ptr<EdoyunClient>> m_client;
    CEdoyunQueue<EdoyunClient> m_lstClient;
};

