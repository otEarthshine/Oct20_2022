// Pun Dumnernchanvanit's


#include "PunCity/PunSTLContainerOverride.h"
#include "PunCity/PunUtils.h"

#if !defined(ToTChar)
#define ToTChar(stdString) UTF8_TO_TCHAR(stdString.c_str())
#endif

#if PUN_LLM_ON

size_t PunScopeLLM::memoryTotal = 0;
std::vector<size_t> PunScopeLLM::memoryTotals;
std::vector<bool> PunScopeLLM::memoryTagOn;


PunScopeLLM::PunScopeLLM(PunSimLLMTag tag)
				: tag(tag)
{
	memoryTagOn[static_cast<int>(tag)] = true;
	
	size_t memory = memoryTotals[static_cast<int>(tag)];
	//PUN_LOG("LLM1 %s size:%llu total:%llu", ToTChar(PunSimLLMTagName(tag)), memory, memoryTotal);
}

PunScopeLLM::~PunScopeLLM()
{
	memoryTagOn[static_cast<int>(tag)] = false;
	
	size_t memory = memoryTotals[static_cast<int>(tag)];
	//PUN_LOG("LLM2 %s size:%llu total:%llu", ToTChar(PunSimLLMTagName(tag)), memory, memoryTotal);
}

void PunScopeLLM::Print()
{
	PUN_DEBUG2("Simulation LLM:");
	for (size_t i = 0; i < memoryTagOn.size(); i++) {
		PUN_DEBUG2("-  %s size:%llu total:%llu", ToTChar(PunSimLLMTagNames[i]), memoryTotals[i], memoryTotal);
	}
}


void PunScopeLLM::PunTrackAlloc(const void* ptr, size_t byteSize)
{
	for (size_t i = 0; i < memoryTagOn.size(); i++) {
		if (memoryTagOn[i]) {
			memoryTotals[i] += byteSize;
		}
	}

	memoryTotal += byteSize;
}
void PunScopeLLM::PunTrackFree(const void* ptr, size_t byteSize)
{
	if (memoryTagOn.size() > 0 &&
		memoryTotals.size() > 0)
	{
		for (size_t i = 0; i < memoryTagOn.size(); i++) {
			if (memoryTagOn[i]) {
				memoryTotals[i] -= byteSize;
			}
		}
		memoryTotal -= byteSize;
	}
}

#endif