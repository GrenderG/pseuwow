#include "common.h"
#include "PseuWoW.h"
#include <time.h>
#include <openssl/rand.h>

#include "Auth/ByteBuffer.h"
#include "DefScript/DefScript.h"
#include "DefScriptInterface.h"
#include "Auth/BigNumber.h"
#include "Auth/ByteBuffer.h"
#include "DefScript/DefScript.h"
#include "Realm/RealmSocket.h"
#include "World/WorldSession.h"
#include "Cli.h"


//###### Start of program code #######

PseuInstanceRunnable::PseuInstanceRunnable()
{
}

void PseuInstanceRunnable::run(void)
{
    _i = new PseuInstance(this);
	_i->SetConfDir("./conf/");
    _i->SetScpDir("./scripts/");
	_i->Init();
	// more
	_i->Run();
    delete _i;
}

void PseuInstanceRunnable::sleep(uint32 msecs)
{
    ZThread::Thread::sleep(msecs);
}

PseuInstance::PseuInstance(PseuInstanceRunnable *run)
{
    _runnable=run;
    _ver="PseuWoW Alpha Build 12 dev 2" DEBUG_APPENDIX;
    _ver_short="A12-dev1" DEBUG_APPENDIX;
    _wsession=NULL;
    _rsession=NULL;
    _scp=NULL;
    _conf=NULL;
    _cli=NULL;
    _stop=false;
    _fastquit=false;
    _startrealm=true;
    createWorldSession=false;
    _error=false;


}

PseuInstance::~PseuInstance()
{
    if(GetConf()->enablecli && _cli)
    {
        _cli->stop();
    }
    delete _scp;
	delete _conf;
	//delete _rsession; // deleted by SocketHandler!!!!!
	delete _wsession;
	log_close();
    
}

bool PseuInstance::Init(void) {
    log_prepare("logfile.txt",this);

    if(_confdir.empty())
        _confdir="./conf/";
    if(_scpdir.empty())
        _scpdir="./scp/";

	srand((unsigned)time(NULL));
	RAND_set_rand_method(RAND_SSLeay()); // init openssl randomizer

	if(SDL_Init(0)==-1) {
			log("SDL_Init: %s", SDL_GetError());
			return false;
		}
	
	_scp=new DefScriptPackage();
    _scp->SetParentMethod((void*)this);
	_conf=new PseuInstanceConf();	
	
    log("Reading PseuWoW.conf...");
	if(!_scp->variables.ReadVarsFromFile(_confdir + "PseuWoW.conf"))
	{
		log("Error reading conf file [%s]",_confdir.append("PseuWoW.conf").c_str()); 
		return false;
	}
    logdetail("Applying configuration...");
	_conf->ApplyFromVarSet(_scp->variables);
	
    log("Reading user permissions...");
    if(_scp->variables.ReadVarsFromFile(_confdir + "users.conf"))
	{
		log("-> Done reading users.");
	}
	else
	{
		log("Error reading conf file [%s] - NO PERMISSIONS SET!",_confdir.append("users.conf").c_str()); 
	}


    logdebug("Setting up DefScripts path '%s'",_scpdir.c_str());
    _scp->SetPath(_scpdir);

    logdebug("Setting up predefined DefScript macros...");
    _scp->variables.Set("@version_short",_ver_short);
    _scp->variables.Set("@version",_ver);

    logdetail("Applying user permissions...");
    _scp->My_LoadUserPermissions(_scp->variables);

    log("Loading DefScripts from folder '%s'",_scpdir.c_str());
    if(!_scp->RunScript("_startup",NULL))
    {
        printf("Error executing '_startup.def'");
    }

    if(GetConf()->enablecli)
    {
        log("Starting CLI...");
        _cli = new CliRunnable(this);
        ZThread::Thread t(_cli);
    }

    if(_stop){
			log("Errors while initializing, proc halted!!");
            if(GetConf()->exitonerror)
                exit(0);
			while(true)SDL_Delay(1000);
    }

    log("Init complete.\n");
	_initialized=true; 
	return true;
}

void PseuInstance::Run(void)
{
    do
    {
        if(!_initialized)
            return;

        _rsession=new RealmSocket(_sh);
        _rsession->SetDeleteByHandler();
        _rsession->SetHost(GetConf()->realmlist);
        _rsession->SetPort(GetConf()->realmport);
        _rsession->SetInstance(this);
        _rsession->Start();
        
        if(_rsession->IsValid())
        {
            _sh.Add(_rsession);
            _sh.Select(1,0);
        }
        _startrealm=false; // the realm is started now

        while((!_stop) && (!_startrealm))
        {
            Update();
        }
    }
    while(GetConf()->reconnect && (!_stop));

    if(_fastquit)
    {
        log("Aborting Instance...");
        return;
    }
    log("Shutting down instance...");

    SaveAllCache();

    if(GetConf()->exitonerror == false && _error)
    {
        log("Exiting on error is disabled, PseuWoW is now IDLE");
        log("-- Press enter to exit --");
        char crap[100];
        fgets(crap,sizeof(crap),stdin); 
    }

}

void PseuInstance::Update()
{
    if(_sh.GetCount())
        _sh.Select(0,0); // update the realmsocket

    if(createWorldSession && (!_wsession))
    {
        createWorldSession=false;
        _wsession=new WorldSession(this);
    }
    if(_wsession && !_wsession->IsValid())
    {
        _wsession->Start(); // update the worldSESSION, which will update the worldsocket itself
    }
    if(_wsession && _wsession->IsValid())
    {
        _wsession->Update(); // update the worldSESSION, which will update the worldsocket itself
    }
    if(_wsession && _wsession->DeleteMe())
    {
        delete _wsession;
        _wsession=NULL;
        _startrealm=true;
        this->Sleep(1000); // wait 1 sec before reconnecting
        return;
    }

    this->Sleep(GetConf()->networksleeptime);
}

void PseuInstance::SaveAllCache(void)
{
    //...
    if(GetWSession() && GetWSession()->IsValid())
    {
        GetWSession()->plrNameCache.SaveToFile();
        //...
    }
}

void PseuInstance::Sleep(uint32 msecs)
{
    GetRunnable()->sleep(msecs);
}



PseuInstanceConf::PseuInstanceConf()
{
}

void PseuInstanceConf::ApplyFromVarSet(VarSet &v)
{
    debug=atoi(v.Get("DEBUG").c_str());
    realmlist=v.Get("REALMLIST");
	accname=v.Get("ACCNAME");
	accpass=v.Get("ACCPASS");
	exitonerror=(bool)atoi(v.Get("EXITONERROR").c_str());
    reconnect=(bool)atoi(v.Get("RECONNECT").c_str());
	realmport=atoi(v.Get("REALMPORT").c_str());
    clientversion_string=v.Get("CLIENTVERSION");
	clientbuild=atoi(v.Get("CLIENTBUILD").c_str());
	clientlang=v.Get("CLIENTLANGUAGE");
	realmname=v.Get("REALMNAME");
	charname=v.Get("CHARNAME");
	networksleeptime=atoi(v.Get("NETWORKSLEEPTIME").c_str());
    showopcodes=atoi(v.Get("SHOWOPCODES").c_str());
    enablecli=(bool)atoi(v.Get("ENABLECLI").c_str());
    allowgamecmd=(bool)atoi(v.Get("ALLOWGAMECMD").c_str());
	enablechatai=(bool)atoi(v.Get("ENABLECHATAI").c_str());

    // clientversion is a bit more complicated to add
    {
        std::string opt=clientversion_string + ".";
        std::string num;
        uint8 p=0;
        for(uint8 i=0;i<opt.length();i++)
        {
            if(!isdigit(opt.at(i)))
            {
                clientversion[p]=(unsigned char)atoi(num.c_str());
                num.clear();
                p++;
                if(p>2)
                    break;
                continue;
            }
            num+=opt.at(i);
        }
    }
}




PseuInstanceConf::~PseuInstanceConf()
{
	//...
}


	

