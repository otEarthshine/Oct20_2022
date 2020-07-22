// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PunCity/PunSTLContainerOverride.h"
#include <iomanip>
#include <sstream>

/**
 * 
 */
class CppUtils
{
public:

	template<typename T>
	static bool TryAdd(std::vector<T>& v, const T& value) {
		if (std::find(v.begin(), v.end(), value) == v.end()) {
			v.push_back(value);
			return true;
		}
		return false;
	}

	template<typename T>
	static void Remove(std::vector<T>& v, T value) {
		for (int i = v.size(); i-- > 0;) {
			if (v[i] == value) {
				v.erase(v.begin() + i);
				return;
			}
		}
		UE_DEBUG_BREAK();
	}

	template<typename T, typename Func>
	static T RemoveOneIf(std::vector<T>& v, Func shouldRemove) {
		for (size_t i = v.size(); i-- > 0;) {
			if (shouldRemove(v[i])) {
				T value = v[i];
				v.erase(v.begin() + i);
				return value;
			}
		}
		UE_DEBUG_BREAK();
		return T();
	}

	template<typename T, typename Func>
	static void RemoveIf(std::vector<T>& v, Func shouldRemove) {
		v.erase(std::remove_if(v.begin(), v.end(), shouldRemove), v.end());
	}

	template<typename T>
	static bool TryRemove(std::vector<T>& v, T value) {
		for (int i = v.size(); i-- > 0;) {
			if (v[i] == value) {
				v.erase(v.begin() + i);
				return true;
			}
		}
		return false;
	}

	template<typename T>
	static void RemoveAll(std::vector<T>& v, T value) {
		for (int i = v.size(); i-- > 0;) {
			if (v[i] == value) {
				v.erase(v.begin() + i);
			}
		}
	}

	template<typename T>
	static bool Contains(std::vector<T>& v, const T& value) { // Note: const std::vector<T>& v causes error...
		return std::find(v.begin(), v.end(), value) != v.end();
	}

	template<typename T, typename Func>
	static bool Contains(std::vector<T>& v, Func func) {
		return std::find_if(v.begin(), v.end(), func) != v.end();
	}

	template<typename T>
	static int32 Sum(const std::vector<T>& v) {
		int32 result = 0;
		for (const T& vElement : v) {
			result += vElement;
		}
		return result;
	}

	template<typename T>
	static int32 Sum(const TArray<T>& arr) {
		int32 result = 0;
		for (const T& element : arr) {
			result += element;
		}
		return result;
	}
	

	template<typename T>
	static std::vector<T> ArrayToVec(const TArray<T>& tArray)
	{
		std::vector<T> vec;
		vec.reserve(tArray.Num());
		for (int32 i = 0; i < tArray.Num(); i++) {
			vec.push_back(tArray[i]);
		}
		return vec;
	}

	template<typename T>
	static TArray<T> VecToArray(const std::vector<T>& vec)
	{
		TArray<T> tArray;
		tArray.SetNum(vec.size());
		for (int32 i = 0; i < vec.size(); i++) {
			tArray[i] = vec[i];
		}
		return tArray;
	}
};

static std::string SpaceToUnderscore(std::string text) {
	for (std::string::iterator it = text.begin(); it != text.end(); ++it) {
		if (*it == ' ') *it = '_';
	}
	return text;
}


template<typename T>
static void VecReset(std::vector<T>& v) {
	std::fill(v.begin(), v.end(), 0);
}

template<typename T>
static void VecReset(std::vector<T>& v, T value) {
	std::fill(v.begin(), v.end(), value);
}