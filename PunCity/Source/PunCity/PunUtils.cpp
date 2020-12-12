// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/PunUtils.h"

DEFINE_LOG_CATEGORY(PunInit);
DEFINE_LOG_CATEGORY(PunSound);
DEFINE_LOG_CATEGORY(PunAsset);
DEFINE_LOG_CATEGORY(PunSaveLoad);
DEFINE_LOG_CATEGORY(PunDisplay);

DEFINE_LOG_CATEGORY(PunTrailer);

DEFINE_LOG_CATEGORY(PunTick);
DEFINE_LOG_CATEGORY(PunPlayerOwned);
DEFINE_LOG_CATEGORY(PunResource);
DEFINE_LOG_CATEGORY(PunBuilding);

DEFINE_LOG_CATEGORY(PunTerrain);

DEFINE_LOG_CATEGORY(PunTickHash);
DEFINE_LOG_CATEGORY(PunSync);

DEFINE_LOG_CATEGORY(PunInput);
DEFINE_LOG_CATEGORY(PunNetwork);
DEFINE_LOG_CATEGORY(PunAI);

DEFINE_LOG_CATEGORY(PunRefreshJob);

DEFINE_LOG_CATEGORY(PunSaveCheck);

#if DEBUG_BUILD
std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<PunALog::Pair>>> PunALog::Logs;

std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<FString>>> PunALog::TickLogs;
#endif