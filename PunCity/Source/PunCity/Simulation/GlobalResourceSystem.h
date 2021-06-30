// Pun Dumnernchanvanit's

#pragma once

#include "IGameSimulationCore.h"
#include "PunCity/CppUtils.h"

/**
 * 
 */
class GlobalResourceSystem
{
public:


	//! Money
	int64 money() const { return _money100 / 100; }
	int64 money100() const { return _money100; }

	int32 moneyCap32() {
		if (money() <= static_cast<int64>(MAX_int32 / 100)) {
			return static_cast<int32>(money());
		}
		return MAX_int32 / 100;
	}

	void SetMoney(int64 amount) { _money100 = amount * 100; }
	void ChangeMoney(int64 amount) {
		_money100 += amount * 100;
	}
	void ChangeMoney100(int64 amount100) {
		_money100 += amount100;
	}

	//! Influence
	int32 influence100() { return _influence100; }
	int32 influence() { return _influence100 / 100; }
	void SetInfluence(int32 amount) { _influence100 = amount * 100; }
	void ChangeInfluence(int32 amount) {
		_influence100 += amount * 100;
	}
	void ChangeInfluence100(int32 amount100) {
		_influence100 += amount100;
	}

	//! Seed
	std::vector<SeedInfo> seedsPlantOwned() const {
		return _unlockedSeeds;
	};

	bool HasSeed(CardEnum seedCardEnum) const {
		for (SeedInfo seedInfo : _unlockedSeeds) {
			if (seedInfo.cardEnum == seedCardEnum) return true;
		}
		return false;
	}
	bool HasSeed(TileObjEnum tileObjEnum) const {
		for (SeedInfo seedInfo : _unlockedSeeds) {
			if (seedInfo.tileObjEnum == tileObjEnum) return true;
		}
		return false;
	}
	
	void AddSeed(SeedInfo seedInfo) {
		_unlockedSeeds.push_back(seedInfo);

		// Sort so seed dropdown has
		std::vector<SeedInfo> newUnlockedSeeds;
		for (SeedInfo info : CommonSeedCards) {
			if (CppUtils::Contains(_unlockedSeeds, info)) {
				newUnlockedSeeds.push_back(info);
			}
		}
		for (SeedInfo info : SpecialSeedCards) {
			if (CppUtils::Contains(_unlockedSeeds, info)) {
				newUnlockedSeeds.push_back(info);
			}
		}

		_unlockedSeeds = newUnlockedSeeds;
	}

	/*
	 * Serialize
	 */
	void Serialize(FArchive& Ar)
	{
		Ar << _money100;
		Ar << _influence100;

		SerializeVecValue(Ar, _unlockedSeeds);
	}

private:
	int64 _money100 = 0;
	int32 _influence100 = 0;

	std::vector<SeedInfo> _unlockedSeeds;
};
