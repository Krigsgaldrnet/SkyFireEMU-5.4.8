/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/**
* @file main.cpp
* @brief Authentication Server main program
*
* This file contains the main program for the
* authentication server
*/
#pragma comment (lib, "Crypt32")
#include <ace/ACE.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Sig_Handler.h>
#include <ace/TP_Reactor.h>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>
#include <openssl/provider.h>

#include "Common.h"
#include "Configuration/Config.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "RealmAcceptor.h"
#include "RealmList.h"
#include "SignalHandler.h"
#include "SystemConfig.h"
#include "Util.h"

#ifdef __linux__
#include <sched.h>
#include <sys/resource.h>
#define PROCESS_HIGH_PRIORITY -15 // [-20, 19], default is 0
#endif

#ifndef _SKYFIRE_REALM_CONFIG
# define _SKYFIRE_REALM_CONFIG  "authserver.conf"
#endif

bool StartDB(const char* host, const char* port, const char* user, const char* pass, const char* database, bool noUseConfigDatabaseInfo);
void StopDB();

bool stopEvent = false;                                     // Setting it to true stops the server

LoginDatabaseWorkerPool LoginDatabase;                      // Accessor to the authserver database

#ifndef _SKYFIRE_AUTH_DATABASE_HOST
# define _SKYFIRE_AUTH_DATABASE_HOST  ""
#endif

#ifndef _SKYFIRE_AUTH_DATABASE_PORT
# define _SKYFIRE_AUTH_DATABASE_PORT  ""
#endif

#ifndef _SKYFIRE_AUTH_DATABASE_USER
# define _SKYFIRE_AUTH_DATABASE_USER  ""
#endif

#ifndef _SKYFIRE_AUTH_DATABASE_PASS
# define _SKYFIRE_AUTH_DATABASE_PASS  ""
#endif

#ifndef _SKYFIRE_AUTH_DATABASE
# define _SKYFIRE_AUTH_DATABASE  ""
#endif

/// Handle authserver's termination signals
class AuthServerSignalHandler : public Skyfire::SignalHandler
{
public:
    virtual void HandleSignal(int sigNum)
    {
        switch (sigNum)
        {
            case SIGINT:
            case SIGTERM:
                stopEvent = true;
                break;
        }
    }
};

/// Print out the usage string for this program on the console.
void usage(const char* prog)
{
    printf("Usage:\n");
    printf(" %s [<options>]\n");
    printf("    -c config_file                   use config_file as configuration file\n");
    printf("    --no_use_config_database_info    dont use database login info from config file\n");
    printf("    --db_host                        sets the database host, requires: --no_use_config_database_info\n");
    printf("    --db_port                        sets the database port, requires: --no_use_config_database_info\n");
    printf("    --db_user                        sets the database user, requires: --no_use_config_database_info\n");
    printf("    --db_password                    sets the database password, requires: --no_use_config_database_info\n");
    printf("    --db_auth                        sets the auth database, requires: --no_use_config_database_info\n");
}

/// Launch the auth server
extern int main(int argc, char** argv)
{
    bool noUseConfigDatabaseInfo = 0;
    const char* db_Host = _SKYFIRE_AUTH_DATABASE_HOST;
    const char* db_Port = _SKYFIRE_AUTH_DATABASE_PORT;
    const char* db_User = _SKYFIRE_AUTH_DATABASE_USER;
    const char* db_Password = _SKYFIRE_AUTH_DATABASE_PASS;
    const char* authDB = _SKYFIRE_AUTH_DATABASE;

    // Command line parsing to get the configuration file name
    char const* configFile = _SKYFIRE_REALM_CONFIG;
    int count = 1;
    while (count < argc)
    {
        if (strcmp(argv[count], "--help") == 0)
        {
            usage(argv[0]);
            return 1;
        }
        if (strcmp(argv[count], "--no_use_config_database_info") == 0)
        {
            noUseConfigDatabaseInfo = argv[count];
        }

        if (noUseConfigDatabaseInfo == 1)
        {
            if (strcmp(argv[count], "--db_host") == 0)
            {
                if (++count >= argc)
                {
                    printf("Runtime-Error: --db_host option requires an input argument\n");
                    usage(argv[0]);
                    return 1;
                }
                else
                {

                    db_Host = argv[count];
                }
            }

            if (strcmp(argv[count], "--db_port") == 0)
            {
                if (++count >= argc)
                {
                    printf("Runtime-Error: --db_port option requires an input argument\n");
                    usage(argv[0]);
                    return 1;
                }
                else
                    db_Port = argv[count];
            }

            if (strcmp(argv[count], "--db_user") == 0)
            {
                if (++count >= argc)
                {
                    printf("Runtime-Error: --db_user option requires an input argument\n");
                    usage(argv[0]);
                    return 1;
                }
                else
                    db_User = argv[count];
            }

            if (strcmp(argv[count], "--db_password") == 0)
            {
                if (++count >= argc)
                {
                    printf("Runtime-Error: --db_password option requires an input argument\n");
                    usage(argv[0]);
                    return 1;
                }
                else
                    db_Password = argv[count];
            }

            if (strcmp(argv[count], "--db_auth") == 0)
            {
                if (++count >= argc)
                {
                    printf("Runtime-Error: --db_auth option requires an input argument\n");
                    usage(argv[0]);
                    return 1;
                }
                else
                    authDB = argv[count];
            }
        }

        if (strcmp(argv[count], "-c") == 0)
        {
            if (++count >= argc)
            {
                printf("Runtime-Error: -c option requires an input argument\n");
                usage(argv[0]);
                return 1;
            }
            else
                configFile = argv[count];
        }
        ++count;
    }

    if (!sConfigMgr->LoadInitial(configFile))
    {
        printf("Invalid or missing configuration file : %s\n", configFile);
        printf("Verify that the file exists and has \'[authserver]\' written in the top of the file!\n");
        return 1;
    }

    //TODO: FIX ME revision.
    //SF_LOG_INFO("server.authserver", "%s (authserver)", _FULLVERSION);
    SF_LOG_INFO("server.authserver", "<Ctrl-C> to stop.\n");

    SF_LOG_INFO("server.authserver", "   ______  __  __  __  __  ______ __  ______  ______ ");
    SF_LOG_INFO("server.authserver", "  /\\  ___\\/\\ \\/ / /\\ \\_\\ \\/\\  ___/\\ \\/\\  == \\/\\  ___\\ ");
    SF_LOG_INFO("server.authserver", "  \\ \\___  \\ \\  _'-\\ \\____ \\ \\  __\\ \\ \\ \\  __<\\ \\  __\\ ");
    SF_LOG_INFO("server.authserver", "   \\/\\_____\\ \\_\\ \\_\\/\\_____\\ \\_\\  \\ \\_\\ \\_\\ \\_\\ \\_____\\ ");
    SF_LOG_INFO("server.authserver", "    \\/_____/\\/_/\\/_/\\/_____/\\/_/   \\/_/\\/_/ /_/\\/_____/ ");
    SF_LOG_INFO("server.authserver", "  Project SkyFireEmu 2011 - 2025(c) Open-sourced Game Emulation ");
    SF_LOG_INFO("server.authserver", "           <http://www.projectskyfire.org/> \n");

    SF_LOG_INFO("server.authserver", "Using configuration file %s.", configFile);

    ///- Check the version of the configuration file
    uint32 confVersion = sConfigMgr->GetIntDefault("ConfVersion", 0);
    if (confVersion < SKYFIREAUTH_CONFIG_VERSION)
    {
        SF_LOG_INFO("server.authserver", "*****************************************************************************");
        SF_LOG_INFO("server.authserver", " WARNING: Your authserver.conf version indicates your conf file is out of date!");
        SF_LOG_INFO("server.authserver", "          Please check for updates, as your current default values may cause");
        SF_LOG_INFO("server.authserver", "          strange behavior.");
        SF_LOG_INFO("server.authserver", "*****************************************************************************");
    }

    SF_LOG_WARN("server.authserver", "%s (Library: %s)", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));

    OSSL_PROVIDER *default_provider = OSSL_PROVIDER_try_load(NULL, "default", 1);
    OSSL_PROVIDER* legacy_provider = OSSL_PROVIDER_try_load(NULL, "legacy", 1);

    if (legacy_provider == NULL)
    {
        SF_LOG_INFO("server.authserver", "Failed loading legacy provider, Try to load legacy provider again.");
        legacy_provider = OSSL_PROVIDER_try_load(NULL, "legacy", 1);
    }

    SF_LOG_INFO("server.authserver", "Loading default provider: (%s)", (default_provider == NULL || !OSSL_PROVIDER_available(NULL, "default")) ? "failed" : "succeeded");
    SF_LOG_INFO("server.authserver", "Loading legacy provider: (%s)", (legacy_provider == NULL || !OSSL_PROVIDER_available(NULL, "legacy")) ? "failed" : "succeeded");

    // recheck 
    if (legacy_provider == NULL)
        return 1;

    OSSL_PROVIDER_unload(legacy_provider);

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)
    ACE_Reactor::instance(new ACE_Reactor(new ACE_Dev_Poll_Reactor(ACE::max_handles(), 1), 1), true);
#else
    ACE_Reactor::instance(new ACE_Reactor(new ACE_TP_Reactor(), true), true);
#endif

    SF_LOG_DEBUG("server.authserver", "Max allowed open files is %d", ACE::max_handles());

    // authserver PID file creation
    std::string pidFile = sConfigMgr->GetStringDefault("PidFile", "");
    if (!pidFile.empty())
    {
        if (uint32 pid = CreatePIDFile(pidFile))
            SF_LOG_INFO("server.authserver", "Daemon PID: %u\n", pid);
        else
        {
            SF_LOG_ERROR("server.authserver", "Cannot create PID file %s.\n", pidFile.c_str());
            return 1;
        }
    }

    // Initialize the database connection
    if (!StartDB(db_Host, db_Port, db_User, db_Password, authDB, noUseConfigDatabaseInfo))
        return 1;

    // Get the list of realms for the server
    sRealmList->Initialize(sConfigMgr->GetIntDefault("RealmsStateUpdateDelay", 20));
    if (sRealmList->size() == 0)
    {
        SF_LOG_ERROR("server.authserver", "No valid realms specified.");
        return 1;
    }

    // Launch the listening network socket
    RealmAcceptor acceptor;

    int32 rmport = sConfigMgr->GetIntDefault("RealmServerPort", 3724);
    if (rmport < 0 || rmport > 0xFFFF)
    {
        SF_LOG_ERROR("server.authserver", "Specified port out of allowed range (1-65535)");
        return 1;
    }

    std::string bind_ip = sConfigMgr->GetStringDefault("BindIP", "0.0.0.0");

    ACE_INET_Addr bind_addr(uint16(rmport), bind_ip.c_str());

    if (acceptor.open(bind_addr, ACE_Reactor::instance(), ACE_NONBLOCK) == -1)
    {
        SF_LOG_ERROR("server.authserver", "Auth server can not bind to %s:%d", bind_ip.c_str(), rmport);
        return 1;
    }

    // Initialize the signal handlers
    AuthServerSignalHandler SignalINT, SignalTERM;

    // Register authservers's signal handlers
    ACE_Sig_Handler Handler;
    Handler.register_handler(SIGINT, &SignalINT);
    Handler.register_handler(SIGTERM, &SignalTERM);

#if defined(_WIN32) || defined(__linux__)

    ///- Handle affinity for multiple processors and process priority
    uint32 affinity = sConfigMgr->GetIntDefault("UseProcessors", 0);
    bool highPriority = sConfigMgr->GetBoolDefault("ProcessPriority", false);

#ifdef _WIN32 // Windows
    HANDLE hProcess = GetCurrentProcess();
    if (affinity > 0)
    {
        ULONG_PTR appAff;
        ULONG_PTR sysAff;

        if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
        {
            ULONG_PTR currentAffinity = affinity & appAff;            // remove non accessible processors

            if (!currentAffinity)
                SF_LOG_ERROR("server.authserver", "Processors marked in UseProcessors bitmask (hex) %x are not accessible for the authserver. Accessible processors bitmask (hex): %x", affinity, appAff);
            else if (SetProcessAffinityMask(hProcess, currentAffinity))
                SF_LOG_INFO("server.authserver", "Using processors (bitmask, hex): %x", currentAffinity);
            else
                SF_LOG_ERROR("server.authserver", "Can't set used processors (hex): %x", currentAffinity);
        }
    }

    if (highPriority)
    {
        if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
            SF_LOG_INFO("server.authserver", "authserver process priority class set to HIGH");
        else
            SF_LOG_ERROR("server.authserver", "Can't set authserver process priority class.");
    }
#else // Linux

    if (affinity > 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        for (unsigned int i = 0; i < sizeof(affinity) * 8; ++i)
            if (affinity & (1 << i))
                CPU_SET(i, &mask);

        if (sched_setaffinity(0, sizeof(mask), &mask))
            SF_LOG_ERROR("server.authserver", "Can't set used processors (hex): %x, error: %s", affinity, strerror(errno));
        else
        {
            CPU_ZERO(&mask);
            sched_getaffinity(0, sizeof(mask), &mask);
            SF_LOG_INFO("server.authserver", "Using processors (bitmask, hex): %lx", *(__cpu_mask*)(&mask));
        }
    }

    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
            SF_LOG_ERROR("server.authserver", "Can't set authserver process priority class, error: %s", strerror(errno));
        else
            SF_LOG_INFO("server.authserver", "authserver process priority class set to %i", getpriority(PRIO_PROCESS, 0));
    }

#endif
#endif

    // maximum counter for next ping
    uint32 numLoops = (sConfigMgr->GetIntDefault("MaxPingTime", 30) * (MINUTE * 1000000 / 100000));
    uint32 loopCounter = 0;

    // Wait for termination signal
    while (!stopEvent)
    {
        // dont move this outside the loop, the reactor will modify it
        ACE_Time_Value interval(0, 100000);

        if (ACE_Reactor::instance()->run_reactor_event_loop(interval) == -1)
            break;

        if ((++loopCounter) == numLoops)
        {
            loopCounter = 0;
            SF_LOG_INFO("server.authserver", "Ping MySQL to keep connection alive");
            LoginDatabase.KeepAlive();
        }
    }

    // Close the Database Pool and library
    StopDB();

    SF_LOG_INFO("server.authserver", "Halting process...");
    return 0;
}

/// Initialize connection to the database
bool StartDB(const char* host, const char* port, const char* user, const char* pass, const char* database, bool noUseConfigDatabaseInfo)
{
    MySQL::Library_Init();
    std::string dbstring;
    if (noUseConfigDatabaseInfo == false)
    {
        dbstring = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");

        if (dbstring.empty())
        {
            SF_LOG_ERROR("server.authserver", "Database not specified");
            return false;
        }
    }

    int32 worker_threads = sConfigMgr->GetIntDefault("LoginDatabase.WorkerThreads", 1);
    if (worker_threads < 1 || worker_threads > 32)
    {
        SF_LOG_ERROR("server.authserver", "Improper value specified for LoginDatabase.WorkerThreads, defaulting to 1.");
        worker_threads = 1;
    }

    int32 synch_threads = sConfigMgr->GetIntDefault("LoginDatabase.SynchThreads", 1);
    if (synch_threads < 1 || synch_threads > 32)
    {
        SF_LOG_ERROR("server.authserver", "Improper value specified for LoginDatabase.SynchThreads, defaulting to 1.");
        synch_threads = 1;
    }

    // NOTE: While authserver is singlethreaded you should keep synch_threads == 1. Increasing it is just silly since only 1 will be used ever.
    if (noUseConfigDatabaseInfo == false)
    {
        if (!LoginDatabase.Open(dbstring, uint8(worker_threads), uint8(synch_threads)))
        {
            SF_LOG_ERROR("server.authserver", "Cannot connect to database");
            return false;
        }
    }
    else
    {
        if (!LoginDatabase.Open(host, port, user, pass, database, uint8(worker_threads), uint8(synch_threads)))
        {
            SF_LOG_ERROR("server.authserver", "Cannot connect to database");
            return false;
        }
    }

    SF_LOG_INFO("server.authserver", "Started auth database connection pool.");
    sLog->SetRealmId(0); // Enables DB appenders when realm is set.
    return true;
}

/// Close the connection to the database
void StopDB()
{
    LoginDatabase.Close();
    MySQL::Library_End();
}
