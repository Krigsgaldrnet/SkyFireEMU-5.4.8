/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/** \file WorldSocketMgr.cpp
*  \ingroup u2w
*  \author Derex <derex101@gmail.com>
*/

#include "WorldSocketMgr.h"

#include <ace/ACE.h>
#include <ace/Atomic_Op.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Guard_T.h>
#include <ace/Log_Msg.h>
#include <ace/os_include/arpa/os_inet.h>
#include <ace/os_include/netinet/os_tcp.h>
#include <ace/os_include/sys/os_socket.h>
#include <ace/os_include/sys/os_types.h>
#include <ace/Reactor.h>
#include <ace/Reactor_Impl.h>
#include <ace/TP_Reactor.h>

#include <set>

#include "Common.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "WorldSocket.h"
#include "WorldSocketAcceptor.h"

/**
* This is a helper class to WorldSocketMgr, that manages
* network threads, and assigning connections from acceptor thread
* to other network threads
*/
class ReactorRunnable : protected ACE_Task_Base
{
public:
    ReactorRunnable() :
        m_Reactor(0),
        m_Connections(0),
        m_ThreadId(-1)
    {
        ACE_Reactor_Impl* imp;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)

        imp = new ACE_Dev_Poll_Reactor();

        imp->max_notify_iterations(128);
        imp->restart(1);

#else

        imp = new ACE_TP_Reactor();
        imp->max_notify_iterations(128);

#endif

        m_Reactor = new ACE_Reactor(imp, 1);
    }

    virtual ~ReactorRunnable()
    {
        Stop();
        Wait();

        delete m_Reactor;
    }

    void Stop()
    {
        m_Reactor->end_reactor_event_loop();
    }

    int Start()
    {
        if (m_ThreadId != -1)
            return -1;

        return (m_ThreadId = activate());
    }

    void Wait() { ACE_Task_Base::wait(); }

    long Connections()
    {
        return static_cast<long> (m_Connections.value());
    }

    int AddSocket(WorldSocket* sock)
    {
        std::lock_guard<std::mutex> guard(m_NewSockets_Lock);

        ++m_Connections;
        sock->AddReference();
        sock->reactor(m_Reactor);
        m_NewSockets.insert(sock);

        sScriptMgr->OnSocketOpen(sock);

        return 0;
    }

    ACE_Reactor* GetReactor()
    {
        return m_Reactor;
    }

protected:
    void AddNewSockets()
    {
        std::lock_guard<std::mutex> guard(m_NewSockets_Lock);

        if (m_NewSockets.empty())
            return;

        for (SocketSet::const_iterator i = m_NewSockets.begin(); i != m_NewSockets.end(); ++i)
        {
            WorldSocket* sock = (*i);

            if (sock->IsClosed())
            {
                sScriptMgr->OnSocketClose(sock, true);

                sock->RemoveReference();
                --m_Connections;
            }
            else
                m_Sockets.insert(sock);
        }

        m_NewSockets.clear();
    }

    virtual int svc()
    {
        SF_LOG_DEBUG("misc", "Network Thread Starting");

        ACE_ASSERT(m_Reactor);

        SocketSet::iterator i, t;

        while (!m_Reactor->reactor_event_loop_done())
        {
            // dont be too smart to move this outside the loop
            // the run_reactor_event_loop will modify interval
            ACE_Time_Value interval(0, 10000);

            if (m_Reactor->run_reactor_event_loop(interval) == -1)
                break;

            AddNewSockets();

            for (i = m_Sockets.begin(); i != m_Sockets.end();)
            {
                if ((*i)->Update() == -1)
                {
                    t = i;
                    ++i;

                    (*t)->CloseSocket();

                    sScriptMgr->OnSocketClose((*t), false);

                    (*t)->RemoveReference();
                    --m_Connections;
                    m_Sockets.erase(t);
                }
                else
                    ++i;
            }
        }

        SF_LOG_DEBUG("misc", "Network Thread exits");

        return 0;
    }

private:
    typedef ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> AtomicInt;
    typedef std::set<WorldSocket*> SocketSet;

    ACE_Reactor* m_Reactor;
    AtomicInt m_Connections;
    int m_ThreadId;

    SocketSet m_Sockets;

    SocketSet m_NewSockets;
    std::mutex m_NewSockets_Lock;
};

WorldSocketMgr::WorldSocketMgr() :
    m_NetThreads(0),
    m_NetThreadsCount(0),
    m_SockOutKBuff(-1),
    m_SockOutUBuff(65536),
    m_UseNoDelay(true),
    m_Acceptor(0) { }

WorldSocketMgr::~WorldSocketMgr()
{
    delete[] m_NetThreads;
    delete m_Acceptor;
}

int
WorldSocketMgr::StartReactiveIO(ACE_UINT16 port, const char* address)
{
    m_UseNoDelay = sConfigMgr->GetBoolDefault("Network.TcpNodelay", true);

    int num_threads = sConfigMgr->GetIntDefault("Network.Threads", 1);

    if (num_threads <= 0)
    {
        SF_LOG_ERROR("misc", "Network.Threads is wrong in your config file");
        return -1;
    }

    m_NetThreadsCount = static_cast<size_t> (num_threads + 1);

    m_NetThreads = new ReactorRunnable[m_NetThreadsCount];

    SF_LOG_DEBUG("misc", "Max allowed socket connections %d", ACE::max_handles());

    // -1 means use default
    m_SockOutKBuff = sConfigMgr->GetIntDefault("Network.OutKBuff", -1);

    m_SockOutUBuff = sConfigMgr->GetIntDefault("Network.OutUBuff", 65536);

    if (m_SockOutUBuff <= 0)
    {
        SF_LOG_ERROR("misc", "Network.OutUBuff is wrong in your config file");
        return -1;
    }

    m_Acceptor = new WorldSocketAcceptor;

    ACE_INET_Addr listen_addr(port, address);

    if (m_Acceptor->open(listen_addr, m_NetThreads[0].GetReactor(), ACE_NONBLOCK) == -1)
    {
        SF_LOG_ERROR("misc", "Failed to open acceptor, check if the port is free");
        return -1;
    }

    for (size_t i = 0; i < m_NetThreadsCount; ++i)
        m_NetThreads[i].Start();

    return 0;
}

int
WorldSocketMgr::StartNetwork(ACE_UINT16 port, const char* address)
{
    if (!sLog->ShouldLog("misc", LogLevel::LOG_LEVEL_DEBUG))
        ACE_Log_Msg::instance()->priority_mask(LM_ERROR, ACE_Log_Msg::PROCESS);

    if (StartReactiveIO(port, address) == -1)
        return -1;

    sScriptMgr->OnNetworkStart();

    return 0;
}

void
WorldSocketMgr::StopNetwork()
{
    if (m_Acceptor)
    {
        m_Acceptor->close();
    }

    if (m_NetThreadsCount != 0)
    {
        for (size_t i = 0; i < m_NetThreadsCount; ++i)
            m_NetThreads[i].Stop();
    }

    Wait();

    sScriptMgr->OnNetworkStop();
}

void
WorldSocketMgr::Wait()
{
    if (m_NetThreadsCount != 0)
    {
        for (size_t i = 0; i < m_NetThreadsCount; ++i)
            m_NetThreads[i].Wait();
    }
}

int
WorldSocketMgr::OnSocketOpen(WorldSocket* sock)
{
    // set some options here
    if (m_SockOutKBuff >= 0)
    {
        if (sock->peer().set_option(SOL_SOCKET,
            SO_SNDBUF,
            (void*)&m_SockOutKBuff,
            sizeof(int)) == -1 && errno != ENOTSUP)
        {
            SF_LOG_ERROR("misc", "WorldSocketMgr::OnSocketOpen set_option SO_SNDBUF");
            return -1;
        }
    }

    static const int ndoption = 1;

    // Set TCP_NODELAY.
    if (m_UseNoDelay)
    {
        if (sock->peer().set_option(ACE_IPPROTO_TCP,
            TCP_NODELAY,
            (void*)&ndoption,
            sizeof(int)) == -1)
        {
            SF_LOG_ERROR("misc", "WorldSocketMgr::OnSocketOpen: peer().set_option TCP_NODELAY errno = %s", ACE_OS::strerror(errno));
            return -1;
        }
    }

    sock->m_OutBufferSize = static_cast<size_t> (m_SockOutUBuff);

    // we skip the Acceptor Thread
    size_t min = 1;

    ACE_ASSERT(m_NetThreadsCount >= 1);

    for (size_t i = 1; i < m_NetThreadsCount; ++i)
        if (m_NetThreads[i].Connections() < m_NetThreads[min].Connections())
            min = i;

    return m_NetThreads[min].AddSocket(sock);
}
