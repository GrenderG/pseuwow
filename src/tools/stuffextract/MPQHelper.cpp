#include <vector>
#include "common.h"
#include "MPQHelper.h"
#include "MPQFile.h"
#include "Locale.h"

#define DATADIR "Data"


MPQHelper::MPQHelper(char *archive)
{
    // TODO: check which files are needed and which are not + recheck for correct ordering
    std::string dir = "Data/";
    std::string ext = ".MPQ";
    std::string ldir = dir + GetLocale() + "/";

    // order goes from last opened to first opened file
    // ok maybe this is a bit too much but should work fine :)
    _patches.push_front(dir+"common"+ext);
    _patches.push_front(dir+"expansion"+ext);
    for(uint32 i=1; i<=MAX_PATCH_NUMBER; i++)
    {
        char buf[200];
        sprintf(buf,"%spatch-%u%s",dir.c_str(),i,ext.c_str());
        _patches.push_front(buf);
    }
    _patches.push_front(dir+"patch"+ext);
    _patches.push_front(ldir+archive+"-"+GetLocale()+ext);
    _patches.push_front(ldir+"locale-"+GetLocale()+ext);
    _patches.push_front(ldir+"expansion-locale-"+GetLocale()+ext);	
    _patches.push_front(ldir+"expansion-"+archive+"-"+GetLocale()+ext);

    _patches.push_front(ldir+"patch"+"-"+GetLocale()+ext);
    for(uint32 i=1; i<=MAX_PATCH_NUMBER; i++)
    {
        char buf[200];
        sprintf(buf,"%spatch-%s-%u%s",ldir.c_str(),GetLocale(),i,ext.c_str());
        //if(FileExists(buf))
        _patches.push_front(buf);
    }

    for(std::list<std::string>::iterator it=_patches.begin(); it != _patches.end(); it++)
    {
        if(::FileExists(*it))
        {
            _files.push_back(new MPQFile((*it).c_str()));
        }
    }
    _files.push_back(new MPQFile(archive));
}

MPQHelper::~MPQHelper()
{
    for(std::list<MPQFile*>::iterator it=_files.begin(); it != _files.end(); it++)
    {
        (*it)->Close();
        delete *it;
    }
}

ByteBuffer MPQHelper::ExtractFile(char* fn)
{
    ByteBuffer bb;
    for(std::list<MPQFile*>::iterator i = _files.begin(); i != _files.end(); i++)
    {
        MPQFile *mpq = *i;
        if(mpq->IsOpen() && mpq->HasFile(fn) && mpq->GetFileSize(fn) > 0)
        {
           // printf("MPQE: Using %s from %s\n",fn,i->c_str());
            bb = mpq->ReadFile(fn);
            return bb;
        }
    }
    return bb; // will be empty if returned here
}

bool MPQHelper::FileExists(char *fn)
{
    for(std::list<MPQFile*>::iterator i = _files.begin(); i != _files.end(); i++)
    {
        if((*i)->IsOpen() && (*i)->HasFile(fn) && (*i)->GetFileSize(fn) > 0)
            return true;
    }
    return false;
}



