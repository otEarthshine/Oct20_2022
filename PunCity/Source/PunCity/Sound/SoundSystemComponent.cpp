// Pun Dumnernchanvanit's


#include "SoundSystemComponent.h"


DEFINE_LOG_CATEGORY(LogPunSound);

void USoundSystemComponent::LoadRawSoundFolders()
{
	LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
	
	if (!_isPackagingData && gameInstance) {
		gameInstance->SoundAssetsList.Empty();
	}
	
	/*
	 * Music
	 */

	LoadRawSoundFolder("Music_PositiveNonWinter", "Music/PositiveNonWinter");
	LoadRawSoundFolder("Music_PositiveSpring", "Music/PositiveSpring");
	//LoadRawSoundFolder("Music_PositiveSummer", "Music/PositiveSummer");
	LoadRawSoundFolder("Music_PositiveWinter", "Music/PositiveWinter");

	LoadRawSoundFolder("Music_NegativeNonWinter", "Music/NegativeNonWinter");
	LoadRawSoundFolder("Music_NegativeWinter", "Music/NegativeWinter");
	
	/*
	 * Ambient
	 */

	//! Wind
	for (const BiomeInfo& info : BiomeInfos)
	{
		string biomeName = info.GetNameWithoutSpace();

		LoadRawSoundFolder("WindSpring_" + biomeName, "Ambient/Wind/WindSpring_" + biomeName);
		LoadRawSoundFolder("WindSummer_" + biomeName, "Ambient/Wind/WindSummer_" + biomeName);
		LoadRawSoundFolder("WindAutumn_" + biomeName, "Ambient/Wind/WindAutumn_" + biomeName);
		LoadRawSoundFolder("WindWinter_" + biomeName, "Ambient/Wind/WindWinter_" + biomeName);
	}
	
	//LoadRawSoundFolder("WindSpring", "Ambient/Wind/WindSpring");
	//LoadRawSoundFolder("WindSummer", "Ambient/Wind/WindSummer");
	//LoadRawSoundFolder("WindAutumn", "Ambient/Wind/WindAutumn");
	//LoadRawSoundFolder("WindWinter", "Ambient/Wind/WindWinter");
	

	//! Other Ambience
	LoadRawSoundFolder("Ocean", "Ambient/Ocean");
	LoadRawSoundFolder("WindAltitude", "Ambient/WindAltitude");
	LoadRawSoundFolder("Rain", "Ambient/Rain");

	
	//! Bird Tweet
	for (std::string biomeExtension : TreeBiomeExtensions)
	{
		LoadRawSoundFolder("BirdTweetSummer" + biomeExtension, "Birds/TweetSummer" + biomeExtension);
		LoadRawSoundFolder("BirdTweetWinter" + biomeExtension, "Birds/TweetWinter" + biomeExtension);
	}

	for (std::string biomeExtension : NonTreeBiomeExtensions)
	{
		LoadRawSoundFolder("NonTreeBirdSound" + biomeExtension, "Birds/NonTreeBirdSound" + biomeExtension);
	}

	//! Crowd
	LoadRawSoundFolder("CrowdLargeFar", "Crowd/Large/CrowdLargeFar");
	LoadRawSoundFolder("CrowdLargeNear", "Crowd/Large/CrowdLargeNear");
	LoadRawSoundFolder("CrowdMediumFar", "Crowd/Medium/CrowdMediumFar");
	LoadRawSoundFolder("CrowdMediumNear", "Crowd/Medium/CrowdMediumNear");
	LoadRawSoundFolder("CrowdSmallFar", "Crowd/Small/CrowdSmallFar");
	LoadRawSoundFolder("CrowdSmallNear", "Crowd/Small/CrowdSmallNear");

	/*
	 * CitizenActions
	 */
	LoadRawSoundFolder("TreeChopping", "CitizenActions/TreeChopping");
	LoadRawSoundFolder("TreeFalling", "CitizenActions/TreeFalling");

	//! Crop
	LoadRawSoundFolder("CropPlanting", "CitizenActions/CropPlanting");
	LoadRawSoundFolder("CropHarvesting", "CitizenActions/CropHarvesting");

	//! StonePicking
	LoadRawSoundFolder("StonePicking", "CitizenActions/StonePicking");

	//! Construction
	LoadRawSoundFolder("RoadConstruction", "CitizenActions/ConstructionRoad");
	LoadRawSoundFolder("WoodConstruction", "CitizenActions/ConstructionWood");
	LoadRawSoundFolder("ConstructionComplete", "CitizenActions/ConstructionComplete");
	LoadRawSoundFolder("ConstructionCompleteRoad", "CitizenActions/ConstructionCompleteRoad");

	//! Bows
	LoadRawSoundFolder("BowImpactDirt", "CitizenActions/BowImpactDirt");
	LoadRawSoundFolder("BowImpactFlesh", "CitizenActions/BowImpactFlesh");
	LoadRawSoundFolder("BowShoot", "CitizenActions/BowShoot");

	//! Buildings
	LoadRawSoundFolder("BuildingBrewery", "Building/Brewery");
	LoadRawSoundFolder("BuildingCharcoalMaker", "Building/CharcoalMaker");
	LoadRawSoundFolder("BuildingCoalMine", "Building/CoalMine");
	
	LoadRawSoundFolder("BuildingFurnitureMaker", "Building/FurnitureMaker");
	LoadRawSoundFolder("BuildingFurnitureMaker_OneShot", "Building/FurnitureMaker_OneShot");
	LoadRawSoundFolder("BuildingSmelter", "Building/Smelter");
	LoadRawSoundFolder("BuildingSmelter_OneShot", "Building/Smelter_OneShot");
	LoadRawSoundFolder("BuildingQuarry", "Building/Quarry");
	LoadRawSoundFolder("BuildingQuarry_OneShot", "Building/Quarry_OneShot");
	
	//

	/*
	 * Dropoff Pickup
	 */

	 //! Dropoff
	LoadRawSoundFolder("Dropoff_Coal", "Dropoff/Coal");
	LoadRawSoundFolder("Dropoff_Cloth", "Dropoff/Cloth");
	LoadRawSoundFolder("Dropoff_Wood", "Dropoff/Wood");
	LoadRawSoundFolder("Dropoff_Stone", "Dropoff/Stone");
	LoadRawSoundFolder("Dropoff_Leather", "Dropoff/Leather");

	// Crops, Meat, Metal would get sound from these
	LoadRawSoundFolder("Dropoff_Crops", "Dropoff/Crops");
	LoadRawSoundFolder("Dropoff_Meat", "Dropoff/Meat");
	LoadRawSoundFolder("Dropoff_Metal", "Dropoff/Metal");
	LoadRawSoundFolder("Dropoff_Ore", "Dropoff/Ore");

	//! Pickup
	LoadRawSoundFolder("Pickup", "Pickup");

	/*
	 * Animals
	 */
	LoadRawSoundFolder("Pig", "Animals/Pig/Neutral");
	LoadRawSoundFolder("PigAngry", "Animals/Pig/Angry");

	LoadRawSoundFolder("Deer", "Animals/Deer/Neutral");
	LoadRawSoundFolder("DeerAngry", "Animals/Deer/Angry");

	LoadRawSoundFolder("Panda", "Animals/Panda/Neutral");
	LoadRawSoundFolder("PandaAngry", "Animals/Panda/Angry");

	/*
	 * UI
	 */
	LoadRawSoundFoldersUIOnly();
}

void USoundSystemComponent::LoadRawSoundFoldersUIOnly()
{
	/*
	 * UI
	 */
	LoadRawSoundFolder("UIWindowOpen", "/UI/UIWindowOpen");
	LoadRawSoundFolder("UIWindowClose", "/UI/UIWindowClose");

	LoadRawSoundFolder("DropdownChange", "/UI/DropdownChange");

	LoadRawSoundFolder("ResearchInitiated", "/UI/ResearchInitiated");
	LoadRawSoundFolder("ResearchComplete", "/UI/ResearchComplete");
	LoadRawSoundFolder("ResearchCompleteNewEra", "/UI/ResearchCompleteNewEra");

	LoadRawSoundFolder("Combo", "/UI/Combo");
	LoadRawSoundFolder("ChooseRareCard", "/UI/ChooseRareCard");
	
	LoadRawSoundFolder("UpgradeHouse", "/UI/UpgradeHouse");
	LoadRawSoundFolder("UpgradeBuilding", "/UI/UpgradeBuilding");
	LoadRawSoundFolder("UpgradeTownhall", "/UI/UpgradeTownhall");

	LoadRawSoundFolder("PopupNeutral", "/UI/PopupNeutral");
	LoadRawSoundFolder("PopupBad", "/UI/PopupBad");
	LoadRawSoundFolder("PopupCannot", "/UI/PopupCannot");

	//LoadRawSoundFolder("QuestNew", "/UI/QuestNew");
	LoadRawSoundFolder("QuestComplete", "/UI/QuestComplete");

	LoadRawSoundFolder("RoundCountdown5", "/UI/RoundCountdown/RoundCountdown5");
	LoadRawSoundFolder("RoundCountdown4", "/UI/RoundCountdown/RoundCountdown4");
	LoadRawSoundFolder("RoundCountdown3", "/UI/RoundCountdown/RoundCountdown3");
	LoadRawSoundFolder("RoundCountdown2", "/UI/RoundCountdown/RoundCountdown2");
	LoadRawSoundFolder("RoundCountdown1", "/UI/RoundCountdown/RoundCountdown1");
	LoadRawSoundFolder("RoundCountdown0", "/UI/RoundCountdown/RoundCountdown0");

	LoadRawSoundFolder("PlacementDrag", "/UI/PlacementDrag");
	LoadRawSoundFolder("PlaceBuilding", "/UI/PlaceBuilding");
	LoadRawSoundFolder("CancelPlacement", "/UI/PlacementCancel");

	LoadRawSoundFolder("CardDeal", "/UI/CardDeal");
	LoadRawSoundFolder("CardHover", "/UI/CardHover");
	LoadRawSoundFolder("CardClick", "/UI/CardClick");

	LoadRawSoundFolder("ButtonHover", "/UI/ButtonHover");
	LoadRawSoundFolder("ButtonClick", "/UI/ButtonClick");
	LoadRawSoundFolder("ButtonClick2", "/UI/ButtonClick2");
	LoadRawSoundFolder("ButtonClickInvalid", "/UI/ButtonClickInvalid");

	LoadRawSoundFolder("Chat", "/UI/Chat");

	LoadRawSoundFolder("UIIncrementalChange", "/UI/UIIncrementalChange");
	LoadRawSoundFolder("UIIncrementalError", "/UI/UIIncrementalError");

	LoadRawSoundFolder("TradeAction", "/UI/TradeAction");
	LoadRawSoundFolder("ClaimLand", "/UI/ClaimLand");

	LoadRawSoundFolder("FoodLowBell", "/UI/BellFoodLow");
	LoadRawSoundFolder("NoToolsBell", "/UI/BellNoTools");
	
	LoadRawSoundFolder("NeedStorageBell", "/UI/BellNeedStorage");
	LoadRawSoundFolder("DeathBell", "/UI/BellDeath");
	LoadRawSoundFolder("BabyBornBell", "/UI/BellBabyBorn");

	LoadRawSoundFolder("TrailerMusic", "/UI/TrailerMusic");
	LoadRawSoundFolder("TrailerBeat", "/UI/TrailerBeat");
}



void USoundSystemComponent::TestSoundWave(TArray<uint8> rawFile)
{
	USoundWaveProcedural* sound = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
	sound->SetSampleRate(44100);
	sound->NumChannels = 1;
	sound->Duration = INDEFINITELY_LOOPING_DURATION;
	sound->SoundGroup = SOUNDGROUP_Default;
	sound->bLooping = false;

	sound->OnSoundWaveProceduralUnderflow.BindLambda
	([this](USoundWaveProcedural* Wave, const int32 SamplesNeeded) {
		FillTestAudio(Wave, SamplesNeeded);
	});
	UGameplayStatics::PlaySound2D(GetWorld(), sound, 1, 1, 0);
}

void USoundSystemComponent::FillTestAudio(USoundWaveProcedural* sound, const int32 SamplesNeeded)
{
	const uint32 SampleCount = SamplesNeeded;
	float Frequency = 1046.5;

	TArray<uint8> Buffer;
	Buffer.SetNum(SampleCount * 2);
	int16* data = (int16*)&Buffer[0];

	// Generate a sine wave which slowly fades away.
	for (size_t i = 0; i < SampleCount; ++i)
	{
		float x = (float)(i) / 44100;
		//float v = (float)(TotalSampleCount - i) / TotalSampleCount;
		data[i] = sin(Frequency * x * 2.f * PI) * MAX_int16;
	}

	sound->QueueAudio((const uint8*)data, Buffer.Num());
}