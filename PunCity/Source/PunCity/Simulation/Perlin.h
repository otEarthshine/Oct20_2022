// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSimulationConstants.h"
#include "PunCity/GameRand.h"

class Perlin {
public:
	Perlin() {
		Init(1);
	}
	Perlin(uint32_t seed) {
		Init(seed);
	}
	void Init(uint32_t seed);

	FloatDet noise(FloatDet x, FloatDet y);
	FloatDet noise01(FloatDet x, FloatDet y) { return (noise(x, y) + FDOne) / 2; }

	//! Serialize
	FArchive& operator>>(FArchive &Ar) {
		if (Ar.IsSaving()) {
			uint32 seed = _rand.seed();
			Ar << seed;
		}
		else {
			uint32 seed = 0;
			Ar << seed;
			Init(seed);
		}
		return Ar;
	}
	
private:
	GameRandObj _rand;
	
	int p[256];
	FloatDet Gx[256];
	FloatDet Gy[256];
};
