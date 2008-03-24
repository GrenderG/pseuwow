#ifndef ADTFILE_H
#define ADTFILE_H

#include <set>

#define CHUNKS_PER_TILE 256

#include "ADTFileStructs.h"

class ADTFile
{
public:
    bool Load(std::string);
    bool LoadMem(ByteBuffer&);

    ADTMapChunk _chunks[CHUNKS_PER_TILE]; // 16x16
    std::vector<std::string> _textures;
    std::vector<std::string> _wmos;
    std::vector<std::string> _models;
    std::vector<MDDF_chunk> _doodadsp;
    std::vector<MODF_chunk> _wmosp;
    std::vector<MCSE_chunk> _soundemm;
    MHDR_chunk mhdr;
    MCIN_chunk mcin[CHUNKS_PER_TILE];
    uint32 _version;
};

void ADT_ExportStringSetByOffset(const uint8*, uint32, std::set<std::string>&, char*);
void ADT_FillTextureData(const uint8*,std::set<std::string>&);
void ADT_FillWMOData(const uint8*,std::set<std::string>&);
void ADT_FillModelData(const uint8*,std::set<std::string>&);

#endif
