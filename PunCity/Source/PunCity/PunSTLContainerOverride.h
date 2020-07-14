// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include <vector>
#include <string>
#include <unordered_map>


enum class PunSimLLMTag : uint8
{
	Simulation,
	Trees,
	Terrain,

	PathAI,
	Flood,
};

static const std::vector<std::string> PunSimLLMTagNames
{
	"Simulation",
	"Trees",
	"Terrain",

	"PathAI",
	"Flood",
};
static const int32 PunSimLLMTagCount = PunSimLLMTagNames.size();
static const std::string& PunSimLLMTagName(PunSimLLMTag tag) {
	check(static_cast<int>(tag) < PunSimLLMTagNames.size());
	return PunSimLLMTagNames[static_cast<int>(tag)];
}

#define PUN_LLM_ON 0

#if PUN_LLM_ON

//class PunScopeLLM
//{
//public:
//	static size_t memoryTotal;
//	static std::vector<size_t> memoryTotals;
//	static std::vector<bool> memoryTagOn;
//
//	static void InitPunLLM()
//	{
//		memoryTotal = 0;
//		memoryTotals.resize(PunSimLLMTagCount);
//		memoryTagOn.resize(PunSimLLMTagCount);
//		UE_LOG(LogTemp, Error, TEXT("InitPunLLM"));
//	}
//
//	PunScopeLLM(PunSimLLMTag tag);
//	~PunScopeLLM();
//
//	static void Print();
//
//
//	PunSimLLMTag tag = PunSimLLMTag::Simulation;
//
//	static void PunTrackAlloc(const void* ptr, size_t byteSize);
//	static void PunTrackFree(const void* ptr, size_t byteSize);
//};
//
//#define PUN_LLM(tag) PunScopeLLM PREPROCESSOR_JOIN(PunScopeLLMObj, __LINE__)(tag);

#define PUN_LLM_ONLY(statement) statement

#else

#define PUN_LLM(tag)
#define PUN_LLM_ONLY(statement) 

#endif


#define USE_CUSTOM_ALLOCATOR 0

#if USE_CUSTOM_ALLOCATOR

//template <class T>
//struct Mallocator
//{
//	typedef T value_type;
//
//	Mallocator() = default;
//	template <class U> constexpr Mallocator(const Mallocator <U>&) noexcept {}
//
//	__declspec(allocator) T* allocate(std::size_t n) {
//		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
//			throw std::bad_alloc();
//
//		if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
//		{
//			//pHeapTracker->AllocateEvent(p, n);
//			PunScopeLLM::PunTrackAlloc(p, n * sizeof(T));
//			return p;
//		}
//
//		throw std::bad_alloc();
//	}
//	__declspec(allocator) void deallocate(T* p, std::size_t n) noexcept
//	{
//		//pHeapTracker->DeallocateEvent(p);
//		PunScopeLLM::PunTrackFree(p, n * sizeof(T));
//		std::free(p);
//	}
//};
//
//template <class T, class U>
//bool operator==(const Mallocator <T>& a, const Mallocator <U>& b) { return a == b; }
//template <class T, class U>
//bool operator!=(const Mallocator <T>& a, const Mallocator <U>& b) { return a != b; }
//
//
//template<typename _Ty>
//using std::vector = std::vector<_Ty, Mallocator<_Ty>>;

#else

//template<typename _Ty>
//using vector_pun = std::vector<_Ty>;

#endif