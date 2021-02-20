// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "PunCity/PunSTLContainerOverride.h"
#include <unordered_set>
#include <unordered_map>
#include <memory>

#if !defined(PUN_CHECK_SERIAL)
	#if WITH_EDITOR
		#define PUN_CHECK_SERIAL(x) if (!(x)) { FDebug::DumpStackTraceToLog(); checkNoEntry(); }
	#else
		#define PUN_CHECK_SERIAL(x) if (!(x)) { FDebug::DumpStackTraceToLog(); UE_LOG(LogTemp, Error, TEXT("PUN_ERROR")); }
	#endif
#endif

/*
 * Serialization
 */

inline void SerializeStr(FArchive& Ar, std::string& str)
{
	FString fString;
	if (Ar.IsSaving()) {
		fString = FString((str).c_str());
	}
	Ar << fString;
	if (Ar.IsLoading()) {
		str = std::string(TCHAR_TO_UTF8(*(fString)));
	}
}


//inline void SerializeBool(FArchive& Ar, bool& boolVal)
//{
//	int32 int32Val = 0;
//	if (Ar.IsSaving()) {
//		int32Val = boolVal;
//	}
//	Ar << int32Val;
//	if (Ar.IsLoading()) {
//		boolVal = int32Val;
//	}
//}


template<typename T>
std::vector<T> SetToVector(std::unordered_set<T>& s) {
	return std::vector<T>(s.begin(), s.end());
}

template<typename K, typename V>
std::vector<std::pair<K, V>> MapToVector(std::unordered_map<K, V>& map) {
	return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

template<typename TValue>
void SerializeVecValue(FArchive& Ar, std::vector<TValue>& v)
{
	// Save size for variable sized vectors
	size_t size;
	size_t elementSize;
	if (Ar.IsSaving()) {
		size = v.size();
		elementSize = size > 0 ? sizeof(v[0]) : 0;
	}
	Ar << size;
	Ar << elementSize;

	check(size >= 0);
	check(size < 32000000 * 4);
	check(elementSize >= 0);
	check(elementSize <= 64);
	
	if (Ar.IsLoading()) {
		v.resize(size);
	}
	
	Ar.Serialize(v.data(), size * elementSize);
}

// Vector bool is a packed array, unpack it before serializing it...
inline void SerializeVecBool(FArchive& Ar, std::vector<bool>& vecBool)
{
	// Save size for variable sized vectors
	std::vector<uint8> vecUint;
	if (Ar.IsSaving()) {
		vecUint.reserve(vecBool.size());
		for (const bool& boolValue : vecBool) {
			vecUint.push_back(boolValue);
		}
	}
	SerializeVecValue(Ar, vecUint);

	if (Ar.IsLoading()) {
		vecBool.clear();
		vecBool.reserve(vecUint.size());
		for (const uint8& uint : vecUint) {
			vecBool.push_back(uint);
		}
	}
}

template<typename T, typename Func>
void SerializeVecLoop(FArchive& Ar, std::vector<T>& v, Func func)
{
	check(&v != nullptr);
	
	size_t size;
	if (Ar.IsSaving()) {
		size = v.size();
	}
	Ar << size;
	check(size < 40000000);
	
	if (Ar.IsLoading()) {
		v.resize(size);
	}
	
	for (size_t i = 0; i < size; i++) {
		func(v[i]);
	}
}

template<typename K, typename TValue>
void SerializeMapValue(FArchive& Ar, std::unordered_map<K, TValue>& mapValue)
{
	std::vector<std::pair<K, TValue>> vecPair;
	if (Ar.IsSaving()) {
		vecPair = MapToVector(mapValue);
	}
	SerializeVecLoop(Ar, vecPair, [&](std::pair<K, TValue>& pair) {
		Ar << pair.first;
		Ar << pair.second;
	});

	if (Ar.IsLoading()) {
		for (std::pair<K, TValue>& pair : vecPair) {
			mapValue.insert(pair);
		}
	}
}

template<typename K, typename V, typename Func>
void SerializeMapLoop(FArchive& Ar, std::unordered_map<K, V>& map, Func serializeValue)
{
	std::vector<std::pair<K, V>> vecPair;
	if (Ar.IsSaving()) {
		vecPair = MapToVector(map);
	}
	SerializeVecLoop(Ar, vecPair, [&](std::pair<K, V>& pair) {
		Ar << pair.first;
		serializeValue(pair.second);
	});

	if (Ar.IsLoading()) {
		for (std::pair<K, V>& pair : vecPair) {
			map.insert(pair);
		}
	}
}

template<typename TValue>
void SerializeVecVecValue(FArchive& Ar, std::vector<std::vector<TValue>>& v)
{
	SerializeVecLoop(Ar, v, [&](std::vector<TValue>& obj) {
		SerializeVecValue(Ar, obj);
	});
}

template<typename TValue>
void SerializeSetValue(FArchive& Ar, std::unordered_set<TValue>& setValue)
{
	std::vector<TValue> vecValue;
	if (Ar.IsSaving()) {
		vecValue = SetToVector(setValue);
	}
	SerializeVecValue(Ar, vecValue);

	if (Ar.IsLoading()) {
		setValue.insert(vecValue.begin(), vecValue.end());
	}
}

template<typename TValue>
void SerializeVecSetValue(FArchive& Ar, std::vector<std::unordered_set<TValue>>& vecSetValue)
{
	SerializeVecLoop(Ar, vecSetValue, [&](std::unordered_set<TValue>& setValue) {
		SerializeSetValue(Ar, setValue);
	});
}

template<typename TValue>
void SerializeVecVecSetValue(FArchive& Ar, std::vector<std::vector<std::unordered_set<TValue>>>& vecVecSetValue)
{
	SerializeVecLoop(Ar, vecVecSetValue, [&](std::vector<std::unordered_set<TValue>>& vecSetValue) {
		check(&vecSetValue != nullptr);
		SerializeVecSetValue(Ar, vecSetValue);
	});
}

template<typename K, typename TValue>
void SerializeVecMapValue(FArchive& Ar, std::vector<std::unordered_map<K, TValue>>& vecMapValue)
{
	SerializeVecLoop(Ar, vecMapValue, [&](std::unordered_map<K, TValue>& mapValue) {
		check(&mapValue != nullptr);
		SerializeMapValue(Ar, mapValue);
	});
}
template<typename K, typename TValue>
void SerializeVecVecMapValue(FArchive& Ar, std::vector<std::vector<std::unordered_map<K, TValue>>>& vecVecMapValue)
{
	SerializeVecLoop(Ar, vecVecMapValue, [&](std::vector<std::unordered_map<K, TValue>>& vecMapValue) {
		check(&vecMapValue != nullptr);
		SerializeVecMapValue(Ar, vecMapValue);
	});
}


template<typename T, typename Func>
void SerializeTArrayLoop(FArchive& Ar, TArray<T>& v, Func func)
{
	check(&v != nullptr);

	size_t size;
	if (Ar.IsSaving()) {
		size = v.Num();
	}
	Ar << size;
	check(size < 40000000);

	if (Ar.IsLoading()) {
		v.SetNum(size);
	}

	for (size_t i = 0; i < size; i++) {
		func(v[i]);
	}
}



template<typename K, typename VObj>
void SerializeVecMapObj(FArchive& Ar, std::vector<std::unordered_map<K, VObj>>& vecMapObj)
{
	SerializeVecLoop(Ar, vecMapObj, [&](std::unordered_map<K, VObj>& mapObj) {
		std::vector<std::pair<K, VObj>> vecPair = MapToVector(mapObj);
		SerializeVecLoop(Ar, vecPair, [&](std::pair<K, VObj>& pair) {
			Ar << pair.first;
			pair.second >> Ar;
		});
	});
}

template<typename TObj>
void SerializeVecObj(FArchive& Ar, std::vector<TObj>& v) {
	SerializeVecLoop(Ar, v, [&](TObj& obj) {
		obj >> Ar;
	});
}

template<typename TObj>
void SerializeVecVecObj(FArchive& Ar, std::vector<std::vector<TObj>>& vecVecObj) {
	SerializeVecLoop(Ar, vecVecObj, [&](std::vector<TObj>& vecObj) {
		SerializeVecLoop(Ar, vecObj, [&](TObj& obj) {
			obj >> Ar;
		});
	});
}

template<typename TPtr, typename TClassEnum, typename PtrCreateFunc>
void SerializePtr(FArchive& Ar, TPtr& ptr, PtrCreateFunc ptrCreateFunc)
{
	TClassEnum classEnum;
	if (Ar.IsSaving()) {
		classEnum = ptr->classEnum();
	}
	Ar << classEnum;

	if (Ar.IsLoading()) {
		ptrCreateFunc(classEnum);
	}
	ptr->Serialize(Ar);
};

/*
 * Crc32
 */
template<typename T>
static int32 VecBytes(const std::vector<T>& v) {
	return sizeof(v[0]) * v.size();
}

template<typename T>
static int32 VecCRC32(const std::vector<T>& v) {
	return FCrc::MemCrc32(v.data(), VecBytes(v));
}

template<typename T>
static void SerializeCrc(FArchive &Ar, const std::vector<T>& v) {
	int32 crc = 0;
	if (Ar.IsSaving()) {
		crc = VecCRC32(v);
		Ar << crc;
	}
	else {
		// Load crc and compare to the current crc
		Ar << crc;
		int32 newCrc = VecCRC32(v);
		PUN_CHECK_SERIAL(crc == newCrc);
	}
}