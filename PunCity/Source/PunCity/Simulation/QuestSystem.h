// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameEventSystem.h"
#include "GameSimulationInfo.h"
#include "IGameSimulationCore.h"
#include "PunCity/PunUtils.h"
#include "UnlockSystem.h"
#include "PlayerOwnedManager.h"
#include "StatSystem.h"

#define LOCTEXT_NAMESPACE "QuestSystem"

//! Interface used for inter-system communication (UI/display will just use QuestSystem directly)
class IQuestSystem
{
public:
	virtual void RemoveQuest(QuestEnum questEnum) = 0;
};

struct Quest
{
	virtual ~Quest() {}

	// Name classEnum is needed for serialization
	virtual QuestEnum classEnum() = 0;
	
	int32 playerId = -1;
	int32 startTick = -1;
	bool done1SecUpdateStatus = false;
	
	IGameSimulationCore* simulation = nullptr;

	virtual FText questTitle() { return LOCTEXT("None", "None"); }
	virtual FText questDescription() { return FText(); }
	
	virtual FText numberDescription() {
		if (neededValue() != -1) {
			return FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(currentValue()), TEXT_NUM(neededValue()));
		}
		return FText();
	}
	virtual float fraction() {
		if (neededValue() != -1) {
			return static_cast<float>(currentValue()) / neededValue();
		}
		return 0.0f;
	}

	virtual bool NeedExclamation() { return false; }

	// These 3 must be used together
	virtual int32 currentValue() { return -1; }
	virtual int32 neededValue() { return -1; }

	virtual void OnStartQuest() {}
	virtual void OnFinishQuest() {}
	
	bool needSatisfied() {
		return neededValue() != -1 && currentValue() >= neededValue();
	}

	virtual void UpdateStatus(int32 value)
	{
		if (needSatisfied())
		{
			OnFinishQuest();
			EndQuest();
		}
	}

	virtual bool ShouldSkipToNextQuest() { return false; }

	// Reward card
	virtual bool CanGetRewardCard();
	virtual void GetRewardCard();
	virtual CardEnum rewardCardEnum() { return CardEnum::None; }
	
	virtual void Tick() {}

	void EndQuest() {
		PUN_CHECK(simulation);
		simulation->iquestSystem(playerId)->RemoveQuest(classEnum());
	}

	void AddStartPopup() {
		simulation->AddPopup(playerId, FText::Format(
			LOCTEXT("Quest_Start", "New Quest: {0}<line><space>{1}"),
			questTitle(),
			questDescription()
		));
	}

	void AddEndPopup(FText textIn) {
		simulation->AddPopup(playerId, 
			FText::Format(NSLOCTEXT("Quest", "QuestComplete_Pop", "Quest Completed: {0}<line><space>{1}"), questTitle(), textIn),
			"QuestComplete"
		);
	}

	virtual void Serialize(FArchive& Ar)
	{
		Ar << playerId;
		Ar << startTick;
		Ar << done1SecUpdateStatus;
	}

};

//!

//struct ChooseLocationQuest final : Quest
//{
//	QuestEnum classEnum() override { return QuestEnum::ChooseLocationQuest; }
//	
//	std::string questTitle() override { return "Choose Location"; }
//	std::string questDescription() override
//	{
//		std::stringstream ss;
//		ss << "Choose a location to start your town.";
//		return ss.str();
//	}
//	
//	void UpdateStatus(int32 value) override
//	{	
//		EndQuest();
//	}
//
//};

//!

struct BuildHousesQuest final : Quest
{
	QuestEnum classEnum() override { return QuestEnum::BuildHousesQuest; }

	FText questTitle() override { return LOCTEXT("BuildHouses_Title", "Build 5 houses"); }
	FText questDescription() override
	{
		return LOCTEXT("BuildHouses_Desc", "Citizens require cozy housing to survive.<space>To build houses:<bullet>Click the \"Build\" button on the bottom left menu</><bullet>Choose house</><bullet>Move your mouse to the desired location, then left-click to place the building</>");
	}

	bool ShouldSkipToNextQuest() override { return currentValue() >= neededValue(); }

	void OnStartQuest() override { AddStartPopup(); }

	bool NeedExclamation() override {
		return simulation->buildingCount(playerId, CardEnum::House) < 5;
	}

	void OnFinishQuest() override
	{
		PUN_CHECK(simulation);
		auto unlockSys = simulation->unlockSystem(playerId);
		unlockSys->townhallUpgradeUnlocked = true;

		AddEndPopup(
			LOCTEXT("BuildHouses_Finish", "Well done! Your people are happy that they finally have houses to live in.<space>Providing enough housing is important. Homeless people can migrate away, or die during winter. On the other hand, having extra houses can attract immigrants.")
		);
		
		if (simulation->townLvl(playerId) == 1) {
			simulation->parameters(playerId)->NeedTownhallUpgradeNoticed = true;
		}
	}

	int32 currentValue() override { return simulation->buildingFinishedCount(playerId, CardEnum::House); }
	int32 neededValue() override { return 5; }
};

struct TownhallUpgradeQuest final : Quest
{
	QuestEnum classEnum() override { return QuestEnum::TownhallUpgradeQuest; }

	FText questTitle() override { return LOCTEXT("UpgradeTownhall_Title", "Upgrade the Townhall"); }
	FText questDescription() override
	{
		return LOCTEXT("UpgradeTownhall_Desc", "Upgrading the Townhall can help you unlock more gameplay elements.<space>To upgrade:<bullet>Click the Townhall to bring up its focus UI</><bullet>Click the [Upgrade Townhall] button on the focus UI</>");
	}

	bool ShouldSkipToNextQuest() override { return currentValue() >= neededValue(); }

	void OnStartQuest() override { AddStartPopup(); }

	void OnFinishQuest() override
	{
		PUN_CHECK(simulation);
		auto unlockSys = simulation->unlockSystem(playerId);
		unlockSys->townhallUpgradeUnlocked = true;

		AddEndPopup(
			LOCTEXT("UpgradeTownhall_Finish", "Great! Your townhall is now level 2.<space>Upgrading the Townhall, Researching Technology, and Upgrading Houses are ways you progress through the game and unlock new gameplay elements."
		));

	}

	int32 currentValue() override { return (simulation->townLvl(playerId) >= 2); }
	int32 neededValue() override { return 1; }
};

struct PopulationQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::PopulationQuest; }
	
	int32 townSizeTier = -1;
	void Serialize(FArchive& Ar) override {
		Quest::Serialize(Ar);
		Ar << townSizeTier;
	}

	FText questTitle() override { return LOCTEXT("GrowPopulation_Title", "Grow Population"); }
	FText questDescription() override
	{
		return LOCTEXT("GrowPopulation_Desc", "To grow your population:<bullet>Add more houses. The more available housing spaces, the more children people will have.</><bullet>Keep your citizens happy</>");
	}
	FText numberDescription() override {
		return FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(simulation->population(playerId)), TEXT_NUM(GetTownSizeMinPopulation(townSizeTier)));
	}

	float fraction() override { return static_cast<float>(simulation->population(playerId)) / GetTownSizeMinPopulation(townSizeTier); }

	void UpdateStatus(int32 value) override
	{
		int32 population = simulation->population(playerId);
		
		if (population >= GetTownSizeMinPopulation(townSizeTier))
		{
			simulation->GenerateRareCardSelection(playerId, RareHandEnum::PopulationQuestCards, NSLOCTEXT("QuestSys", "PopulationQuest", "PopulationQuest"));

			EndQuest();
		}
	}

};


struct GatherMarkQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::GatherMarkQuest; }
	
	FText questTitle() override
	{
		return FText::Format(LOCTEXT("GatherMark_Title", "Gather mark {0} trees"), TEXT_NUM(neededValue()));
	}
	FText questDescription() override
	{
		return LOCTEXT("GatherMark_Desc", "Wood from trees can be used for construction or as fuel.<space>To mark trees for citizens to gather:<bullet>Click the \"Gather\" button on the bottom left menu</><bullet>Move your mouse to the desired location</><bullet>Left-click drag to mark the dragged area for citizens to gather</>");
	}


	int32 currentValue() override { return _gatherMarkedCount; }
	int32 neededValue() override { return 10; }


	void UpdateStatus(int32 value) override
	{
		_gatherMarkedCount += value;
		
		if (_gatherMarkedCount >= neededValue())
		{
			AddEndPopup(LOCTEXT("GatherMark_Finish", "Great job! Now your citizens will cut down those trees."));
			EndQuest();	
		}
	}

	// Skip if already gathered, or on desert
	bool ShouldSkipToNextQuest() override;

	void Serialize(FArchive& Ar) override {
		Quest::Serialize(Ar);
		Ar << _gatherMarkedCount;
	}

private:
	int32 _gatherMarkedCount = 0;
};

struct FoodBuildingQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::FoodBuildingQuest; }

	FText questTitle() override { return LOCTEXT("FoodBuilding_Title", "Build a food producer."); }
	FText questDescription() override
	{
		return LOCTEXT("FoodBuilding_Desc", "People require food to stay alive.<space>To build a food producer:<bullet>Click the card stack on the bottom right of the screen</><bullet>Choose a food producer building</><bullet>Move your mouse to the desired location, then left-click to place the building</>");
	}

	bool ShouldSkipToNextQuest() override { return currentValue() >= neededValue(); }

	void OnStartQuest() override { AddStartPopup(); }

	int32 currentValue() override { return foodBuildingCount(); }
	int32 neededValue() override { return 1; }
	void OnFinishQuest() override {
		AddEndPopup(LOCTEXT("FoodBuilding_Finish", "Superb! After the food producer is built, it will start producing food."));
	}

private:
	int32 foodBuildingCount()
	{
		int32 count = 0;
		for (CardEnum cardEnum : FoodBuildings) {
			if (cardEnum == CardEnum::Forester) {
				continue;
			}
			count += simulation->buildingCount(playerId, cardEnum);
		}
		return count;
	}
};

struct ClaimLandQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::ClaimLandQuest; }

	FText questTitle() override { return LOCTEXT("ClaimTerritory_Title", "Claim territory"); }
	FText questDescription() override
	{
		return LOCTEXT("ClaimTerritory_Desc", "Expand your territory by claiming regions.<space>To claim a region:<bullet>Click on any region/tile adjacent to your territory's border</><bullet>On the opened description UI, click the \"Claim Land\" button</>");
	}

	void OnStartQuest() override { AddStartPopup(); }

	bool ShouldSkipToNextQuest() override { return currentValue() >= neededValue(); }

	int32 currentValue() override { return simulation->playerOwned(playerId).provincesClaimed().size() - 1; }
	int32 neededValue() override { return 1; }
	void OnFinishQuest() override
	{
		AddEndPopup(LOCTEXT("ClaimTerritory_Finish", "Nicely done! Now you can build and gather resources on your new territory."));
	}
};

struct BuildStorageQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::BuildStorageQuest; }

	FText questTitle() override { return LOCTEXT("BuildStorage_Title", "Build a storage yard"); }
	FText questDescription() override
	{
		return LOCTEXT("BuildStorage_Desc", "Storage yards are required to store resources for later use.<space>To build a storage yard:<bullet>Click the \"Build\" button on the bottom left menu</><bullet>Choose storage yard</><bullet>Move your mouse to the desired location, then left-click to place the building</>");
	}

	int32 currentValue() override {
		return storageBuilt() - 2 + simulation->statSystem(playerId).GetStat(AccumulatedStatEnum::StoragesDestroyed);
	}
	int32 neededValue() override { return 1; }
	void OnFinishQuest() override {
		AddEndPopup(LOCTEXT("BuildStorage_Finish", "Great! With a storage built, your citizens will have enough space to store resources."));
	}

private:
	int32 storageBuilt() { return simulation->buildingFinishedCount(playerId, CardEnum::StorageYard); }
};

struct SurviveWinterQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::SurviveWinterQuest; }

	FText questTitle() override { return LOCTEXT("SurviveTheWinter_Title", "Survive the winter"); }
	FText questDescription() override
	{
		return LOCTEXT("SurviveTheWinter_Desc", "Winter is harsh, and requires your town to be well-prepared to face it.<space>To survive the winter, we must have:<bullet>houses for everyone</><bullet>enough stored food</><bullet>enough stored wood or coal for heating</><bullet>enough stored medicine, since disease frequency increases in winter</>");
	}

	bool ShouldSkipToNextQuest() override { return currentValue() >= neededValue(); }

	void OnStartQuest() override { AddStartPopup(); }

	float fraction() override
	{
		float ticksIntoWinter = Time::Ticks() - Time::TicksPerSeason * 3;
		return std::max(0.0f, ticksIntoWinter / Time::TicksPerSeason);
	}

	void Tick() override
	{
		if (Time::Years() > 0)
		{
			AddEndPopup(LOCTEXT("SurviveTheWinter_Finish", "Congratulation! You survived your first winter."));
			EndQuest();
		}
	}
};


struct ProductionQuest : Quest
{
	virtual ResourceEnum resourceEnum() { return ResourceEnum::None; }
	virtual int32 neededProductionCount() { return -1; }

	FText questTitle() override
	{
		return FText::Format(LOCTEXT("ProductionQuest_Title", "Produce {0} {1}"), TEXT_NUM(neededProductionCount()), ResourceNameT(resourceEnum()));
	}
	FText questDescription() override
	{
		return FText::Format(LOCTEXT("ProductionQuest_Desc", "{0} to unlock the reward.\n"), questTitle());
	}
	FText numberDescription() override {
		return FText::Format(INVTEXT("{0}/{1}"), TEXT_NUM(productionSoFar), TEXT_NUM(neededProductionCount()));
	}

	float fraction() override {
		return productionSoFar / static_cast<float>(neededProductionCount());
	}

	//void Tick() override;

	void UpdateStatus(int32 value) override
	{
		productionSoFar += value;
		
		if (productionSoFar >= neededProductionCount() &&
			CanGetRewardCard())
		{
			AddEndPopup(FText::Format(
				LOCTEXT("ProductionQuest_Desc", "Quest completed!\nProduced {0} {1}.\nReward Card: {2}"),
				TEXT_NUM(neededProductionCount()),
				ResourceNameT(resourceEnum()),
				GetBuildingInfo(rewardCardEnum()).name
			));

			GetRewardCard();
			EndQuest();
		}
	}

	void Serialize(FArchive& Ar) override {
		Quest::Serialize(Ar);
		Ar << productionSoFar;
	}

private:
	int32 productionSoFar = 0;
};

struct CooperativeFishingQuest : ProductionQuest
{
	QuestEnum classEnum() override { return QuestEnum::CooperativeFishingQuest; }

	ResourceEnum resourceEnum() override { return ResourceEnum::Fish; }
	int32 neededProductionCount() override { return 500; }
	CardEnum rewardCardEnum() override { return CardEnum::CooperativeFishing; }
};

struct BeerQuest : ProductionQuest
{
	QuestEnum classEnum() override { return QuestEnum::BeerQuest; }

	ResourceEnum resourceEnum() override { return ResourceEnum::Beer; }
	int32 neededProductionCount() override { return 1000; }
	CardEnum rewardCardEnum() override { return CardEnum::MasterBrewer; }
};

struct PotteryQuest : ProductionQuest
{
	QuestEnum classEnum() override { return QuestEnum::PotteryQuest; }

	ResourceEnum resourceEnum() override { return ResourceEnum::Pottery; }
	int32 neededProductionCount() override { return 500; }
	CardEnum rewardCardEnum() override { return CardEnum::MasterPotter; }
};


struct TradeQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::TradeQuest; }
	
	int32 currentValue() override { return valueSoFar; }
	int32 neededValue() override { return 3000; }
	CardEnum rewardCardEnum() override { return CardEnum::CompaniesAct; }
	
	FText questTitle() override
	{
		return FText::Format(LOCTEXT("TradeQuest_Title", "Trade {0} "), TEXT_NUM(neededValue()));
	}
	FText questDescription() override
	{
		return FText::Format(LOCTEXT("TradeQuest_Desc", "{0} to unlock the reward.\n"), questTitle());
	}

	void UpdateStatus(int32 value) override
	{
		valueSoFar += value;

		if (valueSoFar >= neededValue() && CanGetRewardCard())
		{
			AddEndPopup(FText::Format(
				LOCTEXT("TradeQuest_Finish", "Quest completed!<space>Traded {0}<img id=\"Coin\"/>.<space>Reward Card: {1}"),
				TEXT_NUM(neededValue()),
				GetBuildingInfo(rewardCardEnum()).name
			));

			GetRewardCard();
			EndQuest();
		}
	}


	void Serialize(FArchive& Ar) override {
		Quest::Serialize(Ar);
		Ar << valueSoFar;
	}

private:
	int32 valueSoFar = 0;
};


struct HouseUpgradeQuest : Quest
{
	QuestEnum classEnum() override { return QuestEnum::HouseUpgradeQuest; }

	int32 upgradeQuestLvl = 1;
	
	void Serialize(FArchive& Ar) override {
		Quest::Serialize(Ar);
		Ar << upgradeQuestLvl;
	}

	FText questTitle() override
	{
		return FText::Format(
			LOCTEXT("HouseUpgrade_Title", "Acquire {0} house lvl {1}"),
			TEXT_NUM(neededHouseCount()),
			TEXT_NUM(neededHouseLvl())
		);
	}
	FText questDescription() override
	{
		return FText::Format(
			LOCTEXT("HouseUpgrade_Desc", "Upgrade {0} houses to lvl {1}"),
			TEXT_NUM(neededHouseCount()),
			TEXT_NUM(neededHouseLvl())
		);
	}

	int32 currentValue() override { return houseCount(); }
	int32 neededValue() override { return neededHouseCount(); }
	void OnFinishQuest() override {
		// TODO: make this choose between 2 cards??
		
		simulation->GenerateRareCardSelection(playerId, RareHandEnum::RareCards, 
			FText::Format(LOCTEXT("NewWealthStatus_Cards", "New wealth status achieved!\n(House lvl {0})"), TEXT_NUM(neededHouseLvl()))
		);
	}

	static int32 maxQuestLvl() { return 3; }
	static std::pair<int32, int32> HouseCountAndLvl(int32 questLvl) {
		switch (questLvl)
		{
		case 1: return { 10, 2 };
		case 2: return { 15, 3 };
		case 3: return { 25, 4 };
		default:
			UE_DEBUG_BREAK();
			return { 0, 0 };
		}
	}
	
private:
	int32 houseCount() { return simulation->GetHouseLvlCount(playerId, neededHouseLvl(), true); }
	
	int32 neededHouseCount() {
		return HouseCountAndLvl(upgradeQuestLvl).first;
	}
	int32 neededHouseLvl() {
		return HouseCountAndLvl(upgradeQuestLvl).second;
	}
	
};

/**
 * 
 */
class QuestSystem final : public IQuestSystem
{
public:
	QuestSystem(int32 playerId, IGameSimulationCore* simulation) {
		_playerId = playerId;
		_simulation = simulation;
	}
	virtual ~QuestSystem() {}

	void Tick()
	{
		// TODO: why range based loop broken ??
		for (size_t i = _quests.size(); i-- > 0;) {
			auto& quest = _quests[i];
			PUN_CHECK(quest);
			quest->Tick();
		}

		// Must be done in two loops because Tick() or UpdateStatus() may modify the _quest length
		for (size_t i = _quests.size(); i-- > 0;) {
			auto& quest = _quests[i];
			PUN_CHECK(quest);
			
			// 1 sec delayed UpdateStatus after started, to make sure the quest isn't already done
			if (!quest->done1SecUpdateStatus && Time::Ticks() - quest->startTick > Time::TicksPerSecond) {
				quest->done1SecUpdateStatus = true;
				quest->UpdateStatus(0);
			}
		}
	}

	void AddQuest(std::shared_ptr<Quest> quest)
	{	
		quest->playerId = _playerId;
		quest->simulation = _simulation;
		quest->startTick = Time::Ticks();

		// In some cases, the quest's conditions was already satisfied, so we shouldn't start the quest.
		if (quest->ShouldSkipToNextQuest()) {
			OnQuestFinish(quest);
		}
		else {
			quest->OnStartQuest();
			_quests.push_back(quest);
		}

		_startedQuestEnums.push_back(quest->classEnum());
	}
	
	void RemoveQuest(QuestEnum questEnum) override {
		for (size_t i = _quests.size(); i-- > 0;) {
			if (questEnum == _quests[i]->classEnum()) {
				OnQuestFinish(_quests[i]);
				_quests.erase(_quests.begin() + i);
				break;
			}
		}
	}

	std::shared_ptr<Quest> GetQuest(QuestEnum questEnum)
	{
		for (auto quest : _quests) {
			if (quest->classEnum() == questEnum) {
				return quest;
			}
		}
		return nullptr;
	}

	const std::vector<std::shared_ptr<Quest>>& quests()
	{
		// TODO: Debug check
		for (int32 i = 0; i < _quests.size(); i++) {
			PUN_CHECK(_quests[i]);
			if (_quests[i]->classEnum() == QuestEnum::HouseUpgradeQuest) {
				int32 questLvl = std::static_pointer_cast<HouseUpgradeQuest>(_quests[i])->upgradeQuestLvl;
				PUN_CHECK(1 <= questLvl && questLvl <= 3);
			}
		}
		return _quests;
	}

	bool WasQuestStarted(QuestEnum questEnumIn)
	{
		for (QuestEnum questEnum : _startedQuestEnums) {
			if (questEnum == questEnumIn) {
				return true;
			}
		}
		return false;
	}

	std::shared_ptr<Quest> CreateQuest(QuestEnum classEnum)
	{
#define CASE(QuestName) case QuestEnum::QuestName: return std::make_shared<QuestName>();
		switch (classEnum) {
			CASE(GatherMarkQuest);
			CASE(FoodBuildingQuest);
			CASE(ClaimLandQuest);

			CASE(BuildHousesQuest);
			CASE(HouseUpgradeQuest);

			CASE(PopulationQuest);
			
			CASE(BuildStorageQuest);
			CASE(SurviveWinterQuest);
		
			CASE(CooperativeFishingQuest);
			CASE(BeerQuest);
			CASE(PotteryQuest);
			
			CASE(TradeQuest);

			CASE(TownhallUpgradeQuest);
			
		default:
			UE_DEBUG_BREAK();
			return nullptr;
		}
#undef CASE
	}

	void Serialize(FArchive& Ar)
	{
		SerializeVecLoop(Ar, _quests, [&](std::shared_ptr<Quest>& quest) {
			SerializePtr<std::shared_ptr<Quest>, QuestEnum>(Ar, quest, [&](QuestEnum classEnum) {
				quest = CreateQuest(classEnum);
				quest->simulation = _simulation;
			});
		});
		SerializeVecValue(Ar, _startedQuestEnums);

		// Check
		for (size_t i = 0; i < _quests.size(); i++) {
			PUN_CHECK(_quests[i]->playerId == _playerId); // This should be done in Quest.Serialize()
		}
	}

private:
	void OnQuestFinish(std::shared_ptr<Quest>& quest)
	{
		// Quest graph...
		switch(quest->classEnum())
		{
		//case QuestEnum::ChooseLocationQuest: {
		//	AddQuest(std::make_shared<FoodBuildingQuest>());
		//	return;
		//}

		case QuestEnum::FoodBuildingQuest: {
			AddQuest(std::make_shared<GatherMarkQuest>());
			return;
		}

		case QuestEnum::GatherMarkQuest: {
			AddQuest(std::make_shared<ClaimLandQuest>());
			return;
		}

		case QuestEnum::ClaimLandQuest: {
			AddQuest(std::make_shared<BuildHousesQuest>());
			return;
		}

		case QuestEnum::BuildHousesQuest: {
			AddQuest(std::make_shared<TownhallUpgradeQuest>());
			return;
		}

		case QuestEnum::TownhallUpgradeQuest:
		{
			auto popQuest = std::make_shared<PopulationQuest>();
			popQuest->townSizeTier = 1;
			AddQuest(popQuest);

			AddQuest(std::make_shared<SurviveWinterQuest>());
			return;
		}

		case QuestEnum::PopulationQuest: {
			int32 nextTier = std::static_pointer_cast<PopulationQuest>(quest)->townSizeTier + 1;
			if (nextTier <= 5) {
				auto popQuest = std::make_shared<PopulationQuest>();
				popQuest->townSizeTier = std::static_pointer_cast<PopulationQuest>(quest)->townSizeTier + 1;
				AddQuest(popQuest);
			}
			// House upgrade tree after first pop quest
			if (nextTier == 2) {
				//auto newQuest = std::make_shared<HouseUpgradeQuest>();
				//newQuest->upgradeQuestLvl = 1;
				//AddQuest(newQuest);
			}
			return;
		}
		//case QuestEnum::SurviveWinterQuest: {
		//	auto newQuest = std::make_shared<HouseUpgradeQuest>();
		//	newQuest->upgradeQuestLvl = 1;
		//	AddQuest(newQuest);
		//	return;
		//}

		case QuestEnum::HouseUpgradeQuest: {
			int32 nextLvl = std::static_pointer_cast<HouseUpgradeQuest>(quest)->upgradeQuestLvl + 1;
			if (nextLvl <= HouseUpgradeQuest::maxQuestLvl()) {
				auto newQuest = std::make_shared<HouseUpgradeQuest>();
				newQuest->upgradeQuestLvl = nextLvl;
				AddQuest(newQuest);
			}
			return;
		}

			UE_DEBUG_BREAK(); // If hit here, forgot to add return...
		default:
			return;
		}
	}

private:
	std::vector<std::shared_ptr<Quest>> _quests;
	std::vector<QuestEnum> _startedQuestEnums;
	
private:
	int32 _playerId = -1;
	IGameSimulationCore* _simulation = nullptr;
};


#undef LOCTEXT_NAMESPACE