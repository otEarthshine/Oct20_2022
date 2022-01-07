// Pun Dumnernchanvanit's


#include "ChatUI.h"
#include "PunSpacerElement.h"
#include "PunButton.h"
#include "PunCity/Sound/FireForgetAudioComponent.h"

using namespace std;

void UChatUI::RefreshHiddenSettingsUI()
{
#if !AUDIO_ALL
	return;
#endif
	
	auto ambientSound = dataSource()->soundSystem();
	
	// Lazy init
	if (_sortedGroupName.size() == 0) {
		_sortedGroupName.clear();
		for (auto pair : ambientSound->groupToPropertyNameToValue) {
			_sortedGroupName.push_back(pair.first);
		}
		std::sort(_sortedGroupName.begin(), _sortedGroupName.end());

		_chosenGroupName = "WindSeasonal";
	}
	
	HiddenSettingsGroupChooser->ClearChildren();
	HiddenSettingsBox->ClearChildren();
	GroupDebugRows.Empty();
	SoundDebugRows.Empty();

	std::unordered_map<std::string, std::unordered_map<std::string, float>>& groupToPropertyNameToValue = ambientSound->groupToPropertyNameToValue;
	std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, float>>>& groupToSoundToSoundPropertyNameToValue = ambientSound->groupToSoundToSoundPropertyNameToValue;


	// Group Chooser
	for (size_t i = 0; i < _sortedGroupName.size(); i++)
	{
		auto button = AddWidget<UPunButton>(UIEnum::PunButton);
		button->Set(FText(), ToFText(_sortedGroupName[i]), nullptr, FText(), this, CallbackEnum::None, i);
		HiddenSettingsGroupChooser->AddChild(button);
	}
	

	// Show Chosen Group
	//  NOTE: 
	{
		std::string groupName = _chosenGroupName;
		
		// Section Name
		auto groupSectionNameRow = AddWidget<UPunRichText>(UIEnum::PunRichText);
		groupSectionNameRow->SetText("<Header>" + groupName + "</>");
		HiddenSettingsBox->AddChild(groupSectionNameRow);
		HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));

		// Soundfx debug information...
		auto debugRow = AddWidget<UPunRichText>(UIEnum::PunRichText);
		debugRow->SetText(ambientSound->GetdebugStr(groupName));
		HiddenSettingsBox->AddChild(debugRow);
		GroupDebugRows.Add(ToFString(groupName), debugRow);
		
		HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));

		/*
		 * Group properties
		 */
		std::unordered_map<std::string, float>& groupPropertyNameToValue = groupToPropertyNameToValue[groupName];

		// Sort GroupPropertyNames
		std::vector<string> groupPropertyNames;
		for (auto propertyPair : groupPropertyNameToValue) {
			groupPropertyNames.push_back(propertyPair.first);
		}
		std::sort(groupPropertyNames.begin(), groupPropertyNames.end());
		
		for (const std::string& groupPropertyName : groupPropertyNames) {
			auto hiddenSettingsRow = AddWidget<UHiddenSettingsRow>(UIEnum::HiddenSettingsRow);
			hiddenSettingsRow->InitRow({ groupName, groupPropertyName });
			HiddenSettingsBox->AddChild(hiddenSettingsRow);
			HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));
		}
		// Group Spacer
		UPunSpacerElement* groupSpacer = AddWidget<UPunSpacerElement>(UIEnum::PunSpacerWidget);
		groupSpacer->Spacer->SetSize(FVector2D(0, 30));
		HiddenSettingsBox->AddChild(groupSpacer);

		/*
		 * Individual sound properties
		 */
		std::unordered_map<std::string, std::unordered_map<std::string, float>>& soundToSoundPropertyNameToValue = groupToSoundToSoundPropertyNameToValue[groupName];

		// Sort GroupPropertyNames
		std::vector<string> soundNames;
		for (auto propertyPair : soundToSoundPropertyNameToValue) {
			soundNames.push_back(propertyPair.first);
		}
		std::sort(soundNames.begin(), soundNames.end());

		
		for (auto soundName : soundNames) {
			std::unordered_map<std::string, float>& soundPropertyNameToValue = soundToSoundPropertyNameToValue[soundName];

			// Sound Name
			auto soundSectionNameRow = AddWidget<UPunRichText>(UIEnum::PunRichText);
			soundSectionNameRow->SetText("  <Subheader>" + groupName + "_" + soundName + "</>");
			HiddenSettingsBox->AddChild(soundSectionNameRow);
			HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));

			// Sort SoundPropertyNames
			std::vector<string> soundPropertyNames;
			for (auto propertyPair : soundPropertyNameToValue) {
				soundPropertyNames.push_back(propertyPair.first);
			}
			std::sort(soundPropertyNames.begin(), soundPropertyNames.end());

			// Erase Special PlayCount and LastPlayed Property
			auto playCountIt = find(soundPropertyNames.begin(), soundPropertyNames.end(), "PlayCount");
			PUN_CHECK(playCountIt != soundPropertyNames.end());
			soundPropertyNames.erase(playCountIt);
			auto lastPlayedIt = find(soundPropertyNames.begin(), soundPropertyNames.end(), "LastPlayed");
			PUN_CHECK(lastPlayedIt != soundPropertyNames.end());
			soundPropertyNames.erase(lastPlayedIt);

			// Display PlayCount as text box
			auto playCountTextBox = AddWidget<UPunRichText>(UIEnum::PunRichText);
			HiddenSettingsBox->AddChild(playCountTextBox);
			SoundDebugRows.Add(ToFString(soundName), playCountTextBox);
			HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));
			
			for (const std::string& soundPropertyName : soundPropertyNames) {
				auto hiddenSettingsRow = AddWidget<UHiddenSettingsRow>(UIEnum::HiddenSettingsRow);
				hiddenSettingsRow->InitRow({ groupName, soundName, soundPropertyName });
				HiddenSettingsBox->AddChild(hiddenSettingsRow);
				HiddenSettingsBox->AddChild(AddWidget<UPunSpacerElement>(UIEnum::PunThinLineSpacerWidget));
			}

			// Space between each sound
			UPunSpacerElement* spacer = AddWidget<UPunSpacerElement>(UIEnum::PunSpacerWidget);
			spacer->Spacer->SetSize(FVector2D(0, 30));
			HiddenSettingsBox->AddChild(spacer);
		}

		// Space between each group
		UPunSpacerElement* spacer = AddWidget<UPunSpacerElement>(UIEnum::PunSpacerWidget);
		spacer->Spacer->SetSize(FVector2D(0, 50));
		HiddenSettingsBox->AddChild(spacer);
	}

	// Isolate sound
	HiddenSettingsIsolateSoundDropdown->ClearOptions();

	unordered_set<string> groupNames;
	for (const auto& it : groupToPropertyNameToValue) {
		groupNames.insert(it.first);
	}

	unordered_set<string> soundNames;
	for (const auto& groupIt : groupToSoundToSoundPropertyNameToValue) {
		for (const auto& soundIt : groupIt.second) {
			soundNames.insert(soundIt.first);
		}
	}

	// sort
	std::vector<string> groupNamesSort;
	groupNamesSort.insert(groupNamesSort.end(), groupNames.begin(), groupNames.end());
	sort(groupNamesSort.begin(), groupNamesSort.end());

	std::vector<string> soundNamesSort;
	soundNamesSort.insert(soundNamesSort.end(), soundNames.begin(), soundNames.end());
	sort(soundNamesSort.begin(), soundNamesSort.end());


	HiddenSettingsIsolateSoundDropdown->AddOption(FString("None"));
	for (const std::string& name : groupNamesSort) {
		HiddenSettingsIsolateSoundDropdown->AddOption(ToFString(name));
	}
	for (const std::string& name : soundNamesSort) {
		HiddenSettingsIsolateSoundDropdown->AddOption(ToFString(name));
	}
	HiddenSettingsIsolateSoundDropdown->SetSelectedOption(ToFString(dataSource()->soundSystem()->isolateSound));
	HiddenSettingsIsolateSoundDropdown->SetVisibility(ESlateVisibility::Visible);

}


void UChatUI::Tick()
{
	TickDebugUI();

	LEAN_PROFILING_UI(TickChatUI);
	
	// Don't display in single player unless toggled single player chat
	if (gameInstance()->isSinglePlayer && 
		!PunSettings::IsOn("SinglePlayerChat"))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	} 
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	
	//if (!simulation().HasTownhall(playerId())) {
	//	SetVisibility(ESlateVisibility::Collapsed);
	//	return;
	//}

	if (InterfacesInvalid()) {
		return;
	}
	
	
	if (!PunSettings::IsOn("UIChat")) {
		return;
	}
	
#if UI_CHAT
	
	// Chat
	auto& chatSystem = simulation().chatSystem();
	if (chatSystem.needRefreshChatUI) {
		RefreshChatUI();

		// ChatBoxWhenInactive Collapsed means it is game focus
		// If it is in game focus state, display the chat again briefly
		//PlayAnimation(Animations["ChatBoxInactiveFade"]);
	}

	RefreshChatBoxState();

	// Hidden Setings
	UpdateHiddenSettingsDebugInfo();


	// Desync Warning
	if (simulation().isDesynced != DesyncEnum::NotDesynced)
	{
		DesyncText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		DesyncText->SetText(FText::Format(
			INVTEXT("Desync: {0}"),
			FText::FromString(DesyncEnumName[static_cast<int>(simulation().isDesynced)])
		));
	}
	else
	{
		DesyncText->SetVisibility(ESlateVisibility::Collapsed);
	}

#endif

}


void UChatUI::TickDebugUI()
{
	LEAN_PROFILING_UI(TickDebugUI);
	
	/*
	 * Debug
	 *  !!! Must open SinglePlayerChat !!!
	 */

#if USE_LEAN_PROFILING
	if (PunSettings::IsOn("LeanProfilingUI"))
	{
		DebugOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		check(static_cast<int32>(LeanProfilerEnum::Count) == LeanScopeTimerChar.size());

		std::vector<LeanProfilerElement> allProfilerElements = LeanProfiler::LastEnumToElements;

		const int32 groupCount = 8;
		std::vector<std::vector<LeanProfilerElement>> groupToProfilingElements(groupCount);
		
		auto addToProfilingGroup = [&](int32 i)
		{
			int32 profilerEnumInt = static_cast<int32>(allProfilerElements[i].profilerEnum);

			if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::AttackOutgoing)) {
				groupToProfilingElements[7].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::TryCheckBadTile_Human)) {
				groupToProfilingElements[6].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::U_Update_FoodAge)) {
				groupToProfilingElements[5].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::P_FindPath)) {
				groupToProfilingElements[4].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::R_resourceCount)) {
				groupToProfilingElements[3].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::TickMainGameUI)) {
				groupToProfilingElements[2].push_back(allProfilerElements[i]);
			}
			else if (profilerEnumInt >= static_cast<int32>(LeanProfilerEnum::TickUnitDisplay)) {
				groupToProfilingElements[1].push_back(allProfilerElements[i]);
			}
			else {
				groupToProfilingElements[0].push_back(allProfilerElements[i]);
			}
		};

		
		int32 lineCount = 2;
		bool filledFirstColumn = false;
		auto incrementLine = [&](UTextBlock* textBlock, std::stringstream& ss) {
			lineCount++;
			if (!filledFirstColumn && lineCount > 60) {
				textBlock->SetText(FText::FromString(FString(ss.str().c_str())));
				ss.str("");
				filledFirstColumn = true;
			}
		};

		// Simulation Normal Profiling
		{
			std::stringstream ss;

			int32 profilingTicksInterval = PunSettings::Get("LeanProfilingTicksInterval");

			// sort into group
			for (int32 i = 0; i < allProfilerElements.size(); i++)
			{
				if (allProfilerElements[i].nanosecondsSum / 1000 / profilingTicksInterval <= PunSettings::Get("LeanProfilingMin")) {
					continue;
				}

				addToProfilingGroup(i);
			}

			ss << "FPS: " << networkInterface()->GetFPS() << "\n";
			ss << "us Per Frame\n";

			for (int32 j = 0; j < groupCount; j++)
			{
				std::vector<LeanProfilerElement>& profilerElements = groupToProfilingElements[j];

				std::sort(profilerElements.begin(), profilerElements.end(),
					[&](const LeanProfilerElement& a, const LeanProfilerElement& b) -> bool
				{
					return a.nanosecondsSum > b.nanosecondsSum;
				});


				for (int32 i = 0; i < profilerElements.size(); i++)
				{
					if (profilerElements[i].count > 0) {
						ss << LeanScopeTimerChar[static_cast<int32>(profilerElements[i].profilerEnum)] << ":\t"
							<< (profilerElements[i].nanosecondsSum / 1000 / profilingTicksInterval)
							<< "us \t(" << profilerElements[i].count << ")\n";
						
						incrementLine(TopLeftTextDebug, ss);
					}
				}

				ss << "\n";
			}

			TopLeftTextDebug2->SetText(FText::FromString(FString(ss.str().c_str())));
		}

		
		lineCount = 2;
		filledFirstColumn = false;

		// Simulation Max Profiling
		{
			groupToProfilingElements.clear();
			groupToProfilingElements.resize(groupCount);
			
			std::stringstream ss;

			// sort into group
			for (int32 i = 0; i < allProfilerElements.size(); i++)
			{
				if (allProfilerElements[i].maxTickNanosecondsSum / 1000 <= PunSettings::Get("LeanProfilingMaxMin")) {
					continue;
				}

				addToProfilingGroup(i);
			}

			ss << "Max per frame (us)\n";

			for (int32 j = 0; j < groupCount; j++)
			{
				std::vector<LeanProfilerElement>& profilerElements = groupToProfilingElements[j];

				std::sort(profilerElements.begin(), profilerElements.end(),
					[&](const LeanProfilerElement& a, const LeanProfilerElement& b) -> bool
				{
					return a.maxTickNanosecondsSum > b.maxTickNanosecondsSum;
				});


				for (int32 i = 0; i < profilerElements.size(); i++)
				{
					if (profilerElements[i].count > 0) {
						ss << LeanScopeTimerChar[static_cast<int32>(profilerElements[i].profilerEnum)] << ":\t"
							<< (profilerElements[i].maxTickNanosecondsSum / 1000)
							<< "us\n";
						
						incrementLine(TopLeftTextDebug3, ss);
					}
				}

				ss << "\n";
			}

			TopLeftTextDebug4->SetText(FText::FromString(FString(ss.str().c_str())));
		}

		
		return;
	}
#endif

#if TICK_DEBUG_UI
	
	if (PunSettings::IsOn("SoundDebugUI"))
	{
		std::stringstream ss;
		
		SetVisibility(ESlateVisibility::Collapsed); // Why this?
		DebugOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		TArray<UFireForgetAudioComponent*> punAudios = networkInterface()->GetPunAudios();
		for (int32 i = 0; i < punAudios.Num(); i++)
		{
			ss << punAudios[i]->soundName;
			if (punAudios[i]->audio) {
				ss << fixed << setprecision(2);
				ss << "(" << punAudios[i]->audio->VolumeMultiplier << ")";
			}
			ss << "\n";
		}
		
		TopLeftTextDebug->SetText(FText::FromString(FString(ss.str().c_str())));
		return;
	}

	if (PunSettings::IsOn("DebugUI"))
	{
		SetVisibility(ESlateVisibility::Collapsed); // Why this?
		DebugOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		//ss << "(" << to_string(Time::Minutes()) << "m " << to_string(Time::Seconds() - Time::Minutes() * Time::SecondsPerMinute) << "s) \n";
		//ss << "(" << to_string(Time::Ticks()) << " ticks) \n";

		auto& sim = simulation();
		auto& unitSystem = sim.unitSystem();
		auto& statSystem = sim.statSystem();

		std::stringstream ss;
		ss << "World units: " << unitSystem.unitCount() << "\n";
		ss << "deer units: " << unitSystem.unitCount(UnitEnum::DarkDeer) + unitSystem.unitCount(UnitEnum::YellowDeer) + unitSystem.unitCount(UnitEnum::RedDeer) << "\n";
		ss << "boar units: " << unitSystem.unitCount(UnitEnum::Boar) << "\n";
		ss << "human units: " << unitSystem.unitCount(UnitEnum::Human) << "\n";
		ss << "Max animal in province: " << sim.provinceInfoSystem().debugMaxAnimalCount << "\n";
		ss << "Total province animals: " << sim.provinceInfoSystem().debugTotalProvinceAnimalCount << "\n";

		int32 findFullBushTotal = UnitStateAI::debugFindFullBushSuccessCount + UnitStateAI::debugFindFullBushFailCount;
		ss << "FindFullBush Success: " << (static_cast<float>(UnitStateAI::debugFindFullBushSuccessCount) / findFullBushTotal) << "\n";
		ss << "FindFullBush Count: " << findFullBushTotal << "\n";

		//ss << "Trees: " << simulation.treeSystem().treeCount() << "\n";
		ss << "Bushes: " << sim.treeSystem().bushCount() << "\n";
		//ss << "Stones: " << simulation.treeSystem().stoneCount() << "\n";

		//ss << "Births: " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::Birth));
		//ss << "Deaths(Age): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::DeathAge));
		//ss << "Deaths(Starve): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::DeathStarve));
		//ss << "Deaths(Cold): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::DeathCold));

		//ss << "\nBushBirth: " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushBirth));
		ss << "BushDeath(Age): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushDeathAge));
		ss << "BushDeath(Winter): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushDeathWinter));

		ss << "BushUnitTrimmed: " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushUnitTrimmed));
		ss << "BushEmitSeed: " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushEmitSeed));
		ss << "BushEmitSeed(GoodSpot): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushEmitSeedGoodSpot));
		ss << "BushEmitSeed(Success): " << StatSystem::StatsToString(statSystem.GetStatAll(SeasonStatEnum::BushEmitSeedSuccess));

#if DEBUG_BUILD
		ss << "Moving units: " << unitSystem.stateCount("Moving") << "\n";
		ss << "NeedActionUpdate units: " << unitSystem.stateCount("NeedActionUpdate") << "\n";
		ss << "NeedTargetTile units: " << unitSystem.stateCount("NeedTargetTile") << "\n";
		ss << "Wait units: " << unitSystem.stateWaitCount() << "\n";

#define DEBUG_AI(VarName) ss << (TO_STR(VarName)) << " " << (Debug_##VarName) << "\n";

		DEBUG_AI(ResetCountPerSec);
		DEBUG_AI(FailedToFindPath);

		//DEBUG_AI(MoveRandomly);

		//DEBUG_AI(TryGatherTreeOnly_Succeed);
		//DEBUG_AI(TryGatherNotTreeOnly_Succeed);

		//DEBUG_AI(CalculateActions);
		//DEBUG_AI(TryCheckBadTile);
		//DEBUG_AI(TryStoreInventory);
		//DEBUG_AI(TryFindFood);
		//DEBUG_AI(TryHeatup);
		//DEBUG_AI(TryToolup);
		//DEBUG_AI(TryHealup);

		//DEBUG_AI(TryFillLuxuries);
		//DEBUG_AI(TryFun);
		//DEBUG_AI(TryConstructRoad_RoadMaker);
		//DEBUG_AI(TryConstruct_Constructor);
		//DEBUG_AI(TryGatherFruit);
		//DEBUG_AI(TryHunt);
		//DEBUG_AI(TryRanch);
		//DEBUG_AI(TryFarm);
		//DEBUG_AI(TryForesting);

		//DEBUG_AI(TryProduce_Special);
		//DEBUG_AI(TryProduce_Mine);
		//DEBUG_AI(TryProduce_Others);

		//DEBUG_AI(EmergencyTree10_TryMoveResourceAny);
		//DEBUG_AI(EmergencyTree0_TryMoveResourceAny);
		//DEBUG_AI(TryMoveResourcesAny_Drop);
		//DEBUG_AI(TryMoveResourcesAny_All10);
		//DEBUG_AI(TryMoveResourcesAny_All0);

		//DEBUG_AI(TryConstructRoad);
		//DEBUG_AI(TryConstructRoad_NotWorkConstructState);
		//DEBUG_AI(TryConstructRoad_Queued);
		//DEBUG_AI(TryConstructRoad_MaxQueued);

		//DEBUG_AI(TryGoNearbyHome);

		//DEBUG_AI(WorldSpaceUICreate);
		//DEBUG_AI(AddWidget);
		//DEBUG_AI(AddToolTip);

		// IsConnected
#define DEBUG_ISCONNECTED(VarName) DEBUG_AI(IsConnected_##VarName)
		DEBUG_ISCONNECTED(ResourceMoveValid);
		DEBUG_ISCONNECTED(IsMoveValid);
		DEBUG_ISCONNECTED(MoveRandomlyPerlin);
		DEBUG_ISCONNECTED(JustBruteIt);
		DEBUG_ISCONNECTED(MoveRandomly);
		DEBUG_ISCONNECTED(TryStockBurrowFood);
		DEBUG_ISCONNECTED(TryGoNearbyHome);
		DEBUG_ISCONNECTED(FindNearestUnreservedFullBush);
		DEBUG_ISCONNECTED(GetProvinceRandomTile);
		//DEBUG_ISCONNECTED(FindMarketResourceHolderInfo);
		DEBUG_ISCONNECTED(RefreshIsBuildingConnected);
		DEBUG_ISCONNECTED(ClaimProvince);
		DEBUG_ISCONNECTED(RefreshHoverWarning);
		DEBUG_ISCONNECTED(adjacentTileNearestTo);
		DEBUG_ISCONNECTED(DropResourceSystem);

		DEBUG_ISCONNECTED(IsConnectedBuildingResource);
		
#undef DEBUG_ISCONNECTED
		

#undef DEBUG_AI
#endif

		TopLeftTextDebug->SetText(FText::FromString(FString(ss.str().c_str())));
	}
	else {
		DebugOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

#endif
}