// Fill out your copyright notice in the Description page of Project Settings.

#include "PunCity/PunTimer.h"

DEFINE_LOG_CATEGORY(LogTimer);

std::unordered_map<std::string, std::chrono::nanoseconds> ScopeTimerLoop::time_spans;


std::vector<LeanProfilerElement> LeanProfiler::EnumToElements;
std::vector<LeanProfilerElement> LeanProfiler::LastEnumToElements;