// Fill out your copyright notice in the Description page of Project Settings.

#include "Perlin.h"
#include "PunCity/GameRand.h"
#include "CoreMinimal.h"


void Perlin::Init(uint32_t seed)
{
	_rand = GameRandObj(seed);
	
	for (int i = 0; i < 256; ++i) {
		p[i] = i;

		Gx[i] = (_rand.Rand() % FDOne) * 2 - FDOne;
		Gy[i] = (_rand.Rand() % FDOne) * 2 - FDOne;
	}

	int j = 0;
	int swp = 0;
	for (int i = 0; i < 256; i++) {
		j = _rand.Rand() & 255;

		swp = p[i];
		p[i] = p[j];
		p[j] = swp;
	}
}

FloatDet Perlin::noise(FloatDet sample_x, FloatDet sample_y)
{
	// Tile's 4 corners around point
	int x0 = FDToInt(sample_x);
	int x1 = x0 + 1;
	int y0 = FDToInt(sample_y);
	int y1 = y0 + 1;

	// Determine sample point position within unit tile
	FloatDet px0 = sample_x - IntToFD(x0);
	FloatDet px1 = px0 - FDOne;
	FloatDet py0 = sample_y - IntToFD(y0);
	FloatDet py1 = py0 - FDOne;

	// Compute dot product between gradient and sample position vector
	int gIndex = p[(x0 + p[y0 & 255]) & 255];
	FloatDet d00 = FDMul(Gx[gIndex], px0) + FDMul(Gy[gIndex], py0);
	gIndex = p[(x1 + p[y0 & 255]) & 255];
	FloatDet d10 = FDMul(Gx[gIndex], px1) + FDMul(Gy[gIndex], py0);

	gIndex = p[(x0 + p[y1 & 255]) & 255];
	FloatDet d01 = FDMul(Gx[gIndex], px0) + FDMul(Gy[gIndex], py1);
	gIndex = p[(x1 + p[y1 & 255]) & 255];
	FloatDet d11 = FDMul(Gx[gIndex], px1) + FDMul(Gy[gIndex], py1);

	// Interpolate dot product values at sample point using polynomial interpolation 6x^5 - 15x^4 + 10x^3
	FloatDet px0_2 = FDMul(px0, px0);
	FloatDet px0_3 = FDMul(px0_2, px0);
	FloatDet wx = 6 * FDMul(px0_3, px0_2) - 15 * FDMul(px0_2, px0_2) + 10 *px0_3;
	FloatDet py0_2 = FDMul(py0, py0);
	FloatDet py0_3 = FDMul(py0_2, py0);
	FloatDet wy = 6 * FDMul(py0_3, py0_2) - 15 * FDMul(py0_2, py0_2) + 10 * py0_3;

	FloatDet xx1 = d00 + FDMul(wx, (d10 - d00));
	FloatDet xx2 = d01 + FDMul(wx, (d11 - d01));
	FloatDet yy = xx1 + FDMul(wy, (xx2 - xx1));
	return yy;
}