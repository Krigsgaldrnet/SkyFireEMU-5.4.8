/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/** \file
    \ingroup Skyfired
*/

#include <ace/Sig_Handler.h>

#include "Common.h"
#include "Configuration/Config.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseWorkerPool.h"
#include "SignalHandler.h"
#include "SystemConfig.h"
#include "World.h"
#include "WorldRunnable.h"
#include "WorldSocket.h"
#include "WorldSocketMgr.h"

#include "AuthSocket.h"
#include "CliRunnable.h"
#include "Log.h"
#include "Master.h"
#include "RARunnable.h"
#include "RealmList.h"
#include "SFSoap.h"
#include "Timer.h"
#include "Util.h"

#include "BigNumber.h"

#ifdef _WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

#ifdef __linux__
#include <sched.h>
#include <sys/resource.h>
#define PROCESS_HIGH_PRIORITY -15 // [-20, 19], default is 0
#endif

/// Handle worldservers's termination signals
class WorldServerSignalHandler : public Skyfire::SignalHandler
{
public:
    virtual void HandleSignal(int sigNum)
    {
        switch (sigNum)
        {
            case SIGINT:
                World::StopNow(RESTART_EXIT_CODE);
                break;
            case SIGTERM:
#ifdef _WIN32
            case SIGBREAK:
                if (m_ServiceStatus != 1)
#endif
                    World::StopNow(SHUTDOWN_EXIT_CODE);
                break;
        }
    }
};

class FreezeDetectorRunnable : public ACE_Based::Runnable
{
private:
    uint32 _loops;
    uint32 _lastChange;
    uint32 _delaytime;
public:
    FreezeDetectorRunnable() : _loops(0), _lastChange(0), _delaytime(0) { }

    void SetDelayTime(uint32 t) { _delaytime = t; }

    void run() OVERRIDE
    {
        if (!_delaytime)
            return;

        SF_LOG_INFO("server.worldserver", "Starting up anti-freeze thread (%u seconds max stuck time)...", _delaytime / 1000);
        _loops = 0;
        _lastChange = 0;
        while (!World::IsStopped())
        {
            ACE_Based::Thread::Sleep(1000);
            uint32 curtime = getMSTime();
            // normal work
            uint32 worldLoopCounter = World::m_worldLoopCounter;
            if (_loops != worldLoopCounter)
            {
                _lastChange = curtime;
                _loops = worldLoopCounter;
            }
            // possible freeze
            else if (getMSTimeDiff(_lastChange, curtime) > _delaytime)
            {
                SF_LOG_ERROR("server.worldserver", "World Thread hangs, kicking out server!");
                ASSERT(false);
            }
        }
        SF_LOG_INFO("server.worldserver", "Anti-freeze thread exiting without problems.");
    }
};

/// Main function
int Master::Run()
{
    BigNumber seed1;
    seed1.SetRand(16 * 8);

    //TODO: FIX ME revision
    //SF_LOG_INFO("server.worldserver", "%s (worldserver-daemon)", _FULLVERSION);
    SF_LOG_INFO("server.worldserver", "<Ctrl-C> to stop.\n");

    SF_LOG_INFO("server.worldserver", "   ______  __  __  __  __  ______ __  ______  ______ ");
    SF_LOG_INFO("server.worldserver", "  /\\  ___\\/\\ \\/ / /\\ \\_\\ \\/\\  ___/\\ \\/\\  == \\/\\  ___\\ ");
    SF_LOG_INFO("server.worldserver", "  \\ \\___  \\ \\  _'-\\ \\____ \\ \\  __\\ \\ \\ \\  __<\\ \\  __\\ ");
    SF_LOG_INFO("server.worldserver", "   \\/\\_____\\ \\_\\ \\_\\/\\_____\\ \\_\\  \\ \\_\\ \\_\\ \\_\\ \\_____\\ ");
    SF_LOG_INFO("server.worldserver", "    \\/_____/\\/_/\\/_/\\/_____/\\/_/   \\/_/\\/_/ /_/\\/_____/ ");
    SF_LOG_INFO("server.worldserver", "  Project SkyFireEmu 2011 - 2025(c) Open-sourced Game Emulation ");
    SF_LOG_INFO("server.worldserver", "           <http://www.projectskyfire.org/> \n");

    ///- Check the version of the configuration file
    uint32 confVersion = sConfigMgr->GetIntDefault("ConfVersion", 0);
    if (confVersion < SKYFIREWORLD_CONFIG_VERSION)
    {
        SF_LOG_INFO("server.worldserver", "*****************************************************************************");
        SF_LOG_INFO("server.worldserver", " WARNING: Your worldserver.conf version indicates your conf file is out of date!");
        SF_LOG_INFO("server.worldserver", "          Please check for updates, as your current default values may cause");
        SF_LOG_INFO("server.worldserver", "          strange behavior.");
        SF_LOG_INFO("server.worldserver", "*****************************************************************************");
    }

    /// worldserver PID file creation
    std::string pidFile = sConfigMgr->GetStringDefault("PidFile", "");
    if (!pidFile.empty())
    {
        if (uint32 pid = CreatePIDFile(pidFile))
            SF_LOG_INFO("server.worldserver", "Daemon PID: %u\n", pid);
        else
        {
            SF_LOG_ERROR("server.worldserver", "Cannot create PID file %s.\n", pidFile.c_str());
            return 1;
        }
    }

    ///- Start the databases
    if (!_StartDB())
        return 1;

    // set server offline (not connectable)
    for (std::map<uint32, std::string>::const_iterator itr = realmNameStore.begin(); itr != realmNameStore.end(); ++itr)
    {
        LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = (flag & ~%u) | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, REALM_FLAG_INVALID, itr->first);
    }

    ///- Initialize the World
    sWorld->SetInitialWorldSettings();

    ///- Initialize the signal handlers
    WorldServerSignalHandler signalINT, signalTERM;
#ifdef _WIN32
    WorldServerSignalHandler signalBREAK;
#endif /* _WIN32 */

    ///- Register worldserver's signal handlers
    ACE_Sig_Handler handle;
    handle.register_handler(SIGINT, &signalINT);
    handle.register_handler(SIGTERM, &signalTERM);
#ifdef _WIN32
    handle.register_handler(SIGBREAK, &signalBREAK);
#endif

    ///- Launch WorldRunnable thread
    ACE_Based::Thread worldThread(new WorldRunnable);
    worldThread.setPriority(ACE_Based::Highest);

    ACE_Based::Thread* cliThread = NULL;

#ifdef _WIN32
    if (sConfigMgr->GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfigMgr->GetBoolDefault("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        cliThread = new ACE_Based::Thread(new CliRunnable);
    }

    ACE_Based::Thread rarThread(new RARunnable);

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
                SF_LOG_ERROR("server.worldserver", "Processors marked in UseProcessors bitmask (hex) %x are not accessible for the worldserver. Accessible processors bitmask (hex): %x", affinity, appAff);
            else if (SetProcessAffinityMask(hProcess, currentAffinity))
                SF_LOG_INFO("server.worldserver", "Using processors (bitmask, hex): %x", currentAffinity);
            else
                SF_LOG_ERROR("server.worldserver", "Can't set used processors (hex): %x", currentAffinity);
        }
    }

    if (highPriority)
    {
        if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
            SF_LOG_INFO("server.worldserver", "worldserver process priority class set to HIGH");
        else
            SF_LOG_ERROR("server.worldserver", "Can't set worldserver process priority class.");
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
            SF_LOG_ERROR("server.worldserver", "Can't set used processors (hex): %x, error: %s", affinity, strerror(errno));
        else
        {
            CPU_ZERO(&mask);
            sched_getaffinity(0, sizeof(mask), &mask);
            SF_LOG_INFO("server.worldserver", "Using processors (bitmask, hex): %lx", *(__cpu_mask*)(&mask));
        }
    }

    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
            SF_LOG_ERROR("server.worldserver", "Can't set worldserver process priority class, error: %s", strerror(errno));
        else
            SF_LOG_INFO("server.worldserver", "worldserver process priority class set to %i", getpriority(PRIO_PROCESS, 0));
    }

#endif
#endif

    //Start soap serving thread
    ACE_Based::Thread* soapThread = NULL;

    if (sConfigMgr->GetBoolDefault("SOAP.Enabled", false))
    {
        SFSoapRunnable* runnable = new SFSoapRunnable();
        runnable->SetListenArguments(sConfigMgr->GetStringDefault("SOAP.IP", "127.0.0.1"), uint16(sConfigMgr->GetIntDefault("SOAP.Port", 7878)));
        soapThread = new ACE_Based::Thread(runnable);
    }

    ///- Start up freeze catcher thread
    if (uint32 freezeDelay = sConfigMgr->GetIntDefault("MaxCoreStuckTime", 0))
    {
        FreezeDetectorRunnable* fdr = new FreezeDetectorRunnable();
        fdr->SetDelayTime(freezeDelay * 1000);
        ACE_Based::Thread freezeThread(fdr);
        freezeThread.setPriority(ACE_Based::Highest);
    }

    ///- Launch the world listener socket
    uint16 worldPort = uint16(sWorld->getIntConfig(WorldIntConfigs::CONFIG_PORT_WORLD));
    std::string bindIp = sConfigMgr->GetStringDefault("BindIP", "0.0.0.0");

    if (sWorldSocketMgr->StartNetwork(worldPort, bindIp.c_str()) == -1)
    {
        SF_LOG_ERROR("server.worldserver", "Failed to start network");
        World::StopNow(ERROR_EXIT_CODE);
        // go down and shutdown the server
    }

    // set server online (allow connecting now)
    for (std::map<uint32, std::string>::const_iterator itr = realmNameStore.begin(); itr != realmNameStore.end(); ++itr)
    {
        LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag & ~%u, population = 0 WHERE id = '%u'", REALM_FLAG_INVALID, itr->first);
    }

    //TODO: FIX ME revision.
    //SF_LOG_INFO("server.worldserver", "%s (worldserver-daemon) ready...", _FULLVERSION);

    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    worldThread.wait();
    rarThread.wait();

    if (soapThread)
    {
        soapThread->wait();
        soapThread->destroy();
        delete soapThread;
    }

    // set server offline
    for (std::map<uint32, std::string>::const_iterator itr = realmNameStore.begin(); itr != realmNameStore.end(); ++itr)
    {
        LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, itr->first);
    }

    ///- Clean database before leaving
    ClearOnlineAccounts();

    _StopDB();

    SF_LOG_INFO("server.worldserver", "Halting process...");

    if (cliThread)
    {
#ifdef _WIN32

        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[4];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        WriteConsoleInput(hStdIn, b, 4, &numb);

        cliThread->wait();

#else

        cliThread->wait();
        cliThread->destroy();

#endif

        delete cliThread;
    }

    // for some unknown reason, unloading scripts here and not in worldrunnable
    // fixes a memory leak related to detaching threads from the module
    //UnloadScriptingModule();

    // Exit the process with specified return value
    return World::GetExitCode();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    MySQL::Library_Init();

    std::string dbString;
    uint8 asyncThreads, synchThreads;

    if (_noUseConfigDatabaseInfo == false)
    {
        dbString = sConfigMgr->GetStringDefault("WorldDatabaseInfo", "");
        if (dbString.empty())
        {
            SF_LOG_ERROR("server.worldserver", "World database not specified in configuration file");
            return false;
        }
    }

    asyncThreads = uint8(sConfigMgr->GetIntDefault("WorldDatabase.WorkerThreads", 1));
    if (asyncThreads < 1 || asyncThreads > 32)
    {
        SF_LOG_ERROR("server.worldserver", "World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synchThreads = uint8(sConfigMgr->GetIntDefault("WorldDatabase.SynchThreads", 1));

    if (_noUseConfigDatabaseInfo == false)
    {

        ///- Initialize the world database
        if (!WorldDatabase.Open(dbString, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to world database %s", dbString.c_str());
            return false;
        }
    }
    else
    {
        if (!WorldDatabase.Open(_dbHost, _dbPort, _dbUser, _dbPassword, _worldDB, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to world database %s, %s, %s, %s, %s", _dbHost, _dbPort, _dbUser, _dbPassword, _worldDB);
            return false;
        }
    }

    if (_noUseConfigDatabaseInfo == false)
    {
        ///- Get character database info from configuration file
        dbString = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        if (dbString.empty())
        {
            SF_LOG_ERROR("server.worldserver", "Character database not specified in configuration file");
            return false;
        }
    }

    asyncThreads = uint8(sConfigMgr->GetIntDefault("CharacterDatabase.WorkerThreads", 1));
    if (asyncThreads < 1 || asyncThreads > 32)
    {
        SF_LOG_ERROR("server.worldserver", "Character database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synchThreads = uint8(sConfigMgr->GetIntDefault("CharacterDatabase.SynchThreads", 2));

    if (_noUseConfigDatabaseInfo == false)
    {
        ///- Initialize the Character database
        if (!CharacterDatabase.Open(dbString, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to Character database%s, %s", dbString.c_str());
            return false;
        }
    }
    else
    {
        ///- Initialize the Character database
        if (!CharacterDatabase.Open(_dbHost, _dbPort, _dbUser, _dbPassword, _charactersDB, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to Character database%s, %s, %s, %s, %s", _dbHost, _dbPort, _dbUser, _dbPassword, _charactersDB);
            return false;
        }
    }

    if (_noUseConfigDatabaseInfo == false)
    {
        ///- Get login database info from configuration file
        dbString = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        if (dbString.empty())
        {
            SF_LOG_ERROR("server.worldserver", "Login database not specified in configuration file");
            return false;
        }
    }

    asyncThreads = uint8(sConfigMgr->GetIntDefault("LoginDatabase.WorkerThreads", 1));
    if (asyncThreads < 1 || asyncThreads > 32)
    {
        SF_LOG_ERROR("server.worldserver", "Login database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synchThreads = uint8(sConfigMgr->GetIntDefault("LoginDatabase.SynchThreads", 1));

    if (_noUseConfigDatabaseInfo == false)
    {
        ///- Initialise the login database
        if (!LoginDatabase.Open(dbString, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to login database %s", dbString.c_str());
            return false;
        }
    }
    else
    {
        if (!LoginDatabase.Open(_dbHost, _dbPort, _dbUser, _dbPassword, _authDB, asyncThreads, synchThreads))
        {
            SF_LOG_ERROR("server.worldserver", "Cannot connect to database%s %s, %s, %s, %s", _dbHost, _dbPort, _dbUser, _dbPassword, _authDB);
            return false;
        }
    }

    // Load realm names into a store
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST);
    stmt->setInt32(0, sConfigMgr->GetIntDefault("WorldServerPort", 8085));
    PreparedQueryResult result = LoginDatabase.Query(stmt);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            realmNameStore[fields[0].GetUInt32()] = fields[1].GetString(); // Store the realm name into the store
        } while (result->NextRow());
    }
    for (std::map<uint32, std::string>::const_iterator itr = realmNameStore.begin(); itr != realmNameStore.end(); ++itr)
    {
        SF_LOG_INFO("server.worldserver", "World running as realm ID %d", itr->first);
    }

    ///- Clean the database before starting
    ClearOnlineAccounts();

    ///- Insert version info into DB
    //TODO: FIX ME revision
    //WorldDatabase.PExecute("UPDATE version SET core_version = '%s', core_revision = '%s'", _FULLVERSION, _HASH);        // One-time query

    sWorld->LoadDBVersion();

    SF_LOG_INFO("server.worldserver", "Using World DB: %s", sWorld->GetDBVersion());
    return true;
}

void Master::_StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();

    MySQL::Library_End();
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::ClearOnlineAccounts()
{
    // Reset online status for all accounts with characters on the current realm
    for (std::map<uint32, std::string>::const_iterator itr = realmNameStore.begin(); itr != realmNameStore.end(); ++itr)
    {
        LoginDatabase.DirectPExecute("UPDATE account SET online = 0 WHERE online > 0 AND id IN (SELECT acctid FROM realmcharacters WHERE realmid = %d)", itr->first);
    }

    // Reset online status for all characters
    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");

    // Battleground instance ids reset at server restart
    CharacterDatabase.DirectExecute("UPDATE character_battleground_data SET instanceId = 0");
}
