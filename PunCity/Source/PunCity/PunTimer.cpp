// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/PunTimer.h"

DEFINE_LOG_CATEGORY(LogTimer);

std::unordered_map<std::string, nanoseconds> ScopeTimerLoop::time_spans;