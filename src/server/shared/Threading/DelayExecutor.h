/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef _M_DELAY_EXECUTOR_H
#define _M_DELAY_EXECUTOR_H

#include <ace/Activation_Queue.h>
#include <ace/Method_Request.h>
#include <ace/Task.h>

class DelayExecutor : protected ACE_Task_Base
{
public:
    DelayExecutor();
    virtual ~DelayExecutor();
    static DelayExecutor* instance();
    int execute(ACE_Method_Request* new_req);
    int start(int num_threads = 1, ACE_Method_Request* pre_svc_hook = NULL, ACE_Method_Request* post_svc_hook = NULL);
    int deactivate();
    bool activated();
    virtual int svc();

private:
    ACE_Activation_Queue queue_;
    ACE_Method_Request* pre_svc_hook_;
    ACE_Method_Request* post_svc_hook_;
    bool activated_;

    void activated(bool s);
};

#endif // _M_DELAY_EXECUTOR_H
