/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_REALMSOCKET_H
#define SF_REALMSOCKET_H

#include "Common.h"
#include <ace/Basic_Types.h>
#include <ace/Message_Block.h>
#include <ace/SOCK_Stream.h>
#include <ace/Svc_Handler.h>
#include <ace/Synch_Traits.h>

class RealmSocket : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
private:
    typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> Base;

public:
    class Session
    {
    public:
        Session(void);
        virtual ~Session(void);

        virtual void OnRead(void) = 0;
        virtual void OnAccept(void) = 0;
        virtual void OnClose(void) = 0;
    };

    RealmSocket(void);
    virtual ~RealmSocket(void);

    size_t recv_len(void) const;
    bool recv_soft(char* buf, size_t len);
    bool recv(char* buf, size_t len);
    void recv_skip(size_t len);

    bool send(const char* buf, size_t len);

    const std::string& getRemoteAddress(void) const;

    uint16 getRemotePort(void) const;

    virtual int open(void*);

    virtual int close(u_long);

    virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
    virtual int handle_output(ACE_HANDLE = ACE_INVALID_HANDLE);

    virtual int handle_close(ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK);

    void set_session(Session* session);

private:
    ssize_t noblk_send(ACE_Message_Block& message_block);

    ACE_Message_Block input_buffer_;
    Session* session_;
    std::string _remoteAddress;
    uint16 _remotePort;
};

#endif /* __REALMSOCKET_H__ */
