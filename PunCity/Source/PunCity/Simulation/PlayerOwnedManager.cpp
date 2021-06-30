// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerOwnedManager.h"
#include "UnitStateAI.h"
#include "Building.h"
#include "Buildings/House.h"
#include "Buildings/Temples.h"
#include "Buildings/TownHall.h"
#include "Resource/ResourceSystem.h"
#include "HumanStateAI.h"
#include "StatSystem.h"
#include "PunCity/CppUtils.h"
#include "UnlockSystem.h"
#include "Buildings/GathererHut.h"
#include "Buildings/TradeBuilding.h"
#include "WorldTradeSystem.h"

using namespace std;

#define LOCTEXT_NAMESPACE "PlayerOwnedManager"

void PlayerOwnedManager::Tick1Sec()
{
	if (!hasChosenLocation()) return;


	/*
	 * Dark age
	 */
	if (!_isInDarkAge && _simulation->populationTown(_playerId) <= 15)
	{
		_simulation->AddPopup(PopupInfo(_playerId,
			LOCTEXT("CrisisBegin_Pop",
				"Crisis begins.<line><space>"
				"You must survive this slump and pull your settlement back on track.<space>"
				"Crisis, such as this, tests your skill as a leader.<space>"
				"During crisis, Leader Skills are x2 more effective and SP recovers x2 faster."
			),
			{ LOCTEXT("We must survive!", "We must survive!") },
			PopupReceiverEnum::None, false, "PopupBad"
		));

		_isInDarkAge = true;
	}
	if (_isInDarkAge && _simulation->populationTown(_playerId) >= 20)
	{
		_simulation->AddPopup(PopupInfo(_playerId,
			LOCTEXT("CrisisEnd_Pop",
				"Congratulation, you have survived the crisis.<line>"
				"What doesn't kill you makes you stronger.\n"
				"Our people are now crisis hardened, ready to march forward through any future obstacles."
			)
		));

		// TODO: Earn card?? may be x2 leader skill?

		_isInDarkAge = false;
	}

	/*
	 * Buffs
	 */
	for (size_t i = 0; i < _buffTicksLeft.size(); i++)
	{
		if (_buffTicksLeft[i] > 0) {
			_buffTicksLeft[i] = max(0, _buffTicksLeft[i] - Time::TicksPerSecond);
			if (_buffTicksLeft[i] < Time::TicksPerSecond * 30)
			{
				int32 cardEnumInt = i + BuildingEnumCount;

				_simulation->AddPopupNonDuplicate(PopupInfo(_playerId,
					FText::Format(LOCTEXT("BuffRunningOut_Pop", "Your {0} Buff is running out.<space>Would you like to renew it with <img id=\"Coin\"/>xPopulation?"),
						GetBuildingInfoInt(cardEnumInt).name
					),
					{ LOCTEXT("Renew", "Renew"),
						LOCTEXT("Close", "Close") },
					PopupReceiverEnum::ResetBuff, false, "", cardEnumInt)
				);
			}
		}
	}

	/*
	 * ProvinceClaimProgress
	 */
	for (ProvinceClaimProgress& claimProgress : _defendingClaimProgress) {
		claimProgress.Tick1Sec(_simulation);
	}
}

void PlayerOwnedManager::TickRound()
{
	/*
	 * Mid autumn buyer arrival
	 */
	PUN_LOG("MidAutumn TickRound: %d, %d, %d, %d", Time::Ticks(), (Time::Ticks() % Time::TicksPerSeason), (Time::Ticks() % Time::TicksPerSeason != 0), Time::IsAutumn());
	// Show Caravan only if there is no Trading Post/Port
	if (_simulation->playerBuildingFinishedCount(_playerId, CardEnum::TradingPost) == 0 &&
		_simulation->playerBuildingFinishedCount(_playerId, CardEnum::TradingPort) == 0) 
	{
		if (Time::IsAutumn() &&
			Time::Ticks() % Time::TicksPerSeason != 0)
		{
			_simulation->AddPopup(PopupInfo(_playerId,
				LOCTEXT("CaravanArrive_Pop", "A caravan has arrived. They wish to buy any goods you might have."),
				{ LOCTEXT("Trade", "Trade"),
					LOCTEXT("Refuse", "Refuse") },
				PopupReceiverEnum::CaravanBuyer
			));
		}
	}

	if (_simulation->playerBuildingFinishedCount(_playerId, CardEnum::ImmigrationOffice) == 0)
	{
		if (Time::IsSpring() &&
			Time::Ticks() % Time::TicksPerSeason != 0 &&
			Time::Ticks() > Time::TicksPerYear)
		{
			_simulation->ImmigrationEvent(_playerId, 3,
				LOCTEXT("YearlyImmigrantAsk_Pop", "3 Immigrants wishes to join your City."),
				PopupReceiverEnum::TribalJoinEvent
			);
		}
	}
}


void PlayerOwnedManager::TryApplyBuff(CardEnum cardEnum)
{
	if (HasBuff(cardEnum) && GetBuffTicksLeft(cardEnum) > Time::TicksPerMinute * 5) {
		_simulation->AddPopupToFront(_playerId,
			FText::Format(LOCTEXT("BuffAlreadyApplied", "{0} has already been applied."), GetBuildingInfo(cardEnum).name)
		);
		return;
	}

	int32 cost = _simulation->populationTown(_playerId);
	if (_simulation->moneyCap32(_playerId) < cost) {
		_simulation->AddPopupToFront(_playerId, 
			LOCTEXT("BuffNotEnoughCoin", "Require <img id=\"Coin\"/>xPopulation to activate the protection.")
		);
	}
	else {
		_simulation->ChangeMoney(_playerId, -cost);
		AddBuff(cardEnum);

		if (cardEnum == CardEnum::KidnapGuard) {
			_simulation->AddPopupToFront(_playerId, 
				LOCTEXT("AppliedKidnapGuard", "Applied Kidnap Guard. Protect your town against Kidnap for 1 year.")
			);
		}
		else {
			_simulation->AddPopupToFront(_playerId, 
				LOCTEXT("AppliedTreasuryGuard", "Applied Treasury Guard. Protect your town against Snatch and Steal for 1 year.")
			);
		}
	}
}

void PlayerOwnedManager::AddTaxIncomeToString(TArray<FText>& args)
{
	ADDTEXT_(LOCTEXT("TaxIncome_TipTitle", "Income: {0}<img id=\"Coin\"/>\n"), TEXT_100(totalIncome100()));

	ADDTEXT_LOCTEXT("TaxIncomeTip_SectionHouse", " House:\n");

	for (int32 i = 0; i < IncomeEnumCount; i++)
	{
		// Start of new section
		if (i == HouseIncomeEnumCount) {
			ADDTEXT_LOCTEXT("TaxIncomeTip_SectionOthers", " Others:\n");
		}

		int32 income = 0;
		for (int32 townId : _townIds) {
			income += _simulation->townManager(townId).incomes100[i];
		}
		
		if (income != 0) {
			ADDTEXT_(INVTEXT("  {0} {1}\n"), TEXT_100SIGNED(income), IncomeEnumName[i]);
		}
	}

	//if (hasChosenLocation()) {
	//	ADDTEXT_(INVTEXT("{0} ({1})"), TaxOptions[taxLevel], TEXT_PERCENT(taxPercent()));
	//}
}

void PlayerOwnedManager::AddInfluenceIncomeToString(TArray<FText>& args)
{
	ADDTEXT_(LOCTEXT("InfluenceIncome_Tip1",
		"Influence Per Round: {0}<img id=\"Influence\"/>\n"), TEXT_100(totalInfluenceIncome100())
	);

	for (int32 i = 0; i < InfluenceIncomeEnumCount; i++)
	{
		int32 influenceIncome100 = 0;
		for (int32 townId : _townIds) {
			influenceIncome100 += _simulation->townManager(townId).influenceIncomes100[i];
		}
		
		if (influenceIncome100 != 0) {
			ADDTEXT_(INVTEXT(" {0} {1}\n"),
				TEXT_100SIGNED(influenceIncome100),
				InfluenceIncomeEnumName[i]
			);
		}
	}

	//auto addStoredInfluenceRow = [&](InfluenceIncomeEnum influenceEnum) {
	//	ADDTEXT_(INVTEXT(" {0} {1}\n"),
	//		TEXT_100(influenceIncomes100[static_cast<int>(influenceEnum)] * storedToInfluenceRevenue),
	//		InfluenceIncomeEnumName[static_cast<int>(influenceEnum)]
	//	);
	//};

	ADDTEXT_INV_("<space>");
	ADDTEXT_(LOCTEXT("MaxStoredInfluence_Tip",
		"Max Stored Influence: {0}<img id=\"Influence\"/>\n"),
		TEXT_100(maxStoredInfluence100())
	);

	//addStoredInfluenceRow(InfluenceIncomeEnum::Townhall);
	//addStoredInfluenceRow(InfluenceIncomeEnum::Population);
	//addStoredInfluenceRow(InfluenceIncomeEnum::Luxury);
}

#undef LOCTEXT_NAMESPACE