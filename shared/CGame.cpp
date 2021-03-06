/*****************************************************************************
*
*  PROJECT:     Open Faction
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        shared/CGame.cpp
*  PURPOSE:     Main class containing references to all useful objects
*  DEVELOPERS:  Rafal Harabien
*
*****************************************************************************/

#include "CGame.h"
#include "CLevel.h"
#include "CMeshMgr.h"
#include "CMaterialsMgr.h"
#include "CAnimMgr.h"
#include "CVirtualFileSystem.h"
#include "CException.h"
#include "utils.h"
#include "CEventsHandler.h"
#include "CConsole.h"
#include "CString.h"

#include "CStringsTable.h"
#include "CAmmoTable.h"
#include "CWeaponsTable.h"
#include "CItemsTable.h"
#include "CEntitiesTable.h"
#include "CClutterTable.h"
#include "CGameTable.h"
#include "CMpCharactersTable.h"
#include "CFoleyTable.h"
#ifdef OF_CLIENT
# include "CSoundManager.h"
#endif // OF_CLIENT

using namespace std;

CEventsHandler CEventsHandler::Default;

CGame::CGame(CConsole *pConsole, irr::IrrlichtDevice *pIrrDevice):
    m_pConsole(pConsole), m_pEventsHandler(&CEventsHandler::Default), m_pIrrDevice(pIrrDevice),
    m_pStringsTbl(NULL), m_pAmmoTbl(NULL), m_pWeaponsTbl(NULL), m_pItemsTbl(NULL), m_pEntitiesTbl(NULL),
    m_pClutterTbl(NULL), m_pGameTbl(NULL), m_pMpCharactersTbl(NULL), m_pFoleyTbl(NULL)
{
    m_pMaterialsMgr = new CMaterialsMgr(this);
    m_pMeshMgr = new CMeshMgr(this);
    m_pAnimMgr = new CAnimMgr(this);
#ifdef OF_CLIENT
    m_pSoundMgr = new CSoundManager(this);
#else
    m_pSoundMgr = NULL;
#endif // OF_CLIENT

    m_pVfs = &CVirtualFileSystem::GetInst();
    m_pLevel = new CLevel(this);
    
    m_pCamera = NULL;
    
#ifdef OF_CLIENT
    if(m_pIrrDevice)
        m_pIrrDevice->grab();
#endif // OF_CLIENT
}

CGame::~CGame()
{
    delete m_pLevel;
    delete m_pMeshMgr;
    delete m_pMaterialsMgr;
    delete m_pAnimMgr;
    
#ifdef OF_CLIENT
    delete m_pSoundMgr;
    
    if(m_pIrrDevice)
        m_pIrrDevice->drop();
#endif // OF_CLIENT
}

void CGame::InitVfs()
{
#ifndef LINUX
    const CString strRootDir = "C:/games/RedFaction/";
#else
    const CString strRootDir = ""; // current directory
#endif
    
    /* Add custom maps first */
    m_pVfs->AddArchivesDirectory(strRootDir + "user_maps/multi/");
    m_pVfs->AddArchivesDirectory(strRootDir + "user_maps/single/");
    
    /* Add game archives */
    const char *pGameArchives[] = {
#ifdef OF_CLIENT
        "audio.vpp",
#endif // OF_CLIENT
        "levels1.vpp", "levels2.vpp", "levels3.vpp", "levelsm.vpp",
        "maps1.vpp", "maps2.vpp", "maps3.vpp", "maps4.vpp", "maps_en.vpp",
        "meshes.vpp", "motions.vpp",
#ifdef OF_CLIENT
        "music.vpp",
#endif // OF_CLIENT
        "tables.vpp",
#ifdef OF_CLIENT
        "ui.vpp",
#endif // OF_CLIENT
    };
    
    /* Add VPP files to virtual file system */
    for(unsigned i = 0; i < COUNTOF(pGameArchives); ++i)
    {
        m_pVfs->AddArchive(strRootDir + pGameArchives[i]);
        m_pConsole->DbgPrint("Loaded %s\n", pGameArchives[i]);
    }
        
}

void CGame::LoadTables()
{
    CVfsFileStream Stream;
    
    /* Load strings table */
    m_pStringsTbl = new CStringsTable;
    Stream.Open("strings.tbl");
    assert(Stream.good());
    int iStatus = m_pStringsTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load strings.tbl");
    
    /* Load ammo table */
    m_pAmmoTbl = new CAmmoTable;
    Stream.Open("ammo.tbl");
    assert(Stream.good());
    iStatus = m_pAmmoTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load ammo.tbl");
    
    /* Load weapons table */
    m_pWeaponsTbl = new CWeaponsTable(this);
    Stream.Open("weapons.tbl");
    iStatus = m_pWeaponsTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load weapons.tbl");
    
    /* Load items table */
    m_pItemsTbl = new CItemsTable(this);
    Stream.Open("items.tbl");
    iStatus = m_pItemsTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load items.tbl");
    
    /* Load entities table */
    m_pEntitiesTbl = new CEntitiesTable;
    Stream.Open("entity.tbl");
    iStatus = m_pEntitiesTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load entity.tbl");
    
    /* Load clutter table */
    m_pClutterTbl = new CClutterTable;
    Stream.Open("clutter.tbl");
    iStatus = m_pClutterTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load clutter.tbl");
    
    /* Load game table */
    m_pGameTbl = new CGameTable;
    Stream.Open("game.tbl");
    iStatus = m_pGameTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load game.tbl");
    
    /* Load multiplayer characters table */
    m_pMpCharactersTbl = new CMpCharactersTable(this);
    Stream.Open("pc_multi.tbl");
    iStatus = m_pMpCharactersTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load pc_multi.tbl");
    
    /* Load foley sounds table */
    m_pFoleyTbl = new CFoleyTable;
    Stream.Open("foley.tbl");
    iStatus = m_pFoleyTbl->Load(Stream);
    if(iStatus < 0)
        THROW_EXCEPTION("Failed to load pc_multi.tbl");
}

void CGame::LoadMod(const char *pszModName)
{
    CString strPath;
    strPath.Format("mods/%s/", pszModName);
    m_pVfs->AddArchivesDirectory(strPath);
}

void CGame::LoadLevel(const char *pszFilename)
{
    CVfsFileStream Stream;
    Stream.Open(pszFilename);
    m_pLevel->Load(Stream);
}
