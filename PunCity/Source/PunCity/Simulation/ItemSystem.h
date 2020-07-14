// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/CppUtils.h"
#include "PunCity/PunUtils.h"
#include "IGameSimulationCore.h"

/**
 * 
 */
class ItemSystem
{
public:
	ItemSystem(int32_t playerId, IGameSimulationCore* simulation)
	{
		_simulation = simulation;
		_playerId = playerId;
	}
		
	bool HasItem(ItemEnum itemEnum) {
		return CppUtils::Contains(items, itemEnum);
	}

	bool needItemSelection() {
		return !waitingCommandArrival && itemSelections.size() > 0;
	}
	void GenerateItemSelection() {
		//PUN_CHECK(itemSelections.size() == 0);
		itemSelections.clear();
		
		for (int i = 0; i < 3; i++)
		{
			ItemEnum itemEnum = ItemEnum::None;
			// Loop until we find a random item that this player doesn't have
			for (int j = 0; j < ItemCount; j++) {
				itemEnum = static_cast<ItemEnum>(GameRand::Rand() % ItemCount);
				if (!CppUtils::Contains(items, itemEnum) &&
					!CppUtils::Contains(itemSelections, itemEnum)) {
					break;
				}
			}
			itemSelections.push_back(itemEnum);
		}
	}
	
public:
	// Current items
	std::vector<ItemEnum> items;
	
	// Item selections to be displayed
	std::vector<ItemEnum> itemSelections;

	bool waitingCommandArrival = false;

private:
	IGameSimulationCore* _simulation;
	int32_t _playerId;
	
};
