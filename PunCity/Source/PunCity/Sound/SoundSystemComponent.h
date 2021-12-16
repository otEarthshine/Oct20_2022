// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PunCity/Simulation/GameSimulationCore.h"
#include "Components/AudioComponent.h"
#include "PunCity/UI/UISystemBase.h"
#include "PunCity/PunFileUtils.h"
#include <functional>
#include <iomanip>
#include "PunCity/PunGameInstance.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioThread.h"
#include "AudioDevice.h"
#include "FireForgetAudioComponent.h"
#include "Dom/JsonObject.h"
#include "SoundAssets.h"






#include "SoundSystemComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPunSound, All, All);

DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] UpdateParameters"), STAT_PunSoundParam, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _PositionVolume"), STAT_PunSoundParamPositionVolume, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _CheckDespawn"), STAT_PunSoundParamCheckDespawn, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _DelayQueue"), STAT_PunSoundParamDelayQueue, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _Music"), STAT_PunSoundParamMusic, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _BirdTweet"), STAT_PunSoundParamBirdTweet, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _NonMusicLoop"), STAT_PunSoundParamNonMusicLoop, STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _PositionVolume_Position"), STAT_PunSoundParamPositionVolumePosition, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _PositionVolume_Building"), STAT_PunSoundParamPositionVolumeBuilding, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("PUN: [Sound] _PositionVolume_FadingOut"), STAT_PunSoundParamPositionVolumeFadingOut, STATGROUP_Game);


static const std::vector<std::string> TreeBiomeExtensions
{
	"_BorealForest",
	"_Forest",
	"_Jungle",
};
static const std::vector<std::string> NonTreeBiomeExtensions
{
	"_Desert",
	"_Savanna",
	"_Tundra",
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROTOTYPECITY_API USoundSystemComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	USoundSystemComponent()
	{
		_engineImportInConstructor = true;
		LoadRawSoundFolders();
		_engineImportInConstructor = false;
	}

	
	void Init(IGameUIDataSource* dataSource, IGameUIInterface* uiInterface, IPlayerSoundSettings* playerSettings, const UObject* worldContext)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		_LOG(LogPunSound, "Init");
		
		_playerSettings = playerSettings;
		_worldContext = worldContext;
		
		_dataSource = dataSource;
		_uiInterface = uiInterface;

		_lastUpdateTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());

		_isPackagingData = false;

		if (gameInstance) {
			PUN_LOG("SoundSystem Init: %d", gameInstance->SoundNameToAssets.Num());
		}

		InitUI();

		InitMusic();

		InitWindSeasonal();
		InitOtherAmbiences();

#if AUDIO_BIRD
		InitBirdTweets();
#endif

		InitTreeStoneCrop();
		InitResourceDropoffPickup();
		InitDoors();
		InitAnimals();

		// Load sound settings from file
		//SaveOrLoadFromFile(false);
		SaveOrLoadJSONFromFile(false);
	}

	void InitForMainMenu(IPlayerSoundSettings* playerSettings, const UObject* worldContext)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		
		_playerSettings = playerSettings;
		_worldContext = worldContext;

		bool forcedUsedPak = false;		
#if WITH_EDITOR
		forcedUsedPak = true;
#endif
		LoadSounds();
		InitUI();
	}

	void UpdateParameters(int32 playerId, float deltaTime)
	{
		SCOPE_CYCLE_COUNTER(STAT_PunSoundParam);
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound); 
		
		IGameSimulationCore& simulation = _dataSource->simulation();

		_deltaTime = deltaTime;
		float time = UGameplayStatics::GetAudioTimeSeconds(this);

		float gameSpeed = simulation.gameSpeedFloat();

		const int32 updatesPerPositionMove = 10;
		static int32 updateCount = 0;
		updateCount++;
		if (updateCount > updatesPerPositionMove) {
			updateCount = 0;
		}

		// Update audio positions volume etc.
		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamPositionVolume);
			
			// Update audio positions if needed
			for (int32 i = punAudios.Num(); i-- > 0;)
			{
				// Only move once every X times
				if (updateCount != (i % updatesPerPositionMove)) {
					continue;
				}
				
				UAudioComponent* audioComp = punAudios[i]->audio;
				std::string groupName = punAudios[i]->groupName;
				std::string soundName = punAudios[i]->soundName;

				PUN_ENSURE(i < punAudios.Num(), continue);

				// Update position/parameters
				if (audioComp &&
					punAudios[i]->shouldUpdateSpot &&
					punAudios[i]->spotAtom != WorldAtom2::Invalid)
				{
					SCOPE_CYCLE_COUNTER(STAT_PunSoundParamPositionVolumePosition);
					
					FVector location = _dataSource->DisplayLocation(punAudios[i]->spotAtom);
					location.Z = punAudios[i]->spotHeight;
					audioComp->SetWorldLocation(location);


					// Calc volumeMultiplier
					float volumeMultiplier = GetSoundPropertyRef(groupName, soundName, "Volume");
					volumeMultiplier *= GetGroupPropertyRef(groupName, "World_MasterVolume");
					volumeMultiplier *= _playerSettings->masterVolume() * _playerSettings->soundEffectsVolume();;
					volumeMultiplier *= IsolateVolume(groupName, punAudios[i]->soundName);

					SetVolumeMultiplier(audioComp, volumeMultiplier);

					audioComp->SetPitchMultiplier(GetSoundPropertyRef(groupName, soundName, "Pitch") * punAudios[i]->manualPitchMultiplier);

					Adjust3DAttenuation(audioComp, groupName, soundName);

					PUN_CHECK(soundName != "WoodConstruction");
				}

				PUN_ENSURE(i < punAudios.Num(), continue);

				// Building
				if (punAudios[i]->isBuildingSound) // Need isBuildingSound since OneShot sound is also tagged "Building"
				{
					SCOPE_CYCLE_COUNTER(STAT_PunSoundParamPositionVolumeBuilding);
					
					// Spawn one-shot sound
					if (punAudios[i]->hasOneShotSound)
					{
						auto setNextRandomPlayTime = [&](int32 index) {
							float minInterval = GetSoundPropertyRef("Building", punAudios[i]->soundName + "_OneShot", "MinInterval");
							float maxInterval = GetSoundPropertyRef("Building", punAudios[i]->soundName + "_OneShot", "MaxInterval");
							float interval = minInterval + (maxInterval - minInterval) * soundRand.Rand01();

							punAudios[i]->nextRandomSoundPlayTime = time + interval;
						};

						if (punAudios[i]->nextRandomSoundPlayTime < 0) { // initial random play
							setNextRandomPlayTime(i);
						}
						else if (time > punAudios[i]->nextRandomSoundPlayTime) { // interval up
							setNextRandomPlayTime(i);

							Spawn3DSound("Building", punAudios[i]->soundName + "_OneShot",
								punAudios[i]->spotAtom, punAudios[i]->spotHeight);
						}
					}

					// Building's continuous sound
					if (audioComp && punAudios[i]->byteCount != -1)
					{
						auto soundProc = CastChecked<USoundWaveProcedural>(audioComp->Sound);
						int32 byteCountLeft = soundProc->GetAvailableAudioByteCount();
						if (byteCountLeft < punAudios[i]->byteCount / 2)
						{
							const uint64 audioComponentID = audioComp->GetAudioComponentID();
							auto audioDevice = audioComp->GetAudioDevice();

							PunAudioData audioData = GetSoundAsset(punAudios[i]->soundName, punAudios[i]->assetIndex)->GetData();
							FAudioThread::RunCommandOnAudioThread([audioDevice, audioComponentID, audioData]()
							{
								FActiveSound* activeSound = audioDevice->FindActiveSound(audioComponentID);
								if (activeSound) {
									// Must get USoundWaveProcedural again just in case pointer changed?
									CastChecked<USoundWaveProcedural>(activeSound->GetSound())->QueueAudio(audioData.data, audioData.byteCount);
								}
							});
						}
					}
				}

				PUN_ENSURE(i < punAudios.Num(), continue);

				if (punAudios[i]->isFadingOut && audioComp)
				{
					SCOPE_CYCLE_COUNTER(STAT_PunSoundParamPositionVolumeFadingOut);
					
					float fadeVolume = (punAudios[i]->fadeEndTime - time) / (punAudios[i]->fadeEndTime - punAudios[i]->fadeStartTime);

					if (fadeVolume > MinVolume) {
						bool includeCrossFadeIn = punAudios[i]->groupName == "Music" ? false : true;
						float volumeMultiplier = GetPunAudioVolumeWithFadeIn(punAudios[i]->groupName, punAudios[i]->soundName, punAudios[i]->playTime(this), includeCrossFadeIn);
						SetVolumeMultiplier(audioComp, volumeMultiplier * fadeVolume);
					}
					else {
						//punAudios[i]->isDone = true;
						RemovePunAudio(i);
						PUN_LOG("isFadingOut Done %s %s fadeVolume:%f Num:%d", ToTChar(groupName), ToTChar(soundName), fadeVolume, punAudios.Num());
					}
				}
			}
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamCheckDespawn);
			CheckDespawnPunAudio();
		}
		
		//PUN_LOG("_groupNameToAudioStartTime %d", _groupNameToAudioStartTime.size());

		// Delayed audio start queue
		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamDelayQueue);
			
			for (size_t i = _groupNameToAudioStartTime.size(); i-- > 0;)
			{
				if (time >= _groupNameToAudioStartTime[i].second)
				{
					const std::string& groupName = _groupNameToAudioStartTime[i].first;
					auto punAudio = GetAudioComponent(groupName);

					_groupNameToPlayStartTime[groupName] = time;
					UAudioComponent* audio = punAudio->audio;
					if (audio) {
						audio->SetPaused(false);
						audio->Stop();
						audio->Play();
					}
					_groupNameToAudioStartTime.erase(_groupNameToAudioStartTime.begin() + i);

					PUN_LOG("_groupNameToAudioStartTime %s %s", ToTChar(_groupNameToAudioStartTime[i].first), ToTChar(punAudio->soundName));
				}
			}
		}
		

		// Music
		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamMusic);
			UpdateMusic(simulation, playerId);
		}

#if AUDIO_BIRD
		// Bird tweet
		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamBirdTweet);
			UpdateBirdTweets();
		}
#endif

		// Non-music looping volume
		{
			SCOPE_CYCLE_COUNTER(STAT_PunSoundParamNonMusicLoop);

			/*
			 * Adjust Non-Music looping Volume
			 *  FadeIn/OutDuration are used to cap the speed at which the whole group fade in (such as rain fading in)
			 *  CrossFadeDuration is when we change between sounds within the group (WindSpring->WindSummer)
			 */
			for (auto& pair : GroupNameToAudioComponent)
			{
				auto punAudio = pair.Value;
				std::string groupName = punAudio->groupName;
				std::string activeSoundName = punAudio->soundName;

				if (groupName == "Music") {
					continue; // Music updated elsewhere
				}

				float playTime = punAudio->playTime(this);

				// Fade out the old sound if needed to play a new sound
				// Make sure the old sound played for at least 5.8 sec
				if (playTime > 5.8f)
				{
					std::string preferredSoundName = GetPreferredSound(groupName);
					if (preferredSoundName != activeSoundName) {
						CrossFadeInSound(groupName, preferredSoundName);
						continue;
					}
				}

				float volumeMultiplier = GetPunAudioVolumeWithFadeIn(groupName, activeSoundName, playTime, true);

				//! Move to target volume if target volume is valid...
				auto found = _groupToGetTargetVolumeFunc.find(groupName);
				if (found != _groupToGetTargetVolumeFunc.end())
				{
					float targetVolume = found->second(); // Execute targetVolumeFunc
					float& currentVolume = GetHiddenPropertyRef(groupName, "CurrentVolume");

					float fadeInDuration = GetGroupPropertyRef(groupName, "FadeInDuration");
					float fadeOutDuration = GetGroupPropertyRef(groupName, "FadeOutDuration");

					// Move current volume closer to target volume
					UpdateFraction(currentVolume, targetVolume, deltaTime, fadeInDuration, fadeOutDuration);

					float finalVolume = currentVolume * volumeMultiplier;
					if (playTime >= 0.0f) { // TODO: otherwise something stopped it when doing SetVolumeMultiplier]
						SetVolumeMultiplier(punAudio->audio, finalVolume);
					}
					continue;
				}

				// Adjust volume according to fade
				SetVolumeMultiplier(punAudio->audio, volumeMultiplier);

			}
		}
	}

	// Special GetAudioTimeSeconds that suppresses warning
	static float GetAudioTimeSeconds(const UObject* WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		return World ? World->GetAudioTimeSeconds() : 0.f;
	}

	// Note: separate this out since it must be called from PunGameInstance
	void CheckDespawnPunAudio()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		/*
		 * Despawn audios
		 */
		float time = GetAudioTimeSeconds(this);
		
		for (int32 i = punAudios.Num(); i-- > 0;)
		{
			float timeSinceStart = time - punAudios[i]->startTime;
			//bool neverStartForTooLong = timeSinceStart > punAudios[i]->duration * 10.0f && !punAudios[i]->bPlayStarted;

			// Remove building sound from list if out of range
			if (punAudios[i]->isBuildingSound) {
				if (!_dataSource->IsInSampleRange(WorldRegion2(punAudios[i]->regionId).centerTile())) 
				{
					if (punAudios[i]->audio) {
						punAudios[i]->audio->bAutoDestroy = true;
						punAudios[i]->audio->Stop();
					}
					punAudios.RemoveAt(i);

					PUN_LOG("Despawn Audio isBuildingSound out of range %d", i);
					continue;
				}
			}
			
			//// Despawn if needed
			//if (punAudios[i]->isDone || 
			//	(!punAudios[i]->isLooping && neverStartForTooLong))
			//{
			//	if (punAudios[i]->groupName == "Music") {
			//		PUN_LOG("Despawn1 audio null: %s %s Num:%d", ToTChar(punAudios[i]->groupName), ToTChar(punAudios[i]->soundName), punAudios.Num());
			//	}
			//	
			//	//// Remove from building list if needed...
			//	//if (punAudios[i]->isBuildingSound) {
			//	//	PUN_CHECK(punAudios[i]->regionId != -1);
			//	//	PUN_CHECK(punAudios[i]->buildingId != -1);

			//	//	auto regionIdFound = _regionIdToBuildingIdToSound.find(punAudios[i]->regionId);
			//	//	if (regionIdFound != _regionIdToBuildingIdToSound.end()) {
			//	//		regionIdFound->second.erase(punAudios[i]->buildingId);
			//	//	}
			//	//}


			//	if (punAudios[i]->audio) {
			//		punAudios[i]->audio->bAutoDestroy = true;
			//		punAudios[i]->audio->Stop();
			//	}
			//	punAudios.RemoveAt(i);
			//}
			// despawn when its audio was destroyed
			if (i < punAudios.Num() &&
				punAudios[i]->audio == nullptr) 
			{
				//PUN_LOG("Despawn2 audio null: %s %s Num:%d", ToTChar(punAudios[i]->groupName), ToTChar(punAudios[i]->soundName), punAudios.Num());
				
				punAudios.RemoveAt(i);
			}

		}
	}

	float GetPunAudioVolumeWithFadeIn(std::string groupName, std::string soundName, float playTime, bool includeCrossFade)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		float volumeMultiplier = GetSoundPropertyRef(groupName, soundName, "Volume");
		volumeMultiplier *= GetHiddenPropertyRef(groupName, "RandomVolume");
		volumeMultiplier *= GetGroupPropertyRef(groupName, "Group_MasterVolume");
		volumeMultiplier *= _playerSettings->masterVolume();

		volumeMultiplier *= (groupName == "Music") ? _playerSettings->musicVolume() : _playerSettings->ambientVolume();
		
		volumeMultiplier *= IsolateVolume(groupName, soundName);

		if (includeCrossFade) {
			// Cross fade in
			float crossFadeVolume = Clamp01(playTime / GetGroupPropertyRef(groupName, "CrossFadeDuration"));

			return volumeMultiplier * crossFadeVolume; // Need 0.01 to make sure it doesn't just stop the track...
		}
		
		return volumeMultiplier;
	}

	float zoomHeightFraction() { return _dataSource->zoomDistance() / GetCameraZoomAmount(MaxZoomStep); }

	static void SetVolumeMultiplier(UAudioComponent* audio, float volume)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);

		if (!audio) {
			UE_DEBUG_BREAK();
			return;
		}
		
		audio->SetVolumeMultiplier(std::max(volume, MinVolume)); // Need 0.01 to make sure it doesn't just stop the track...
		// TODO: Use bAlwaysPlay instead?
	}

	/*
	 * Raw Sound Import from Saved folder
	 */
	void LoadRawSoundFoldersUIOnly(); // Helper
	void LoadRawSoundFolders();

	void LoadRawSoundFolder(std::string soundName, std::string folderPath) {
		LoadRawSoundFolder(soundName, folderPath.c_str());
	}
	void LoadRawSoundFolder(std::string soundName, const char* folderPath)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);

		if (_engineImportInConstructor)
		{
			FString folderPathF(folderPath);
			if (folderPathF[0] == '/') {
				folderPathF.RemoveAt(0);
			}
			
			FString contentSavePath = FString("/Game/Sound/Sound/") + folderPathF;
			TArray<UObject*> importedAssets;
			EngineUtils::FindOrLoadAssetsByPath(contentSavePath, importedAssets, EngineUtils::ATL_Regular);

			soundNameToWaves.Add(ToFString(soundName), TArray<USoundWave*>());

			for (auto importedAsset : importedAssets)
			{
				USoundWave* sound = Cast<USoundWave>(importedAsset);
				if (sound) {
					sound->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
					soundNameToWaves[ToFString(soundName)].Add(sound);
				}
			}

			//PUN_LOG("Engine Import %s %s imports:%d sounds:%d", ToTChar(soundName), *FString(folderPath), importedAssets.Num(), soundNameToWaves[ToFString(soundName)].Num());
			return;
		}

		// If we are taking sound from cachedEngineImport, folder, or pak
		// Need to make SoundAssets first
		if (!_isPackagingData) {
			gameInstance->TryAddSound(ToFString(soundName), NewObject<USoundAssets>());
		}

		if (_forceUseEngineImport)
		{
			USoundAssets* soundAssets = GetSoundAssets(soundName);
			TArray<USoundWave*>& waves = soundNameToWaves[ToFString(soundName)];
			for (auto wave : waves)
			{
				USoundAsset* soundAsset = NewObject<USoundAsset>();
				soundAsset->sound = wave;
				soundAsset->isRawData = false;

				soundAsset->soundName = soundName;
				soundAsset->assetIndex = soundAssets->assets.Num();

				soundAssets->assets.Add(soundAsset);
			}

			PUN_CHECK(soundAssets->assets.Num() > 0);

			PUN_LOG("Load from engine import %s %s sounds:%d", ToTChar(soundName), *FString(folderPath), soundAssets->assets.Num());

			return;
		}

		checkNoEntry(); // The rest is legacy
		return;
		
		//_LOG(LogPunSound, "LoadRawSoundFolder name:%s _packagingKeys:%d _packagingData:%d", ToTChar(soundName), _packagingKeys.Num(), _packagingData.Num());
		

		FString savedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
		FString findDirectory = savedPath + FString("/Sound/") + FString(folderPath) + FString("/");

		TArray<FString> foundFiles;

		FindOrLookupSoundDirectory(foundFiles, folderPath);


		checkf(foundFiles.Num() > 0, TEXT("Failed to find sound in folder: %s"), *findDirectory);

		for (FString filePath : foundFiles) {
			LoadRawSoundFiles(soundName, filePath);
		}
	}

	void FindOrLookupSoundDirectory(TArray<FString>& foundFiles, const char* folderPath)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		if (shouldLoadFromPak())
		{
			FString folderPathF(folderPath);

			// pathInSoundFolder was saved in _packagingKeys, so we loop through to find the path there instead...
			for (int32 i = 0; i < _packagingKeys.Num(); i++) {
				if (_packagingKeys[i].Left(folderPathF.Len() + 1) == folderPathF + "/") { // Need extra '/' to prevent confusing ButtonClick, ButtonClick2 etc.
					foundFiles.Add(_packagingKeys[i]);
				}
			}
		}
		else {
			FString savedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
			FString findDirectory = savedPath + FString("/Sound/") + FString(folderPath) + FString("/");

			IFileManager::Get().FindFiles(foundFiles, *findDirectory, TEXT(".wav"));

			// foundFiles are just file names, so we append the folder to it
			for (int32 i = 0; i < foundFiles.Num(); i++) {
				foundFiles[i] = folderPath + FString("/") + foundFiles[i];
			}
		}
	}

	//// TODO: remove
	//bool TryLoadRawSoundFolder(std::string soundName, const char* folderPath)
	//{
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);


	//	TArray<FString> foundFiles;

	//	FindOrLookupSoundDirectory(foundFiles, folderPath);
	//	

	//	if (foundFiles.Num() <= 0) {
	//		return false;
	//	}

	//	if (!_isPackagingData) {
	//		gameInstance->TryAddSound(ToFString(soundName), NewObject<USoundAssets>());
	//		//gameInstance->SoundNameToAssets[ToFString(soundName)] = nullptr;
	//	}

	//	for (FString filePath : foundFiles) {
	//		LoadRawSoundFiles(soundName, filePath);
	//	}

	//	return true;
	//}

	//// For buildings we need /OneShot folder as well..
	//void LoadRawSoundFolder_Building(std::string soundName, const char* folderPath)
	//{
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
	//	
	//	LoadRawSoundFolder(soundName, folderPath);
	//	
	//	std::string oneShotPath = folderPath;
	//	oneShotPath += "/OneShot";
	//	TryLoadRawSoundFolder(soundName + "_OneShot", oneShotPath.c_str());
	//}
	
	/*
	 * Load from pak
	 * Load from sound files
	 * Package data to pak
	 */
	void LoadRawSoundFiles(std::string soundName, const FString& pathInSoundFolder)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);

		TArray<uint8> loadedRawWAV; // Only for if we load from file
		FString savedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
		FString soundPath = savedPath + FString("/Sound/") + pathInSoundFolder;

		int32 foundIndex = -1;
		
		if (shouldLoadFromPak())
		{
			//_LOG(LogPunSound, "LoadRawSoundFiles name:%s path:%s _packagingKeys:%d _packagingData:%d", ToTChar(soundName), *pathInSoundFolder, _packagingKeys.Num(), _packagingData.Num());
			
			// Load from Pak with Keys as path
			foundIndex = _packagingKeys.Find(pathInSoundFolder);
			checkf(foundIndex != INDEX_NONE, TEXT("Sound not loaded: %s"), *pathInSoundFolder);

			//_LOG(LogPunSound, "LoadRawSoundFiles foundIndex:%d", foundIndex);
		}
		else
		{
			//_LOG(LogPunSound, "LoadRawSoundFiles name:%s _packagingKeys:%d _packagingData:%d", ToTChar(soundName), _packagingKeys.Num(), _packagingData.Num());
			
			if (!FFileHelper::LoadFileToArray(loadedRawWAV, *soundPath))
			{
				std::string soundPathS = ToStdString(soundPath);
				PUN_LOG("Failed to load raw file from %s", ToTChar(soundPathS)); 
				return;
			}
		}

		PUN_CHECK(loadedRawWAV.Num() > 0 ||
					_packagingData.Num() > 0 ||
					_packagingDataCompressed.Num() > 0);
		//PunLog("LoadRaw success size:" + std::to_string(rawWAV.Num()));

		// Marked for packaging, this function is called to package sound files
		if (_isPackagingData) 
		{
			// For packaging data, we just take rawWAV, extract rawPCM and put it in _packagingData
			// All of them will be compressed later just before going into the pak file
			
			PUN_LOG("_isPackagingData[%d]:%s", _packagingData.Num(), *soundPath);
			_packagingKeys.Add(pathInSoundFolder);
			
			FWaveModInfo waveInfo;
			if (waveInfo.ReadWaveInfo(loadedRawWAV.GetData(), loadedRawWAV.Num())) 
			{
				TArray<uint8> rawPCM;
				rawPCM.SetNum(waveInfo.SampleDataSize);
				FMemory::Memcpy(rawPCM.GetData(), waveInfo.SampleDataStart, waveInfo.SampleDataSize);
				
				PunWaveModInfo waveModInfo(waveInfo);
				_packagingWaveInfo.Add(waveModInfo);
				_packagingData.Add(rawPCM);
			}
			else {
				UE_DEBUG_BREAK();
			}

			return;
		}

		/*
		 * If this is not marked for packaging, prepare it for usage in game
		 */

		auto setSoundAsset = [&](PunWaveModInfo& waveModInfo, TArray<uint8>& data, TArray<uint8>& compressedData)
		{
			USoundAssets* soundAssets = GetSoundAssets(soundName);

			USoundAsset* soundAsset = NewObject<USoundAsset>();
			soundAsset->isRawData = true;
			soundAsset->waveInfo = waveModInfo;
			
			soundAsset->data = NewObject<UPoolArray>();
			soundAsset->data->data = data;
			
			soundAsset->compressedData = NewObject<UPoolArray>();
			soundAsset->compressedData->data = compressedData;
			
			soundAsset->soundName = soundName;
			soundAsset->assetIndex = soundAssets->assets.Num();

			soundAssets->assets.Add(soundAsset);


			// Debug...
			std::string soundPathS = ToStdString(soundPath);
			std::stringstream ss;
			ss << "Read WaveInfo success: " << soundPathS;
			ss << ", assetIndex:" << soundAsset->assetIndex;
			//PunLog(ss.str());
		};

		// Read waveHeader if it wasn't from pak file
		if (!shouldLoadFromPak())
		{
			FWaveModInfo waveInfo;
			if (waveInfo.ReadWaveInfo(loadedRawWAV.GetData(), loadedRawWAV.Num()))
			{
				TArray<uint8> rawPCM;
				rawPCM.SetNum(waveInfo.SampleDataSize);
				FMemory::Memcpy(rawPCM.GetData(), waveInfo.SampleDataStart, waveInfo.SampleDataSize);

				PunWaveModInfo waveModInfo(waveInfo);

				TArray<uint8> dummyCompressedData;
				setSoundAsset(waveModInfo, rawPCM, dummyCompressedData);
				return;
			}
			
			PUN_LOG("[ERROR]Failed to read WaveInfo");
			return;
		}

		PunWaveModInfo waveModInfo = _packagingWaveInfo[foundIndex];

		//PUN_LOG("waveModInfo.sampleDataSize:%d rawWAV:%d", waveModInfo.sampleDataSize, rawWAV.Num());
		
		setSoundAsset(waveModInfo, _packagingData[foundIndex], _packagingDataCompressed[foundIndex]);

		//PUN_LOG("Set Sound Assets from pak");
	}

	/*
	 * Important!
	 */
//	void PackageSound()
//	{
//		checkNoEntry();
//		
//		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
//		
//		_isPackagingData = true;
//		_packagingKeys.Empty();
//		_packagingData.Empty();
//		_packagingDataCompressed.Empty();
//		
//		LoadRawSoundFolders();
//
//		FAES::FAESKey aesKey = GetAesKey();
//
//		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
//		FString saveFileName = "Paks/Extras.paka";
//		
//		PunFileUtils::SaveFile(path + saveFileName, [&](FArchive& Ar) 
//		{
//			Ar << _packagingKeys;
//			Ar << _packagingWaveInfo;
//			
//			for (int32 i = 0; i < _packagingKeys.Num(); i++) 
//			{
//				// OGG Compression
//				FAudioPlatformOggModule oggModule;
//				FAudioFormatOgg* oggAudioFormat = static_cast<FAudioFormatOgg*>(oggModule.GetAudioFormat());
//				FSoundQualityInfo qualityInfo;
//				qualityInfo.Quality = 40;
//				qualityInfo.NumChannels = _packagingWaveInfo[i].channelCount;
//				qualityInfo.SampleRate = _packagingWaveInfo[i].samplesPerSec;
//				qualityInfo.SampleDataSize = _packagingWaveInfo[i].sampleDataSize;
//				qualityInfo.Duration = _packagingWaveInfo[i].duration;
//				qualityInfo.bStreaming = false;
//
//				//PUN_LOG("Ogg Cooking RawPCM:%d", _packagingData[i].Num());
//
//				TArray<uint8> compressedPCM;
//				oggAudioFormat->Cook(NAME_OGG, _packagingData[i], qualityInfo, compressedPCM);
//
//				//PUN_LOG("Ogg Cooking Compressed:%d", compressedPCM.Num());
//				
//				Ar << compressedPCM;
//			}
//		}, &aesKey);
//
//		_packagingKeys.Empty();
//		_packagingData.Empty();
//		_packagingDataCompressed.Empty();
//		_isPackagingData = false;
//	}
//
//	void UsePackageSound()
//	{
//		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
//		
//		bool forcedUsedPak = false;
//#if WITH_EDITOR
//		forcedUsedPak = true;
//#endif
//		LoadSounds(forcedUsedPak);
//	}

	static FAES::FAESKey GetAesKey() {
		FAES::FAESKey aesKey;
		for (int32 i = 0; i < 32; i++) {
			aesKey.Key[i] = (i * i - i) % 256;
		}
		return aesKey;
	}

	void LoadSounds()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		_LOG(LogPunSound, "LoadSounds");
		
		SCOPE_TIMER_FILTER(5000, "Init LoadSounds");

		_isLoadingFromPak = false;

		// Use Sound Folder if there is one
		FString soundDirPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + FString("/Sound");

		TArray<FString> foundSoundDirs;
		IFileManager::Get().FindFiles(foundSoundDirs, *soundDirPath, false, true);

		
		//// If no SoundFolder, use pak's sound
		//if (_forceUseEngineImport) {
		//	
		//}
		//else if (forceUsePakSound || foundSoundDirs.Num() == 0)
		//{
		//	_LOG(LogPunSound, "LoadSounds pak");
		//	
		//	// Use 
		//	FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
		//	FString saveFileName = "Paks/Extras.paka";

		//	TArray<FString> foundFiles;
		//	IFileManager::Get().FindFiles(foundFiles, *(path + saveFileName), true, false);

		//	checkf(foundFiles.Num() > 0, TEXT("Failed to find Extras.paka"));

		//	_isLoadingFromPak = true;
		//	if (foundFiles.Num() == 0) {
		//		_isLoadingFromPak = false;
		//		_bFoundSoundFiles = false;
		//	} else {
		//		_bFoundSoundFiles = true;
		//	}

		//	// If not yet loaded from the menu, load it
		//	if (_packagingKeys.Num() == 0)
		//	{
		//		_LOG(LogPunSound, "LoadSounds pak, load files begin");

		//		FAES::FAESKey aesKey = GetAesKey();
		//		
		//		PunFileUtils::LoadFile(path + saveFileName, [&](FArchive& Ar) 
		//		{
		//			Ar << _packagingKeys;
		//			Ar << _packagingWaveInfo;
		//			
		//			_packagingData.SetNum(_packagingKeys.Num());
		//			_packagingDataCompressed.SetNum(_packagingKeys.Num());
		//			
		//			for (int32 i = 0; i < _packagingKeys.Num(); i++) 
		//			{
		//				TArray<uint8> compressedPCM;
		//				Ar << compressedPCM;

		//				if (_packagingKeys[i].Left(5) == "Music")
		//				{
		//					SCOPE_TIMER_("No Decompress OGG %s bytes:%d", *_packagingKeys[i], compressedPCM.Num());
		//					_packagingDataCompressed[i] = compressedPCM;
		//				}
		//				// OGG Decompression for non-music
		//				else {
		//					SCOPE_TIMER_("Decompress OGG %s", *_packagingKeys[i]);
		//					DecompressOGG(_packagingWaveInfo[i], compressedPCM, _packagingData[i]);
		//				}
		//				//// rawPCM.SetNum(0);
		//				//FVorbisAudioInfo AudioInfo;

		//				//// Parse the audio header for the relevant information
		//				//FSoundQualityInfo qualityInfo = _packagingWaveInfo[i].qualityInfo();
		//				//if (!AudioInfo.ReadCompressedInfo(compressedPCM.GetData(), compressedPCM.Num(), &qualityInfo)) {
		//				//	PUN_LOG("Ogg ReadCompressedInfo failed");
		//				//}

		//				//// Decompress all the sample data
		//				//_packagingData[i].Empty(qualityInfo.SampleDataSize);
		//				//_packagingData[i].AddZeroed(qualityInfo.SampleDataSize);
		//				//AudioInfo.ExpandFile(_packagingData[i].GetData(), &qualityInfo);
		//			}
		//		}, &aesKey);

		//		_LOG(LogPunSound, "LoadSounds pak, load files end _packagingKeys:%d _packagingData:%d", _packagingKeys.Num(), _packagingData.Num());
		//	}
		//	else {
		//		_LOG(LogPunSound, "LoadSounds pak: No Sound Folder");
		//	}
		//}

		LoadRawSoundFolders();

		PUN_CHECK(gameInstance->SoundNameToAssets.Num());

		_packagingKeys.Empty();
		_packagingWaveInfo.Empty();
		_packagingData.Empty();
		_packagingDataCompressed.Empty();
	}

	static uint8 DecompressOGG(PunWaveModInfo waveModInfo, TArray<uint8>& compressedPCM, TArray<uint8>& decompressedPCM)
	{
		//PUN_LOG("DecompressOGG compressedSize:%d", compressedPCM.Num());
		
		// OGG Decompression
		FVorbisAudioInfo AudioInfo;

		// Parse the audio header for the relevant information
		FSoundQualityInfo qualityInfo = waveModInfo.qualityInfo();
		if (!AudioInfo.ReadCompressedInfo(compressedPCM.GetData(), compressedPCM.Num(), &qualityInfo)) {
			PUN_LOG("Ogg ReadCompressedInfo failed");
		}

		// Decompress all the sample data
		decompressedPCM.Empty(qualityInfo.SampleDataSize);
		decompressedPCM.AddZeroed(qualityInfo.SampleDataSize);
		AudioInfo.ExpandFile(decompressedPCM.GetData(), &qualityInfo);
		
		return 1;
	}

private:
	/*
	 * ProceduralSound Helpers
	 */

	UFireForgetAudioComponent* CreateRuntimeSound_EngineImport(std::string groupName, USoundAsset* soundAsset, bool isLooping)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);

		UFireForgetAudioComponent* fireAndForget = NewObject<UFireForgetAudioComponent>();
		fireAndForget->startTime = UGameplayStatics::GetAudioTimeSeconds(this);
		fireAndForget->duration = soundAsset->sound->GetDuration();
		fireAndForget->soundName = soundAsset->soundName;
		fireAndForget->groupName = groupName;
		fireAndForget->sound = soundAsset->sound;
		fireAndForget->assetIndex = soundAsset->assetIndex;
		fireAndForget->isLooping = isLooping;

		soundAsset->sound->bLooping = isLooping; // Note: This means the same sound can't be used for both Loop/NonLoop

		if (groupName == "Music") {
			PUN_LOG("CreateRuntimeSound_EngineImport %s", ToTChar(soundAsset->soundName));
		}

		return fireAndForget;
	}

	UFireForgetAudioComponent* CreateRuntimeSound(std::string groupName, USoundAsset* soundAsset, bool isLooping)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		PunWaveModInfo waveInfo = soundAsset->waveInfo;

#if WITH_EDITOR
		std::string soundWaveName = groupName + std::string("_") + soundAsset->soundName;
		USoundWaveProcedural* sound = NewObject<USoundWaveProcedural>(this, FName(ToTChar(soundWaveName)));
#else
		USoundWaveProcedural* sound = NewObject<USoundWaveProcedural>(this);
#endif
		
		sound->SetSampleRate(waveInfo.samplesPerSec);
		sound->NumChannels = waveInfo.channelCount;
		sound->Duration = waveInfo.duration;
		sound->SoundGroup = SOUNDGROUP_Default;
		sound->TotalSamples = waveInfo.samplesPerSec * waveInfo.duration;
		sound->bLooping = false;

		std::stringstream ss;
		ss << "Create USoundWaveProcedural ";
		ss << " SampleRate:" << waveInfo.samplesPerSec;
		ss << " NumChannels:" << sound->NumChannels;
		ss << " Duration:" << sound->Duration;
		ss << " TotalSamples:" << sound->TotalSamples;
		ss << " bLooping:" << sound->bLooping;
		PunLog(ss.str());

		UFireForgetAudioComponent* fireAndForget = NewObject<UFireForgetAudioComponent>();
		fireAndForget->startTime = UGameplayStatics::GetAudioTimeSeconds(this);
		fireAndForget->duration = waveInfo.duration;
		fireAndForget->soundName = soundAsset->soundName;
		fireAndForget->groupName = groupName;
		fireAndForget->sound = sound;
		fireAndForget->assetIndex = soundAsset->assetIndex;
		fireAndForget->isLooping = isLooping;

		return fireAndForget;
	}

	//void SetupRuntimeSound(UFireForgetAudioComponent* punAudio, UAudioComponent* audio)
	//{
	//	LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
	//	
	//	//PUN_LOG("SetupRuntimeSound %s %s", ToTChar(punAudio->groupName), ToTChar(punAudio->soundName));
	//	
	//	punAudio->audio = audio;

	//	static int32 audioIndex = 0;
	//	punAudio->Rename(*FString::Printf(TEXT("%s_%d"), ToTChar(punAudio->soundName), audioIndex));
	//	audioIndex++;

	//	punAudios.Add(punAudio);

	//	PUN_CHECK(punAudio->soundName != "");

	//	USoundWaveProcedural* soundProc = CastChecked<USoundWaveProcedural>(audio->Sound);

	//	if (punAudio->isLooping)
	//	{
	//		soundProc->OnSoundWaveProceduralUnderflow.BindLambda([this, punAudio](USoundWaveProcedural* soundLamb, const int32 SamplesNeeded)
	//		{	
	//			PunAudioData audioData = GetSoundAsset(punAudio->soundName, punAudio->assetIndex)->GetData();
	//			soundLamb->QueueAudio(audioData.data, audioData.byteCount);
	//			punAudio->byteCount = audioData.byteCount;

	//			//PUN_LOG("OnSoundWaveProceduralUnderflow isLooping AvailableByteCount:%d punAudio->byteCount:%d", soundLamb->GetAvailableAudioByteCount(), punAudio->byteCount);
	//		});
	//	}
	//	else
	//	{
	//		soundProc->OnSoundWaveProceduralUnderflow.BindLambda([this, punAudio](USoundWaveProcedural* soundLamb, const int32 SamplesNeeded)
	//		{
	//			//float time = UGameplayStatics::GetAudioTimeSeconds(this);
	//			if (punAudio->bPlayStarted) { // If more than half duration passed, end this
	//				punAudio->isDone = true;
	//			}
	//			else {
	//				// TODO: find why there is soundName = ""
	//				if (punAudio->soundName != "") {
	//					//_LOG(PunSound, "OnSoundWaveProceduralUnderflow Start soundName:%s assetIndex:%d", ToTChar(punAudio->soundName), punAudio->assetIndex);

	//					USoundAsset* soundAsset = GetSoundAsset(punAudio->soundName, punAudio->assetIndex);
	//					//_LOG(PunSound, "-- OnSoundWaveProceduralUnderflow %s", ToTChar(soundAsset->ToString()));

	//					
	//					PunAudioData audioData = soundAsset->GetData();
	//					soundLamb->QueueAudio(audioData.data, audioData.byteCount);
	//					punAudio->bPlayStarted = true;
	//					punAudio->byteCount = audioData.byteCount;
	//				}
	//				else {
	//					punAudio->isDone = true;
	//				}
	//			}
	//		});
	//	}
	//}

	void TestSoundWave(TArray<uint8> rawFile);
	void FillTestAudio(USoundWaveProcedural* sound, const int32 SamplesNeeded);

public:
	/*
	 * Data
	 */
	std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, float>>> groupToSoundToSoundPropertyNameToValue;
	std::unordered_map<std::string, std::unordered_map<std::string, float>> groupToPropertyNameToValue;

	void Add2DSoundProperties(std::string groupName, std::string soundName, std::unordered_map<std::string, float> propertyTypeToValue = std::unordered_map<std::string, float>())
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		if (propertyTypeToValue.empty()) {
			propertyTypeToValue = defaultSoundPropertyTypeToValue;
		}
		groupToSoundToSoundPropertyNameToValue[groupName][soundName] = propertyTypeToValue;
	}
	void Add3DSoundProperties(std::string groupName, std::string soundName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		auto propertyTypeToValue = defaultSoundPropertyTypeToValue;
		propertyTypeToValue.emplace("AttenuationRadius", 20.0f);
		propertyTypeToValue.emplace("AttenuationFalloffDistance", 90.0f);

		propertyTypeToValue.emplace("LowPassFilter_Radius", 20.0f);
		propertyTypeToValue.emplace("LowPassFilter_FalloffDistance", 90.0f);
		
		propertyTypeToValue.emplace("PlayProbability", 1.0f);

		groupToSoundToSoundPropertyNameToValue[groupName][soundName] = propertyTypeToValue;
	}
	void AddSoundProperty(std::string groupName, std::string soundName, std::string propertyName, float value)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		groupToSoundToSoundPropertyNameToValue[groupName][soundName][propertyName] = value;
	}

	float& GetSoundPropertyRef(std::string groupName, std::string soundName, std::string soundPropertyName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		PUN_CHECK(groupToSoundToSoundPropertyNameToValue.find(groupName) != groupToSoundToSoundPropertyNameToValue.end());
		PUN_CHECK(groupToSoundToSoundPropertyNameToValue[groupName].find(soundName) != groupToSoundToSoundPropertyNameToValue[groupName].end());
		PUN_CHECK(groupToSoundToSoundPropertyNameToValue[groupName][soundName].find(soundPropertyName) != groupToSoundToSoundPropertyNameToValue[groupName][soundName].end());

		return groupToSoundToSoundPropertyNameToValue[groupName][soundName][soundPropertyName];
	}
	std::vector<std::string> GetSoundNamesFromGroup(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		auto& soundToSoundPropertyNameToValue = groupToSoundToSoundPropertyNameToValue[groupName];
		std::vector<std::string> soundNames;
		for (auto& soundPropertyNameToValue : soundToSoundPropertyNameToValue) {
			soundNames.push_back(soundPropertyNameToValue.first);
		}
		return soundNames;
	}

	void AddGroupProperty(std::string groupName, std::string groupPropertyName, float value)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		groupToPropertyNameToValue[groupName][groupPropertyName] = value;
	}
	float& GetGroupPropertyRef(std::string groupName, std::string groupPropertyName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		PUN_CHECK(groupToPropertyNameToValue.find(groupName) != groupToPropertyNameToValue.end());
		PUN_CHECK(groupToPropertyNameToValue[groupName].find(groupPropertyName) != groupToPropertyNameToValue[groupName].end());
		return groupToPropertyNameToValue[groupName][groupPropertyName];
	}

	void SaveOrLoadJSONFromFile(bool isSaving)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		FString path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

		auto saveOrLoadJsonObject = [&](const TSharedRef<FJsonObject>& jsonObject)
		{
			for (auto& groupIt : groupToPropertyNameToValue) {
				std::unordered_map<std::string, float>& propertyNameToValue = groupIt.second;

				if (isSaving) {
					auto groupJsonObject = MakeShared<FJsonObject>();
					jsonObject->SetObjectField(ToFString(groupIt.first), groupJsonObject);
				}
				
				for (auto& propertyIt : propertyNameToValue)
				{
					if (isSaving) {
						auto groupJsonObject = jsonObject->GetObjectField(ToFString(groupIt.first));
						groupJsonObject->SetNumberField(ToFString(propertyIt.first), propertyIt.second);
					}
					else {
						const TSharedPtr<FJsonObject>* groupJsonObject;
						if (jsonObject->TryGetObjectField(ToFString(groupIt.first), groupJsonObject)) {
							double value;
							if (groupJsonObject->Get()->TryGetNumberField(ToFString(propertyIt.first), value)) {
								propertyIt.second = value;

								//_LOG(PunSound, "Set %s, %s, as %f", ToTChar(groupIt.first), ToTChar(propertyIt.first), value);
							}
						}
					}
				}
			}

			for (auto& groupIt : groupToSoundToSoundPropertyNameToValue) {
				std::unordered_map<std::string, std::unordered_map<std::string, float>>& soundToSoundPropertyNameToValue = groupIt.second;

				for (auto& soundIt : soundToSoundPropertyNameToValue) {
					std::unordered_map<std::string, float>& propertyNameToValue = soundIt.second;

					if (isSaving) {
						auto groupJsonObject = jsonObject->GetObjectField(ToFString(groupIt.first));
						auto soundJsonObject = MakeShared<FJsonObject>();
						groupJsonObject->SetObjectField(ToFString(soundIt.first), soundJsonObject);
					}
					
					for (auto& propertyIt : propertyNameToValue)
					{
						if (isSaving) {
							auto groupJsonObject = jsonObject->GetObjectField(ToFString(groupIt.first));
							auto soundJsonObject = groupJsonObject->GetObjectField(ToFString(soundIt.first));
							soundJsonObject->SetNumberField(ToFString(propertyIt.first), propertyIt.second);
						}
						else {
							const TSharedPtr<FJsonObject>* groupJsonObject;
							if (jsonObject->TryGetObjectField(ToFString(groupIt.first), groupJsonObject)) 
							{
								const TSharedPtr<FJsonObject>* soundJsonObject;
								if (groupJsonObject->Get()->TryGetObjectField(ToFString(soundIt.first), soundJsonObject))
								{
									double value;
									if (soundJsonObject->Get()->TryGetNumberField(ToFString(propertyIt.first), value)) {
										propertyIt.second = value;

										//_LOG(PunSound, "Set %s, %s, %s, as %f", ToTChar(groupIt.first), ToTChar(soundIt.first), ToTChar(propertyIt.first), value);
									}
								}
							}
						}
					}
				}
			}
		};

		
		
		FString saveFileName = "Paks/SoundSettings.json";

		if (isSaving) {
			auto jsonObject = MakeShared<FJsonObject>();
			saveOrLoadJsonObject(jsonObject);
			
			FString jsonString;
			
			TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&jsonString);
			FJsonSerializer::Serialize(jsonObject, writer);
			
			FFileHelper::SaveStringToFile(jsonString, *(path + saveFileName));
		}
		else {
			FString jsonString;
			FFileHelper::LoadFileToString(jsonString, *(path + saveFileName));

			TSharedPtr<FJsonObject> jsonObject(new FJsonObject());
			
			TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);
			FJsonSerializer::Deserialize(reader, jsonObject);

			saveOrLoadJsonObject(jsonObject.ToSharedRef());
		}
	}

private:
	std::unordered_map<std::string, float> defaultSoundPropertyTypeToValue = 
	{
		{ "Pitch", 1.0f},
		{ "PitchModulationMin", 1.0f},
		{ "PitchModulationMax", 1.1f},

		{ "ManualOverride", -1.0f},

		{ "Volume", 1.0f },
		{ "VolumeModulationMin", 1.0f },
		{ "VolumeModulationMax", 1.1f },

		{ "LowPassFilterMin", 10000.0f },
		{ "LowPassFilterMax", 20000.0f },

		{ "PlayCount", 0 }, // Note this is special, can't be manipulated, and will be display as textBox that will be updated every tick...
		{ "LastPlayed", 0 },
	};

	std::unordered_map<std::string, std::function<std::string()>> _groupToGetPreferredSoundFunc;

	/*
	 * Data helpers
	 */

	void Add3DAudioGroupProperties(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		AddGroupProperty(groupName, "World_MasterVolume", 1.0f);
	}


	std::string GetManualOverrideSound(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		// If ManualVolumeOverride is not -1 (invalid), return the sound
		// If there is more than one manual override, return the maximum...
		auto& soundToSoundPropertyNameToValue = groupToSoundToSoundPropertyNameToValue[groupName];

		std::string maxManualOverrideSound = "";
		float maxManualVolumeOverride = -1.0f;
		for (auto it : soundToSoundPropertyNameToValue) {
			std::unordered_map<std::string, float>& propertyNameToValue = it.second;
			if (propertyNameToValue["ManualOverride"] >= 0.0f &&
				propertyNameToValue["ManualOverride"] >= maxManualVolumeOverride) {
				maxManualOverrideSound = it.first;
				maxManualVolumeOverride = propertyNameToValue["ManualOverride"];
			}
		}
		return maxManualOverrideSound;
	}
	std::string GetPreferredSound(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		std::string soundName = GetManualOverrideSound(groupName);
		if (soundName != "") {
			return soundName;
		}

		if (_groupToGetPreferredSoundFunc.find(groupName) != _groupToGetPreferredSoundFunc.end()) {
			return _groupToGetPreferredSoundFunc[groupName]();
		}

		// if there is no function to get preferred sound, just use the existing sound
		return GetActiveSound(groupName);
	}

	/*
	 * TargetVolume
	 *  If this is set, the volume will be adjusted according to the function given
	 *  Note:
	 *   - "Volume" will multiply with this to get the final volume
	 *   - The update on target volume will not be performed if there is a manual override
	 */
	std::unordered_map<std::string, std::unordered_map<std::string, float>> _hiddenGroupToPropertyNameToValue; // This is hidden from UI and not saved
	std::unordered_map<std::string, std::function<float()>> _groupToGetTargetVolumeFunc;

	void AddHiddenProperty(std::string groupName, std::string groupPropertyName, float value)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		_hiddenGroupToPropertyNameToValue[groupName][groupPropertyName] = value;
	}
	float& GetHiddenPropertyRef(std::string groupName, std::string groupPropertyName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		PUN_CHECK(_hiddenGroupToPropertyNameToValue.find(groupName) != _hiddenGroupToPropertyNameToValue.end());
		PUN_CHECK(_hiddenGroupToPropertyNameToValue[groupName].find(groupPropertyName) != _hiddenGroupToPropertyNameToValue[groupName].end());
		return _hiddenGroupToPropertyNameToValue[groupName][groupPropertyName];
	}

	void InitTargetVolume(std::string groupName, std::function<float()> getTargetVolumeFunc)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		AddHiddenProperty(groupName, "CurrentVolume", 0.0f);
		_groupToGetTargetVolumeFunc[groupName] = getTargetVolumeFunc;
	}

public:
	/*
	 * Debug
	 */
	std::string GetdebugStr(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		if (_debugDescriptionFuncs.find(groupName) != _debugDescriptionFuncs.end()) {
			return _debugDescriptionFuncs[groupName](groupName);
		}
		return "";
	}

private:
	std::unordered_map<std::string, std::function<std::string(std::string)>> _debugDescriptionFuncs;

	// TODO: this could be "LastPlayed" variable??
	std::unordered_map<std::string, float> _groupNameToPlayStartTime;

	float GetPlayStartTime(std::string groupName) {
		return _groupNameToPlayStartTime[groupName];
	}

public:
	/*
	 * Audio Components
	 */

	void ResetSound()
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		// Stop the AudioComponent to activate OnAudioFinish()
		for (auto& pair : GroupNameToAudioComponent) {
			if (pair.Value->audio) {
				pair.Value->audio->Stop();
			}
		}
	}

	UFireForgetAudioComponent* Spawn3DSound(std::string groupName, std::string soundName, WorldAtom2 worldAtom, float height, bool usePlayProbability = true, bool isLooping = false, float speed = 1.0f)
	{
		if (PunSettings::TrailerSession) {
			return nullptr;
		}
		// Note: usePlayProbability is needed to separate Animal's UI sound vs animal random ambient
		if (usePlayProbability && soundRand.Rand01() > GetSoundPropertyRef(groupName, soundName, "PlayProbability")) {
			return nullptr;
		}
		

		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);

		// Less construction sound at high game speed
		if (soundName == "WoodConstruction") {
			int32 gameSpeed = std::max(1, _dataSource->simulation().gameSpeed());
			if (soundRand.Rand() % gameSpeed != 0) {
				return nullptr;
			}
		}

		if (IsolateVolume(groupName, soundName) == 0.0f) {
			return nullptr;
		}

		FVector spawnLocation = _dataSource->DisplayLocation(worldAtom);
		spawnLocation.Z = height;

		USoundAsset* soundAsset = GetRandomSoundAsset(soundName);
		PUN_CHECK(soundAsset);

		USoundWave* sound = soundAsset->sound;

		// RawData sound
		// FireAndForget is needed for moving the sound 3D location
		UFireForgetAudioComponent* fireAndForget = nullptr;
		PUN_CHECK(!soundAsset->isRawData);
		if (soundAsset->isRawData) {
			//fireAndForget = CreateRuntimeSound(groupName, soundAsset, false);
			//sound = fireAndForget->sound;
			//fireAndForget->spotAtom = worldAtom;
			//fireAndForget->spotHeight = height;
		}
		else {
			fireAndForget = CreateRuntimeSound_EngineImport(groupName, soundAsset, isLooping);
			fireAndForget->spotAtom = worldAtom;
			fireAndForget->spotHeight = height;

			fireAndForget->shouldUpdateSpot = (soundName != "WoodConstruction") &&
												(soundName != "ConstructionCompleteRoad") &&
												(soundName != "CropPlanting") &&
												(soundName != "BowShoot");
		}

		UAudioComponent* audio = UGameplayStatics::SpawnSoundAtLocation(this, sound, spawnLocation);
		audio->bAutoDestroy = true;
		
		// RawData sound
		if (soundAsset->isRawData) {
			//SetupRuntimeSound(fireAndForget, audio);
		} else {
			AddPunAudio(fireAndForget, audio);
		}

		float soundPropertyVolume = GetSoundPropertyRef(groupName, soundName, "Volume");
		float masterVolume = GetGroupPropertyRef(groupName, "World_MasterVolume");

		audio->VolumeMultiplier = soundPropertyVolume * masterVolume * _playerSettings->masterVolume() * _playerSettings->soundEffectsVolume();
		audio->VolumeModulationMin = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMin");
		audio->VolumeModulationMax = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMax");

		//float pitchFromSpeed = 1.0f / speed;
		fireAndForget->manualPitchMultiplier = speed;
		
		//! Pitch is adjusted in Pitch 
		audio->PitchMultiplier = GetSoundPropertyRef(groupName, soundName, "Pitch") * fireAndForget->manualPitchMultiplier;
		audio->PitchModulationMin = GetSoundPropertyRef(groupName, soundName, "PitchModulationMin");
		audio->PitchModulationMax = GetSoundPropertyRef(groupName, soundName, "PitchModulationMax");

		audio->SetLowPassFilterEnabled(true);

		Adjust3DAttenuation(audio, groupName, soundName);


		float lowpassFilterRadius = 10.0f * GetSoundPropertyRef(groupName, soundName, "LowPassFilter_Radius");
		float lowpassFilterFalloffDistance = 10.0f * GetSoundPropertyRef(groupName, soundName, "LowPassFilter_FalloffDistance");
		float lowPassMin = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMin");
		float lowPassMax = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMax");
		
		FSoundAttenuationSettings settings = audio->AttenuationOverrides;
		float tileRadius = GetSoundPropertyRef(groupName, soundName, "AttenuationRadius");
		settings.AttenuationShapeExtents = FVector(tileRadius * 10.0f, 0.0f, 0.0f);
		settings.FalloffDistance = GetSoundPropertyRef(groupName, soundName, "AttenuationFalloffDistance") * 10.0f;

		settings.AbsorptionMethod = EAirAbsorptionMethod::Linear;
		settings.LPFRadiusMin = lowpassFilterRadius;
		settings.LPFRadiusMax = lowpassFilterRadius + lowpassFilterFalloffDistance;
		settings.LPFFrequencyAtMin = lowPassMax;
		settings.LPFFrequencyAtMax = lowPassMin;

		audio->AdjustAttenuation(settings);
		
		audio->bAllowSpatialization = true;
		audio->bOverrideAttenuation = true;

		audio->Play();


		GetSoundPropertyRef(groupName, soundName, "PlayCount")++;
		GetSoundPropertyRef(groupName, soundName, "LastPlayed") = UGameplayStatics::GetAudioTimeSeconds(this);

		return fireAndForget;
	}

	void Spawn2DSound(std::string groupName, std::string soundName)
	{
		if (PunSettings::TrailerSession 
			//&& soundName != "TrailerMusic" 
			//&& soundName != "TrailerBeat"
			) {
			return;
		}
		if (IsolateVolume(groupName, soundName) == 0.0f) {
			return;
		}
		
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		USoundAsset* soundAsset = GetRandomSoundAsset(soundName);
		PUN_CHECK(soundAsset);
		
		USoundWave* sound = soundAsset->sound;

		// Note: 2D sound doesn't need punAudio
		// RawData sound
		//UFireForgetAudioComponent* fireAndForget = nullptr;
		PUN_CHECK(!soundAsset->isRawData);
		//if (soundAsset->isRawData) {
			//fireAndForget = CreateRuntimeSound(groupName, soundAsset, false);
			//sound = fireAndForget->sound;
		//}

		// Note: 2D Sound use _worldContext which is gameInstance
		if (!IsValidPun(_worldContext)) {
			return;
		}
		UAudioComponent* audio = UGameplayStatics::SpawnSound2D(_worldContext, sound);
		if (!IsValidPun(audio)) {
			return;
		}
		audio->bAutoDestroy = true;

		// RawData sound
		//if (soundAsset->isRawData) {
		//	SetupRuntimeSound(fireAndForget, audio);
		//}

		float soundPropertyVolume = GetSoundPropertyRef(groupName, soundName, "Volume");
		float masterVolume = GetGroupPropertyRef(groupName, "UI_MasterVolume");

		audio->VolumeMultiplier = soundPropertyVolume * masterVolume * _playerSettings->masterVolume() * _playerSettings->soundEffectsVolume();
		audio->VolumeModulationMin = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMin");
		audio->VolumeModulationMax = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMax");

		audio->PitchMultiplier = GetSoundPropertyRef(groupName, soundName, "Pitch");
		audio->PitchModulationMin = GetSoundPropertyRef(groupName, soundName, "PitchModulationMin");
		audio->PitchModulationMax = GetSoundPropertyRef(groupName, soundName, "PitchModulationMax");


		audio->SetLowPassFilterEnabled(true);
		float lowPassMin = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMin");
		float lowPassMax = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMax");
		float lowPassFreq = soundRand.RandRange(lowPassMin, lowPassMax);
		audio->LowPassFilterFrequency = lowPassFreq;

		audio->Play();

		GetSoundPropertyRef(groupName, soundName, "PlayCount")++;
		GetSoundPropertyRef(groupName, soundName, "LastPlayed") = GetAudioTimeSeconds(this);
	}

private:
	void Adjust3DAttenuation(UAudioComponent* audio, std::string groupName, std::string soundName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		// Attenuation
		FSoundAttenuationSettings settings = audio->AttenuationOverrides;
		float tileRadius = GetSoundPropertyRef(groupName, soundName, "AttenuationRadius");
		float attenuationRadius = tileRadius * 10.0f;
		settings.AttenuationShapeExtents = FVector(attenuationRadius, 0.0f, 0.0f);
		settings.FalloffDistance = 10.0f * GetSoundPropertyRef(groupName, soundName, "AttenuationFalloffDistance");
		audio->AdjustAttenuation(settings);
		//settings.AbsorptionMethod = EAirAbsorptionMethod::Linear;
		//settings.LPFRadiusMin = tileRadius * 10.0f;
		//settings.LPFRadiusMax = tileRadius * 10.0f + settings.FalloffDistance;
		//settings.LPFFrequencyAtMin = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMax");
		//settings.LPFFrequencyAtMax = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMin");	

		// Low pass filter by distance
		float lowpassFilterRadius = 10.0f * GetSoundPropertyRef(groupName, soundName, "LowPassFilter_Radius");
		float lowpassFilterFalloffDistance = 10.0f * GetSoundPropertyRef(groupName, soundName, "LowPassFilter_FalloffDistance");
		float distance = FVector::Distance(audio->GetComponentLocation(), _dataSource->cameraLocation());
		float attenuationRatio = Clamp01((distance - lowpassFilterRadius) / lowpassFilterFalloffDistance); // 0 at low zoom

		float lowPassMin = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMin");
		float lowPassMax = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMax");
		float lowPassFreq = lowPassMin + (lowPassMax - lowPassMin) * (1.0f - attenuationRatio);

		audio->SetLowPassFilterFrequency(lowPassFreq);
	}

private:
	/*
	 * Music Prep
	 */
	
	void InitMusic()
	{
		PUN_LOG("InitMusic");
		
		std::string groupName = "Music";

		auto addMusic = [&](std::string soundName)
		{
			Add2DSoundProperties(groupName, soundName);
			_musicSoundNames.push_back(soundName);
			_musicDecompressedAssetCached.Add(nullptr);
			_musicDecompressedAssetInUse.Add(nullptr);
		};

		addMusic("Music_PositiveNonWinter");
		addMusic("Music_PositiveSpring");
		//addMusic("Music_PositiveSummer");
		addMusic("Music_PositiveWinter");

		addMusic("Music_NegativeNonWinter");
		addMusic("Music_NegativeWinter");

		//UpdateDecompressedMusicCache();

		AddHiddenProperty(groupName, "CurrentVolume", 1.0f);
		AddHiddenProperty(groupName, "RandomVolume", 1.0f);

		AddGroupProperty(groupName, "Group_MasterVolume", 1.0f);
		AddGroupProperty(groupName, "FadeInDuration", 3.0f);
		AddGroupProperty(groupName, "FadeOutDuration", 3.0f);
		AddGroupProperty(groupName, "RandomPauseDurationMin", 10.0f);
		AddGroupProperty(groupName, "RandomPauseDurationMax", 60.0f);

		// For FadeOut completely then fadeIn, just fadeOut, and create new sound with delay equal to fadeOutDuration
		
		float fadeOutDuration = GetGroupPropertyRef(groupName, "FadeOutDuration");
		FadeOutFadeInSound(groupName, "Music_PositiveNonWinter", fadeOutDuration, GetRandomPauseDuration(groupName), false);

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			std::string localSoundName = GetActiveSound(localGroupName);
			auto punAudio = GetAudioComponent(localGroupName);
			
			ss << "Track: " << localSoundName << "_" << punAudio->assetIndex;
			
			ss << std::fixed << std::showpoint << std::setprecision(1);
			float duration = punAudio->audio->Sound->GetDuration();
			ss << ", Duration: " << punAudio->playTime(this) << "/" << duration << ", queue:" << _groupNameToAudioStartTime.size();

			ss << std::setprecision(2);
			ss << ", Volume:";
			ss << "[FadeInVolume: " << GetHiddenPropertyRef(localGroupName, "CurrentVolume") << ", ";
			ss << "Random: " << GetHiddenPropertyRef(localGroupName, "RandomVolume") << ", ";
			ss << "Final: " << punAudio->audio->VolumeMultiplier << ", ";
			ss <<  (punAudio->audio->IsPlaying() ? "On" : "Off") << "]";

			return ss.str();
		};
		_debugDescriptionFuncs[groupName] = getDebugDescriptionFunc;
	}

	void UpdateMusic(IGameSimulationCore& simulation, int32 playerId)
	{
		// Only update every second for performance
		if (UGameplayStatics::GetAudioTimeSeconds(GetWorld()) > _lastUpdateTime + 1.0f)
		{
			_lastUpdateTime = UGameplayStatics::GetAudioTimeSeconds(GetWorld());
			
			auto punAudio = GetAudioComponent("Music");
			
			//PUN_LOG("Update Music: %s %s Num:%d", ToTChar(punAudio->groupName), ToTChar(punAudio->soundName), punAudios.Num());
			
			//UpdateDecompressedMusicCache();

			
			const auto& townIds = simulation.playerOwned(playerId).townIds();

			float freezingCount = 0;
			float starvingCount = 0;
			for (int32 townId : townIds) {
				const auto& adultIds = simulation.townManager(townId).adultIds();
				
				for (int32 id : adultIds) {
					UnitStateAI& unit = simulation.unitAI(id);
					if (unit.showNeedFood()) starvingCount++;
					if (unit.showNeedHeat()) freezingCount++;
				}
			}
			bool negative = starvingCount > 0 || freezingCount > 0;

			// Fade out the old sound if needed to play a new sound
			std::vector<std::string> preferredMusicPieces = GetPreferredMusic(negative);
			std::string activeSoundName = GetActiveSound("Music");

			
			auto getRandomMusic = [&](const std::string& randomPreferredMusic)
			{
				// Loop until we get music that isn't consecutive
				USoundAsset* soundAsset = nullptr;
				for (int32 i = 0; i < 100; i++) {
					soundAsset = GetRandomSoundAsset(randomPreferredMusic);
					if (soundAsset != _lastPlayedMusic) {
						break;
					}
				}
				
				PUN_LOG("getRandomMusic %s", ToTChar(soundAsset->soundName));
				_lastPlayedMusic = soundAsset;
				return soundAsset;
			};

			
			if (negative)
			{
				// Musical interruption for negative music
				// If activeSoundName not within preferredMusic, change to preferredMusic
				if (activeSoundName != "Music_NegativeNonWinter" &&
					activeSoundName != "Music_NegativeWinter")
				{
					std::string randomPreferredMusic = preferredMusicPieces[soundRand.Rand() % preferredMusicPieces.size()];
					float fadeOutDuration = GetGroupPropertyRef("Music", "FadeOutDuration");

					USoundAsset* soundAsset = getRandomMusic(randomPreferredMusic);

					// Note negative music begins right away without GetRandomPauseDuration(groupName)
					FadeOutFadeInSound("Music", randomPreferredMusic, fadeOutDuration, fadeOutDuration, false, soundAsset);
					return;
				}
			}

			//float volumeMultiplier = GetSoundPropertyRef("Music", activeSoundName, "Volume");
			//volumeMultiplier *= GetHiddenPropertyRef("Music", "RandomVolume");
			//volumeMultiplier *= GetGroupPropertyRef("Music", "Music_MasterVolume");
			//volumeMultiplier *= _playerSettings->masterVolume();
			//volumeMultiplier *= _playerSettings->musicVolume();
			//volumeMultiplier *= IsolateVolume("Music", activeSoundName);
			
			// Adjust volume according to fade
			UAudioComponent* audio = punAudio->audio;

			float volumeMultiplier = GetPunAudioVolumeWithFadeIn("Music", activeSoundName, punAudio->playTime(this), false);
			SetVolumeMultiplier(audio, volumeMultiplier);

			if (_forceUseEngineImport)
			{
				// Finished playing
				if (punAudio->isFinished(GetWorld()))
				{
					// inactive while we are switching to a new track, we shouldn't do anything
					bool isSwitching = false;
					for (size_t i = 0; i < _groupNameToAudioStartTime.size(); i++) {
						if (_groupNameToAudioStartTime[i].first == punAudio->groupName) {
							isSwitching = true;
							break;
						}
					}

					if (!isSwitching)
					{
						std::string randomPreferredMusic = preferredMusicPieces[soundRand.Rand() % preferredMusicPieces.size()];

						USoundAsset* asset = getRandomMusic(randomPreferredMusic);
						check(asset);

						punAudio->soundName = asset->soundName;
						punAudio->assetIndex = asset->assetIndex;
						audio->Sound = asset->sound;

						float pauseDuration = GetRandomPauseDuration("Music");
						float currentTime = UGameplayStatics::GetAudioTimeSeconds(this);
						float startTime = currentTime + pauseDuration;
						audio->Stop();
						punAudio->startTime = startTime; // negative playTime during pause
						
						_groupNameToAudioStartTime.push_back(std::make_pair(punAudio->groupName, startTime));

						PUN_LOG("Switch music audio null: %s %s Num:%d pause:%f", ToTChar(punAudio->groupName), ToTChar(punAudio->soundName), punAudios.Num(), pauseDuration);
					}
				}
			}
			else
			{

				checkNoEntry();
				
				///*
				// * RawData sound
				// */
				//auto soundProc = CastChecked<USoundWaveProcedural>(audio->Sound);
				//int32 byteCountLeft = soundProc->GetAvailableAudioByteCount();
				//if (byteCountLeft < 1000000)
				//{
				//	const uint64 audioComponentID = audio->GetAudioComponentID();
				//	auto audioDevice = audio->GetAudioDevice();
				//	auto uiInterface = _uiInterface;

				//	std::string randomPreferredMusic = preferredMusicPieces[soundRand.Rand() % preferredMusicPieces.size()];

				//	USoundAsset* asset = GetRandomSoundAsset(randomPreferredMusic);
				//	check(asset);

				//	float pauseDuration = GetRandomPauseDuration("Music");
				//	int32 bytePerSec = (asset != nullptr) ? (asset->waveInfo.samplesPerSec * asset->waveInfo.sizeOfSample) : 40000;
				//	int32 pauseByteCount = bytePerSec * pauseDuration;

				//	// Negative music starts right away without pause
				//	if (negative) {
				//		pauseByteCount = 0;
				//	}

				//	if (emptyFeed.Num() < pauseByteCount) {
				//		emptyFeed.SetNumZeroed(pauseByteCount, false);
				//	}

				//	if (asset->isPermanentUncompressed())
				//	{
				//	}
				//	else if (asset->bUncompressedDataReady.IsReady()) {
				//		MoveMusicAssetToInUse(asset->soundName);
				//	}
				//	else {
				//		// Add 20-sec delay if the music wasn't loaded
				//		pauseByteCount = std::fmax(pauseByteCount, bytePerSec * 20.0f);
				//	}


				//	FAudioThread::RunCommandOnAudioThread([audioDevice, audioComponentID, uiInterface, asset, pauseByteCount]()
				//	{
				//		FActiveSound* ActiveSound = audioDevice->FindActiveSound(audioComponentID);
				//		if (ActiveSound) {
				//			auto soundProc = CastChecked<USoundWaveProcedural>(ActiveSound->GetSound());

				//			if (pauseByteCount > 0) {
				//				soundProc->QueueAudio(emptyFeed.GetData(), pauseByteCount);
				//			}

				//			if (asset == nullptr) {
				//				// null asset, don't do anything
				//			}
				//			else if (asset->isPermanentUncompressed()) {
				//				PunAudioData audioData = asset->GetData();
				//				soundProc->QueueAudio(audioData.data, audioData.byteCount);
				//			}
				//			else if (asset->bUncompressedDataReady.IsReady()) {
				//				PunAudioData audioData = asset->GetData();
				//				soundProc->QueueAudio(audioData.data, audioData.byteCount);

				//				// After Queuing, we can despawn
				//				asset->bUsedUncompressedData = true;
				//			}

				//			//if (rawPCMPtr) {
				//			//	soundProc->QueueAudio(rawPCMPtr->GetData(), rawPCMPtr->Num());
				//			//}
				//		}
				//		//AsyncTask(ENamedThreads::GameThread, [&]() {
				//		//	uiInterface->PunLog("QueueMusic(Update)");
				//		//});
				//	});
				//}
				//// RawData sound ends
				//
			}
			

		}

		float playTime = UGameplayStatics::GetAudioTimeSeconds(this) - GetPlayStartTime("Music");
		float fadeInVolume = Clamp01(playTime / GetGroupPropertyRef("Music", "FadeInDuration"));
		fadeInVolume += MinVolume; // Need MinVolume to make sure it doesn't just stop the track...

		GetHiddenPropertyRef("Music", "CurrentVolume") = fadeInVolume;
	}

	std::vector<std::string> GetPreferredMusic(bool negative)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		std::string soundName = GetManualOverrideSound("Music");
		if (soundName != "") {
			return { soundName };
		}

		if (negative)
		{
			if (Time::IsSpring() || Time::IsSummer() || Time::IsAutumn()) return { "Music_NegativeNonWinter" };
			if (Time::IsWinter()) return { "Music_NegativeWinter" };
		}
		else
		{
			if (Time::IsSpring()) return { "Music_PositiveSpring", "Music_PositiveNonWinter" };
			if (Time::IsSummer()) return { "Music_PositiveNonWinter" };
			if (Time::IsAutumn()) return { "Music_PositiveNonWinter" };
			if (Time::IsWinter()) return { "Music_PositiveWinter" };
		}
		UE_DEBUG_BREAK();
		return {};
	}

	// Engine import
	UPROPERTY() USoundAsset* _lastPlayedMusic = nullptr;

	// Next music to be played
	std::vector<std::string> _musicSoundNames;
	UPROPERTY() TArray<USoundAsset*> _musicDecompressedAssetCached;
	UPROPERTY() TArray<USoundAsset*> _musicDecompressedAssetInUse; // Asset is in this state while waiting for audio thread to copy the music into buffer

	UPROPERTY() USoundAsset* _musicBeingDecompressed = nullptr;
	
	USoundAsset* GetRandomSoundAsset(std::string soundName)
	{
		// Use Engine Import
		if (_forceUseEngineImport)
		{
			TArray<USoundAsset*>& soundArray = GetSoundAssets(soundName)->assets;
			PUN_CHECK(soundArray.Num() > 0);
			return soundArray[soundRand.Rand() % soundArray.Num()];
		}

		checkNoEntry();
		return nullptr;
		
		//// Music's random sound is actually not truly random
		//// We generate a shuffled queue of what to play, and continuously decompress the top most music on the queue
		//if (soundName.substr(0, 5) == "Music") 
		//{
		//	// Find and return the cached asset
		//	for (int32 i = 0; i < _musicSoundNames.size(); i++) {
		//		if (_musicSoundNames[i] == soundName) {
		//			if (_musicDecompressedAssetCached[i]) {
		//				return _musicDecompressedAssetCached[i];
		//			} 
		//			return _musicBeingDecompressed;
		//		}
		//	}
		//	UE_DEBUG_BREAK();
		//	return _musicBeingDecompressed;
		//}

		//TArray<USoundAsset*>& soundArray = GetSoundAssets(soundName)->assets;
		//PUN_CHECK(soundArray.Num() > 0);
		//return soundArray[soundRand.Rand() % soundArray.Num()];
	}

	//void MoveMusicAssetToInUse(std::string soundName)
	//{
	//	for (int32 i = 0; i < _musicSoundNames.size(); i++) {
	//		if (_musicSoundNames[i] == soundName) {
	//			_musicDecompressedAssetInUse[i] = _musicDecompressedAssetCached[i];
	//			_musicDecompressedAssetCached[i] = nullptr;

	//			//UpdateDecompressedMusicCache(); // refill the music cache rightaway
	//			return;
	//		}
	//	}
	//	UE_DEBUG_BREAK();
	//}
	

	/*
	 * Decompressed Data Pool
	 *  Pool is needed to prevent memory alloc/copy
	 *  We need two pools for 10mb sized negative music and 50mb positive music
	 */
	UPROPERTY() TArray<UPoolArray*> _largeMusicDecompressedDataPool;
	UPROPERTY() TArray<UPoolArray*> _smallMusicDecompressedDataPool;
	
	static UPoolArray* SpawnMusicData(TArray<UPoolArray*>& pool) {
		for (int32 i = 0; i < pool.Num(); i++) {
			if (!pool[i]->bInUse) {
				pool[i]->bInUse = true;
				return pool[i];
			}
		}
		pool.Add(NewObject<UPoolArray>());
		return pool.Last();
	}
	static void DespawnMusicData(UPoolArray* poolArray) {
		poolArray->data.SetNumZeroed(false);
		poolArray->bInUse = false;
	}

	//void UpdateDecompressedMusicCache()
	//{
	//	if (_forceUseEngineImport) {
	//		return;
	//	}
	//	
	//	for (int32 i = 0; i < _musicSoundNames.size(); i++) 
	//	{
	//		// This ensures we only decompress 1 music at a time
	//		if (_musicBeingDecompressed && 
	//			_musicBeingDecompressed->bUncompressedDataReady.IsReady())
	//		{
	//			_musicBeingDecompressed = nullptr;
	//		}
	//		
	//		// Refill the cache if needed
	//		if (_musicBeingDecompressed == nullptr &&
	//			_musicDecompressedAssetCached[i] == nullptr) 
	//		{
	//			//_musicDecompressedAssetInUse[i]; // TODO: don't play same music consecutively

	//			// Add a random asset to the cache
	//			TArray<USoundAsset*>& soundArray = GetSoundAssets(_musicSoundNames[i])->assets;
	//			PUN_CHECK(soundArray.Num() > 0);
	//			USoundAsset* asset = soundArray[soundRand.Rand() % soundArray.Num()];

	//			// Uncompress the compressedData to prepare it for use
	//			PunWaveModInfo waveModInfo = asset->waveInfo;
	//			UPoolArray* compressedData = asset->compressedData;

	//			if (_musicSoundNames[i] == "Music_NegativeWinter" ||
	//				_musicSoundNames[i] == "Music_NegativeNonWinter") 
	//			{
	//				asset->data = SpawnMusicData(_smallMusicDecompressedDataPool);
	//			} else {
	//				asset->data = SpawnMusicData(_largeMusicDecompressedDataPool);
	//			}

	//			UPoolArray* data = asset->data;
	//			asset->bUncompressedDataReady = Async(EAsyncExecution::Thread, [waveModInfo, compressedData, data]() {
	//				return DecompressOGG(waveModInfo, compressedData->data, data->data);
	//			});
	//			_musicBeingDecompressed = asset;

	//			
	//			_musicDecompressedAssetCached[i] = asset;
	//		}

	//		// Reset and remove _musicDecompressedAssetInUse if it was already used
	//		if (_musicDecompressedAssetInUse[i] &&
	//			_musicDecompressedAssetInUse[i]->bUsedUncompressedData) 
	//		{
	//			USoundAsset* asset = _musicDecompressedAssetInUse[i];
	//			asset->bUncompressedDataReady.Reset();
	//			asset->bUsedUncompressedData = false;
	//			DespawnMusicData(asset->data);
	//			asset->data = nullptr;
	//		}
	//	}
	//}

private:
	/*
	 * Ambient/Music Prep
	 */
	UPROPERTY() TMap<FString, UFireForgetAudioComponent*> GroupNameToAudioComponent;
	std::unordered_map<std::string, std::string> _groupNameToActiveSoundName;

	std::vector<std::pair<std::string, float>> _groupNameToAudioStartTime; // For delayed audio start

	UFireForgetAudioComponent* GetAudioComponent(std::string groupName) {
		if (GroupNameToAudioComponent.Contains(ToFString(groupName))) {
			return GroupNameToAudioComponent[ToFString(groupName)];
		}
		return nullptr;
	}

	float GetRandomPauseDuration(std::string groupName)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		float pauseDurationMin = GetGroupPropertyRef(groupName, "RandomPauseDurationMin");
		float pauseDurationMax = GetGroupPropertyRef(groupName, "RandomPauseDurationMax");
		return pauseDurationMin + (pauseDurationMax - pauseDurationMin) * (soundRand.Rand() % 1000) / 1000.0f;
	}

	std::string GetActiveSound(std::string groupName) {
		return GroupNameToAudioComponent[ToFString(groupName)]->soundName;
	}

	/*
	 * CrossFade
	 *  - keep currentSoundName for 1) QueueAudio looping, 2) Check if we should switch to other sound...
	 *  - Swap the currentSound to a list of audios that will be cross faded away...
	 *  - Spawn a new sound and fade it in...
	 */
	void CrossFadeInSound(std::string groupName, std::string soundName)
	{
		PUN_LOG("CrossFadeInSound %s %s", ToTChar(groupName), ToTChar(soundName));
		
		FadeOutFadeInSound(groupName, soundName, GetGroupPropertyRef(groupName, "CrossFadeDuration"), GetRandomPauseDuration(groupName));
	}
	
	void FadeOutFadeInSound(std::string groupName, std::string soundName, float fadeOutDuration, float startDelay, bool isLooping = true, USoundAsset* soundAsset = nullptr)
	{
		LLM_SCOPE_(EPunSimLLMTag::PUN_Sound);
		
		PUN_LOG("FadeOutFadeInSound %s %s fadeOut:%f startDelay:%f looping:%d", ToTChar(groupName), ToTChar(soundName), fadeOutDuration, startDelay, isLooping);

		if (soundAsset == nullptr) {
			soundAsset = GetRandomSoundAsset(soundName);
		}
		//USoundAsset* soundAsset = GetRandomSoundAsset(soundName);
		PUN_CHECK(soundAsset);
		
		// Fade out old sound
		UFireForgetAudioComponent* oldPunAudio = GetAudioComponent(groupName);
		if (oldPunAudio) {
			float time = UGameplayStatics::GetAudioTimeSeconds(this);
			oldPunAudio->fadeStartTime = time;
			oldPunAudio->fadeEndTime = time + fadeOutDuration;
			oldPunAudio->isFadingOut = true;
			oldPunAudio->isLooping = false; // Ensures it gets terminated

			PUN_LOG("--- %s : fadeout old: start:%f end:%f", ToTChar(groupName), oldPunAudio->fadeStartTime, oldPunAudio->fadeEndTime);
		}


		USoundWave* sound = soundAsset->sound;
		
		// RawData sound
		PUN_CHECK(!soundAsset->isRawData);
		UFireForgetAudioComponent* punAudio = nullptr;
		if (soundAsset->isRawData) {
			//punAudio = CreateRuntimeSound(groupName, soundAsset, isLooping);
			//sound = punAudio->sound;
		} else {
			punAudio = CreateRuntimeSound_EngineImport(groupName, soundAsset, isLooping);
		}


		// Create Audio
		UAudioComponent* audio = UGameplayStatics::CreateSound2D(_worldContext, sound); // Use Create instead of Spawn to prevent play() right away
		audio->bAutoDestroy = false;
		audio->bAlwaysPlay = true;
		audio->bIsMusic = true;

		// RawData sound
		if (soundAsset->isRawData) {
			//SetupRuntimeSound(punAudio, audio);
		} else {
			AddPunAudio(punAudio, audio);
		}

		
		// Replace sound
		if (GroupNameToAudioComponent.Contains(ToFString(groupName))) {
			GroupNameToAudioComponent[ToFString(groupName)] = punAudio;
		} else {
			GroupNameToAudioComponent.Add(ToFString(groupName), punAudio);
		}

		// Random values for Volume
		float volumeMin = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMin");
		float volumeMax = GetSoundPropertyRef(groupName, soundName, "VolumeModulationMax");
		float& randomVolume = GetHiddenPropertyRef(groupName, "RandomVolume");
		randomVolume = volumeMin + (volumeMax - volumeMin) * soundRand.Rand01();

		audio->VolumeMultiplier = randomVolume + MinVolume; // prevent shutdown..

		audio->PitchMultiplier = GetSoundPropertyRef(groupName, soundName, "Pitch");
		audio->PitchModulationMin = GetSoundPropertyRef(groupName, soundName, "PitchModulationMin");
		audio->PitchModulationMax = GetSoundPropertyRef(groupName, soundName, "PitchModulationMax");

		// Low pass filter
		audio->bEnableLowPassFilter = true;
		float lowPassMin = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMin");
		float lowPassMax = GetSoundPropertyRef(groupName, soundName, "LowPassFilterMax");
		audio->LowPassFilterFrequency = soundRand.RandRange(lowPassMin, lowPassMax);

		// TODO: since start delay doesn't work, do a manual delay
		float currentTime = UGameplayStatics::GetAudioTimeSeconds(this);


		if (startDelay > 0.0f) {
			PUN_LOG("-- Delayed start %s %s delay:%f", ToTChar(groupName), ToTChar(soundName), startDelay);
			audio->SetPaused(true);
			_groupNameToAudioStartTime.push_back(std::make_pair(groupName, currentTime + startDelay));
		} else {
			//PUN_LOG("Audio->Play %s %s", ToTChar(groupName), ToTChar(soundName));
			audio->Play();
		}

		//_groupNameToActiveSoundName[groupName] = soundName;
		_groupNameToPlayStartTime[groupName] = currentTime + startDelay;

		// Don't increment PlayCount for permanent sound unless we determine it was played for at least 1/2 the duration
		float& lastPlayed = GetSoundPropertyRef(groupName, soundName, "LastPlayed");
		//float duration = audio->Sound->GetDuration();
		//if (currentTime > lastPlayed + duration / 2.0f) {
			GetSoundPropertyRef(groupName, soundName, "PlayCount")++;
		//}
		lastPlayed = currentTime;

	}


private:
	/*
	 * Wind Seasonal
	 */

	void InitWindSeasonal()
	{
		std::string groupName = "WindSeasonal";

		for (const BiomeInfo& info : BiomeInfos)
		{
			string biomeName = info.GetDisplayNameWithoutSpace();
			Add2DSoundProperties(groupName, "WindSpring_" + biomeName);
			Add2DSoundProperties(groupName, "WindSummer_" + biomeName);
			Add2DSoundProperties(groupName, "WindAutumn_" + biomeName);
			Add2DSoundProperties(groupName, "WindWinter_" + biomeName);
		}
		
		InitAmbientSoundHelper(groupName, "WindSpring_Forest", [&]()
		{
			//return "WindSpring_" + GetBiomeInfo(BiomeEnum::Forest).GetNameWithoutSpace();
			
			BiomeEnum biomeEnum = _dataSource->simulation().GetBiomeEnum(_dataSource->cameraAtom().worldTile2());
			string biomeName = GetBiomeInfo(biomeEnum).GetDisplayNameWithoutSpace();
			
			if (Time::IsSpring()) return "WindSpring_" + biomeName;
			if (Time::IsSummer()) return "WindSummer_" + biomeName;
			if (Time::IsAutumn()) return "WindAutumn_" + biomeName;
			if (Time::IsWinter()) return "WindWinter_" + biomeName;

			UE_DEBUG_BREAK();
			return std::string();
		});

		GetGroupPropertyRef(groupName, "Group_MasterVolume") = 15.0f;
	}

private:
	/*
	 * Ocean Ambience
	 * Wind Altitude
	 * Rain
	 */
	void InitOtherAmbiences()
	{
		//! Ocean
		InitAmbientSoundHelper("OceanGroup", "Ocean");
		AddGroupProperty("OceanGroup", "MinVolume_Altitude", .058);
		AddGroupProperty("OceanGroup", "MaxVolume_Altitude", .025);
		AddGroupProperty("OceanGroup", "DistanceSoundStart", 75);
		AddGroupProperty("OceanGroup", "DistanceSoundFull", 40);

		InitTargetVolume("OceanGroup", [&]() {
			float nearestSeaDistance = _dataSource->NearestSeaDistance();
			// nearestSeaDistance = 0, fraction = 1
			// nearestSeaDistance = soundFull, fraction = 1
			// nearestSeaDistance = soundStart, fraction = 0
			float distanceSoundStart = GetGroupPropertyRef("OceanGroup", "DistanceSoundStart");
			float distanceSoundFull = GetGroupPropertyRef("OceanGroup", "DistanceSoundFull");
			float ambienceOceanGroundDistanceFactor = (distanceSoundStart - nearestSeaDistance) / (distanceSoundStart - distanceSoundFull);

			float HeightStart = GetGroupPropertyRef("OceanGroup", "MinVolume_Altitude");
			float fullHeight = GetGroupPropertyRef("OceanGroup", "MaxVolume_Altitude");
			float ambienceOceanHeightFactor = (HeightStart - zoomHeightFraction()) / (HeightStart - fullHeight);

			return Clamp01(ambienceOceanHeightFactor) * Clamp01(ambienceOceanGroundDistanceFactor);
		});

		//! Wind Altitude
		InitAmbientSoundHelper("WindAltitudeGroup", "WindAltitude");
		AddGroupProperty("WindAltitudeGroup", "MinVolume_Altitude", .058);
		AddGroupProperty("WindAltitudeGroup", "MaxVolume_Altitude", .120);
		InitTargetVolume("WindAltitudeGroup", [&]() {
			float HeightStart = GetGroupPropertyRef("WindAltitudeGroup", "MinVolume_Altitude");
			float fullHeight = GetGroupPropertyRef("WindAltitudeGroup", "MaxVolume_Altitude");

			float windAltitudeVolume = (zoomHeightFraction() - HeightStart) / (fullHeight - HeightStart);
			return Clamp01(windAltitudeVolume);
		});

		//! Rain
		InitAmbientSoundHelper("RainGroup", "Rain");
		AddGroupProperty("RainGroup", "MinVolume_Altitude", .2);
		AddGroupProperty("RainGroup", "MaxVolume_Altitude", .025);
		AddGroupProperty("RainGroup", "HighAltitudeMinimumSoundFraction", .35);
		InitTargetVolume("RainGroup", [&]() {
			if (!Time::IsRaining()) {
				return 0.0f;
			}
			float HeightStart = GetGroupPropertyRef("RainGroup", "MinVolume_Altitude");
			float fullHeight = GetGroupPropertyRef("RainGroup", "MaxVolume_Altitude");
			float minSoundRatio = GetGroupPropertyRef("RainGroup", "HighAltitudeMinimumSoundFraction");
			float ambienceOceanHeightFactor = (HeightStart - zoomHeightFraction()) / (HeightStart - fullHeight);

			return Clamp01(ambienceOceanHeightFactor * (1.0f - minSoundRatio) + minSoundRatio);
		});
	}

private:
	/*
	 * Bird
	 */
	void InitBirdTweets()
	{
		AddGroupProperty("BirdTweets", "World_MasterVolume", 1.0f);

		AddGroupProperty("BirdTweets", "TweetIntervalMin_Summer", 0.0f);
		AddGroupProperty("BirdTweets", "TweetIntervalMax_Summer", 0.5f);
		AddGroupProperty("BirdTweets", "TweetIntervalMin_Winter", 0.0f);
		AddGroupProperty("BirdTweets", "TweetIntervalMax_Winter", 3.0f);
		AddGroupProperty("BirdTweets", "SummerTweetIntervalPower", 5.0f);

		AddGroupProperty("BirdTweets", "SummerBirdPower", 5.0f);


		// Special radius
		for (std::string biomeExtension : TreeBiomeExtensions)
		{
			Add3DSoundProperties("BirdTweets", "BirdTweetSummer" + biomeExtension);
			Add3DSoundProperties("BirdTweets", "BirdTweetWinter" + biomeExtension);
			GetSoundPropertyRef("BirdTweets", "BirdTweetSummer" + biomeExtension, "AttenuationFalloffDistance") = 300.0f;
			GetSoundPropertyRef("BirdTweets", "BirdTweetWinter" + biomeExtension, "AttenuationFalloffDistance") = 300.0f;
		}
		for (std::string biomeExtension : NonTreeBiomeExtensions)
		{
			Add3DSoundProperties("BirdTweets", "NonTreeBirdSound" + biomeExtension);
			GetSoundPropertyRef("BirdTweets", "NonTreeBirdSound" + biomeExtension, "AttenuationFalloffDistance") = 300.0f;
			GetSoundPropertyRef("BirdTweets", "NonTreeBirdSound" + biomeExtension, "PlayProbability") = 0.1f;
		}
	

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			ss << std::fixed << std::showpoint << std::setprecision(1);
			ss << "NextTweetTry: " << _nextBirdTryTweetTime;
			ss << ", TweetInterval: " << _currentTweetInterval;
			ss << std::setprecision(2);
			ss << ", SummerBirdTweetIntervalWeight: " << 1.0f - winterBirdTweetIntervalWeight();
			ss << ", SummerBirdProbability: " << 1.0f - winterBirdTweetProbability();
			return ss.str();
		};
		_debugDescriptionFuncs["BirdTweets"] = getDebugDescriptionFunc;
	}

	void UpdateBirdTweets()
	{
		// Try to tweet after some interval
		float time = UGameplayStatics::GetAudioTimeSeconds(this);
		if (time >= _nextBirdTryTweetTime)
		{
			// Random Sample trees in the area where birds could be
			std::vector<int32> regionIds = _dataSource->sampleRegionIds();
			if (regionIds.size() > 0)
			{
				// Sample one random region from sampleIds
				int32 regionId = regionIds[soundRand.Rand() % regionIds.size()];
				WorldRegion2 region(regionId);

				// Get tree tiles within the region
				std::vector<int32> treeTileIds;
				region.ExecuteOnRegion_Tile([&](int16 x, int16 y) {
					int32 tileId = WorldTile2(x, y).tileId();
					TileObjInfo tileInfo = _dataSource->simulation().treeSystem().tileInfo(tileId);
					if (tileInfo.type == ResourceTileType::Tree) {
						treeTileIds.push_back(tileId);
					}
				});

				// Use sample success probability to determine if a bird should tweet
				int32 maximumTreeCount = CoordinateConstants::TileIdsPerRegion / 4;
				bool shouldTweet = (soundRand.Rand() % maximumTreeCount) < treeTileIds.size();
				BiomeEnum biomeEnum = _dataSource->simulation().terrainGenerator().GetBiome(region);
				if (shouldTweet)
				{
					std::string biomeExtension;
					switch (biomeEnum) {
						case BiomeEnum::BorealForest: biomeExtension = "_BorealForest"; break;
						case BiomeEnum::Forest: biomeExtension = "_Forest"; break;
						case BiomeEnum::Jungle: biomeExtension = "_Jungle"; break;
						default: biomeExtension = "_Forest"; break;
					}
					
					// Spawn the bird tweet at a random successful sample spot
					int32 tileToSpawn = treeTileIds[soundRand.Rand() % treeTileIds.size()];
					std::string tweetSound = soundRand.Rand01() < winterBirdTweetProbability() ? ("BirdTweetWinter" + biomeExtension) : ("BirdTweetSummer" + biomeExtension);

					Spawn3DSound("BirdTweets", tweetSound, WorldTile2(tileToSpawn).worldAtom2(), 30.0f, true);
				}
				else
				{
					std::string biomeExtension;
					switch (biomeEnum) {
					case BiomeEnum::Desert: biomeExtension = "_Desert"; break;
					case BiomeEnum::Savanna: biomeExtension = "_Savanna"; break;
					case BiomeEnum::Tundra: biomeExtension = "_Tundra"; break;
					default: biomeExtension = ""; break;
					}
					
					// Try for desert...
					if (biomeExtension != "")
					{
						WorldTile2 randomTile(soundRand.Rand() % CoordinateConstants::TilesPerRegion + region.minXTile(),
							soundRand.Rand() % CoordinateConstants::TilesPerRegion + region.minYTile());
						Spawn3DSound("BirdTweets", "NonTreeBirdSound" + biomeExtension, randomTile.worldAtom2(), 3.0f, true);
					}
				}

				// Randomize interval for next tweet
				float winterWeight = winterBirdTweetIntervalWeight();

				float intervalMin_Summer = GetGroupPropertyRef("BirdTweets", "TweetIntervalMin_Summer");
				float intervalMin_Winter = GetGroupPropertyRef("BirdTweets", "TweetIntervalMin_Winter");
				float intervalMin = intervalMin_Summer * (1.0f - winterWeight) + intervalMin_Winter * winterWeight;

				float intervalMax_Summer = GetGroupPropertyRef("BirdTweets", "TweetIntervalMax_Summer");
				float intervalMax_Winter = GetGroupPropertyRef("BirdTweets", "TweetIntervalMax_Winter");
				float intervalMax = intervalMax_Summer * (1.0f - winterWeight) + intervalMax_Winter * winterWeight;

				_currentTweetInterval = intervalMin + (intervalMax - intervalMin) * (soundRand.Rand() % 1000) / 1000.0f;
				_nextBirdTryTweetTime = time + _currentTweetInterval;
			}
		}
	}

	float winterBirdTweetIntervalWeight() {
		return winterWeight01(GetGroupPropertyRef("BirdTweets", "SummerTweetIntervalPower"));
	}

	float winterBirdTweetProbability() {
		return winterWeight01(GetGroupPropertyRef("BirdTweets", "SummerBirdPower"));
	}

	float winterWeight01(float summerPower) {
		int32 ticksThisYear = Time::Ticks() % Time::TicksPerYear;
		float earlySpringShift = 0.05;
		float sinCurve = sin((static_cast<float>(ticksThisYear) / Time::TicksPerYear - 0.625 + earlySpringShift) * 2.0f * PI) * 0.5f + 0.5f;
		float exponent = summerPower;
		return std::pow(sinCurve, exponent);
	}

	float _nextBirdTryTweetTime = 0.0f;
	float _currentTweetInterval = 0.0f;

private:
	/*
	 * CitizenAction
	 */

	void InitTreeStoneCrop()
	{
		Add3DAudioGroupProperties("CitizenAction");

		Add3DSoundProperties("CitizenAction", "TreeChopping");
		Add3DSoundProperties("CitizenAction", "TreeFalling");

		Add3DSoundProperties("CitizenAction", "CropPlanting");
		Add3DSoundProperties("CitizenAction", "CropHarvesting");

		Add3DSoundProperties("CitizenAction", "StonePicking");

		Add3DSoundProperties("CitizenAction", "RoadConstruction");
		Add3DSoundProperties("CitizenAction", "WoodConstruction");
		Add3DSoundProperties("CitizenAction", "ConstructionComplete");
		Add3DSoundProperties("CitizenAction", "ConstructionCompleteRoad");

		Add3DSoundProperties("CitizenAction", "BowImpactDirt");
		Add3DSoundProperties("CitizenAction", "BowImpactFlesh");
		Add3DSoundProperties("CitizenAction", "BowShoot");

		//! Military
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Archer");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Artillery");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Battleship");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Cannon");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Catapult");

		Add3DSoundProperties("CitizenAction", "Battle_Attack_Conscript");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Frigate");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Galley");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Infantry");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_KnightArab");

		Add3DSoundProperties("CitizenAction", "Battle_Attack_Knight");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_MachineGun");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_MilitiaArab");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Militia");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Musketeer");

		Add3DSoundProperties("CitizenAction", "Battle_Attack_NationalGuard");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Swordman");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Tank");
		Add3DSoundProperties("CitizenAction", "Battle_Attack_Warrior");

		Add3DSoundProperties("CitizenAction", "Battle_Loss_Battleship");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Battleship_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Frigate");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Frigate_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Galley");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Galley_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Human");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Human_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Metal");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Metal_P");

		Add3DSoundProperties("CitizenAction", "Battle_Loss_StoneBuilding");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_StoneBuilding_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Treasure");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Treasure_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_WoodBuilding");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_WoodBuilding_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Wood");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Wood_P");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Metal");
		Add3DSoundProperties("CitizenAction", "Battle_Loss_Metal_P");

		Add3DSoundProperties("CitizenAction", "BattleBegin");
		

		//! Buildings
		Add3DAudioGroupProperties("Building");
		AddBuildingSoundProperties("Building", "BuildingBrewery");
		AddBuildingSoundProperties("Building", "BuildingCharcoalMaker");
		//AddBuildingSoundProperties("Building", "BuildingFurnitureMaker");
		AddBuildingSoundProperties("Building", "BuildingSmelter");
		AddBuildingSoundProperties("Building", "BuildingQuarry");
		AddBuildingSoundProperties("Building", "BuildingCoalMine");


		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			return ss.str();
		};
		_debugDescriptionFuncs["CitizenAction"] = getDebugDescriptionFunc;
	}

	void AddBuildingSoundProperties(std::string groupName, std::string soundName) {
		Add3DSoundProperties(groupName, soundName);

		if (HasSoundAssets(soundName + "_OneShot"))
		{
			Add3DSoundProperties(groupName, soundName + "_OneShot");
			AddSoundProperty(groupName, soundName + "_OneShot", "MinInterval", 2.0f);
			AddSoundProperty(groupName, soundName + "_OneShot", "MaxInterval", 5.0f);
		}
	}

	void InitResourceDropoffPickup()
	{
		Add3DAudioGroupProperties("ResourceDropoffPickup");

		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Coal");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Cloth");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Wood");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Stone");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Leather");

		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Crops");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Meat");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Metal");
		Add3DSoundProperties("ResourceDropoffPickup", "Dropoff_Ore");

		Add3DSoundProperties("ResourceDropoffPickup", "Pickup");

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			return ss.str();
		};
		_debugDescriptionFuncs["ResourceDropoffPickup"] = getDebugDescriptionFunc;
	}

	static std::string GetResourceDropoffSound(ResourceEnum resourceEnum)
	{
		if (IsMeatEnum(resourceEnum)) {
			return "Dropoff_Meat";
		}

		if (IsFoodEnum(resourceEnum)) {
			return "Dropoff_Crops";
		}

		if (IsMetalEnum(resourceEnum)) {
			return "Dropoff_Metal";
		}

		if (resourceEnum == ResourceEnum::Coal) {
			return "Dropoff_Coal";
		}

		if (IsOreEnum(resourceEnum)) {
			return "Dropoff_Ore";
		}

		switch (resourceEnum)
		{
		case ResourceEnum::Cloth: return "Dropoff_Cloth";
		case ResourceEnum::Wood: return "Dropoff_Wood";
		case ResourceEnum::Stone: return "Dropoff_Stone";
		case ResourceEnum::Leather: return "Dropoff_Leather";
		default:
			return "Dropoff_Crops";
		}
	}

	static std::string GetBuildingWorkSound(CardEnum buildingEnum)
	{
		switch (buildingEnum)
		{
		case CardEnum::CharcoalMaker: return "BuildingCharcoalMaker";
		case CardEnum::BeerBrewery: return "BuildingBrewery";
		case CardEnum::VodkaDistillery: return "BuildingBrewery";
		case CardEnum::TequilaDistillery: return "BuildingBrewery";
		//case CardEnum::FurnitureWorkshop: return "BuildingFurnitureMaker";
			
		case CardEnum::IronSmelter:
		case CardEnum::GoldSmelter:
			return "BuildingSmelter";
			
		case CardEnum::CoalMine:
		case CardEnum::IronMine:
			return "BuildingCoalMine";
			
		case CardEnum::Quarry:
			return "BuildingQuarry";
		default:
			return "";
		}
	}

public:
	void SpawnResourceDropoffAudio(ResourceEnum resourceEnum, WorldAtom2 worldAtom) {
		std::string dropoffSound = GetResourceDropoffSound(resourceEnum);

		Spawn3DSound("ResourceDropoffPickup", dropoffSound, worldAtom, 0.3f);
	}

	void TryStartBuildingWorkSound(CardEnum buildingEnum, int32 buildingId, WorldAtom2 worldAtom)
	{
		std::string buildingSound = GetBuildingWorkSound(buildingEnum);
		if (buildingSound != "")
		{
			int32 regionId = worldAtom.worldTile2().regionId();

			auto punAudio = Spawn3DSound("Building", buildingSound, worldAtom, 3.0f);
			if (punAudio)
			{
				punAudio->isBuildingSound = true;
				punAudio->buildingId = buildingId;
				punAudio->regionId = regionId;

				punAudio->isLooping = true;
				punAudio->hasOneShotSound = HasSoundAssets(punAudio->soundName + "_OneShot");
			}
			
			//std::unordered_map<int32, UFireForgetAudioComponent*>& buildingIdToSound = _regionIdToBuildingIdToSound[regionId];

			//// Just play the sound if no lastSoundTime
			//// Note: SHIPPING BEWARE... buildingIdToSound[buildingId] turned null after swapping Alistair UI to "TreeFalling" then back to "Building"
			//auto found = buildingIdToSound.find(buildingId);
			//if (found == buildingIdToSound.end() || 
			//	buildingIdToSound[buildingId] == nullptr) 
			//{
			//	buildingIdToSound[buildingId] = Spawn3DSound("Building", buildingSound, worldAtom, 3.0f);

			//	if (buildingIdToSound[buildingId])
			//	{
			//		buildingIdToSound[buildingId]->isBuildingSound = true;
			//		buildingIdToSound[buildingId]->buildingId = buildingId;
			//		buildingIdToSound[buildingId]->regionId = regionId;

			//		buildingIdToSound[buildingId]->isLooping = true;
			//		buildingIdToSound[buildingId]->hasOneShotSound = HasSoundAssets(buildingIdToSound[buildingId]->soundName + "_OneShot");
			//	}
			//}
		}
	}
	void TryStopBuildingWorkSound(CardEnum buildingEnum, int32 buildingId, WorldAtom2 worldAtom)
	{
		if (GetBuildingWorkSound(buildingEnum) == "") {
			return;
		}

		for (int32 i = punAudios.Num(); i-- > 0;) {
			if (punAudios[i]->buildingId == buildingId) {
				RemovePunAudio(i);
			}
		}
		
		//std::unordered_map<int32, UFireForgetAudioComponent*>& buildingIdToSound = _regionIdToBuildingIdToSound[worldAtom.worldTile2().regionId()];

		//// Just play the sound if no lastSoundTime
		//auto found = buildingIdToSound.find(buildingId);
		//if (found != buildingIdToSound.end()) {
		//	if (found->second) {
		//		found->second->isDone = true;
		//	}
		//	buildingIdToSound.erase(buildingId);
		//}
	}

	//std::unordered_map<int32, std::unordered_map<int32, UFireForgetAudioComponent*>> _regionIdToBuildingIdToSound;

private:
	/*
	 * Doors
	 */

	 // Note: For now, just spawn door sound when arriving/going out of workplace...
	void InitDoors()
	{
		Add3DAudioGroupProperties("Doors");

		Add3DSoundProperties("Doors", "DoorOpen");
		Add3DSoundProperties("Doors", "DoorClose");

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			return ss.str();
		};
		_debugDescriptionFuncs["Doors"] = getDebugDescriptionFunc;
	}

private:
	/*
	 * UI
	 */

	void InitUI()
	{
		AddGroupProperty("UI", "UI_MasterVolume", 1.0f);

		Add2DSoundProperties("UI", "UIWindowOpen");
		Add2DSoundProperties("UI", "UIWindowClose");

		Add2DSoundProperties("UI", "DropdownChange");

		Add2DSoundProperties("UI", "ResearchInitiated");
		Add2DSoundProperties("UI", "ResearchComplete");
		Add2DSoundProperties("UI", "ResearchCompleteNewEra");

		Add2DSoundProperties("UI", "Combo");
		Add2DSoundProperties("UI", "ChooseRareCard");

		Add2DSoundProperties("UI", "UpgradeHouse");
		Add2DSoundProperties("UI", "UpgradeBuilding");
		Add2DSoundProperties("UI", "UpgradeTownhall");

		Add2DSoundProperties("UI", "PopupNeutral");
		Add2DSoundProperties("UI", "PopupBad");
		Add2DSoundProperties("UI", "PopupCannot");

		//Add2DSoundProperties("UI", "QuestNew");
		Add2DSoundProperties("UI", "QuestComplete");

		for (int32 i = 0; i <= 5; i++ ) {
			Add2DSoundProperties("UI", "RoundCountdown" + std::to_string(i));
		}

		Add2DSoundProperties("UI", "PlacementDrag");
		Add2DSoundProperties("UI", "PlaceBuilding");
		Add2DSoundProperties("UI", "CancelPlacement");

		Add2DSoundProperties("UI", "CardDeal");
		Add2DSoundProperties("UI", "CardHover");
		Add2DSoundProperties("UI", "CardClick");

		Add2DSoundProperties("UI", "ButtonHover");
		Add2DSoundProperties("UI", "ButtonClick");
		Add2DSoundProperties("UI", "ButtonClick2");
		Add2DSoundProperties("UI", "ButtonClickInvalid");

		Add2DSoundProperties("UI", "Chat");

		Add2DSoundProperties("UI", "UIIncrementalChange");
		Add2DSoundProperties("UI", "UIIncrementalError");
		
		Add2DSoundProperties("UI", "TradeAction");
		Add2DSoundProperties("UI", "ClaimLand");

		Add2DSoundProperties("UI", "FoodLowBell");
		Add2DSoundProperties("UI", "NoToolsBell");
		Add2DSoundProperties("UI", "NeedStorageBell");
		Add2DSoundProperties("UI", "DeathBell");
		Add2DSoundProperties("UI", "BabyBornBell");

		Add2DSoundProperties("UI", "TrailerMusic");
		Add2DSoundProperties("UI", "TrailerBeat");

		GetSoundPropertyRef("UI", "UIIncrementalChange", "Volume") = 2.0f;

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			return ss.str();
		};
		_debugDescriptionFuncs["UI"] = getDebugDescriptionFunc;
	}

private:
	/*
	 * Animals
	 */

	void InitAnimals()
	{
		Add3DAudioGroupProperties("Animals");
		AddGroupProperty("Animals", "UI_MasterVolume", 1.0f);

		Add3DSoundProperties("Animals", "Pig");
		Add3DSoundProperties("Animals", "PigAngry");

		Add3DSoundProperties("Animals", "Deer");
		Add3DSoundProperties("Animals", "DeerAngry");

		Add3DSoundProperties("Animals", "Panda");
		Add3DSoundProperties("Animals", "PandaAngry");

		std::vector<std::string> soundNames = GetSoundNamesFromGroup("Animals");
		for (const std::string& name : soundNames) {
			GetSoundPropertyRef("Animals", name, "PlayProbability") = 0.35f;
		}

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			return ss.str();
		};
		_debugDescriptionFuncs["Animals"] = getDebugDescriptionFunc;
	}

	std::string GetAnimalSound(UnitEnum unitEnum, bool isAngry)
	{
		PUN_CHECK(IsAnimal(unitEnum));

		switch (unitEnum) {
		case UnitEnum::Boar:
		case UnitEnum::Pig:
			return isAngry ? "PigAngry" : "Pig";
		case UnitEnum::RedDeer:
		case UnitEnum::YellowDeer:
		case UnitEnum::DarkDeer:
			return isAngry ? "DeerAngry" : "Deer";
		case UnitEnum::BrownBear:
		case UnitEnum::BlackBear:
		case UnitEnum::Panda:
			return isAngry ? "PandaAngry" : "Panda";
		default:
			return "";
		}
	}

	//float _lastPlayedAnimalSound = 0.0f;
	//const float _animalSoundMinTryInterval = 10.0f;

public:
	void SpawnAnimalSound(UnitEnum unitEnum, bool isAngry, WorldAtom2 worldAtom, bool usePlayProbability)
	{
		//float time = UGameplayStatics::GetTimeSeconds(this);
		//if (time - _lastPlayedAnimalSound < _animalSoundMinTryInterval) {
		//	return;
		//}
		//_lastPlayedAnimalSound = time;
		
		std::string animalSound = GetAnimalSound(unitEnum, isAngry);
		if (animalSound != "") {
			Spawn3DSound("Animals", animalSound, worldAtom, 3.0f, usePlayProbability);
		}
	}


private:

	void UpdateFraction(float& value, float target, float deltaTime, float fadeInDuration = 3.0f, float fadeOutDuration = 3.0f)
	{
		if (target > value) {
			value = std::min(1.0f, value + deltaTime / fadeInDuration);
		}
		else if (target < value) {
			value = std::max(0.0f, value - deltaTime / fadeOutDuration);
		}
	}

	// Helpers
	void InitAmbientSoundHelper(std::string groupName, std::string defaultSoundName, std::function<std::string()> getPreferredSoundFunc = nullptr)
	{
		Add2DSoundProperties(groupName, defaultSoundName);
		//AddPermanentAudioGroupProperties(groupName);

		AddHiddenProperty(groupName, "CurrentVolume", 1.0f);
		AddHiddenProperty(groupName, "RandomVolume", 1.0f);

		AddGroupProperty(groupName, "Group_MasterVolume", 1.0f);
		AddGroupProperty(groupName, "FadeInDuration", 8.0f);
		AddGroupProperty(groupName, "FadeOutDuration", 8.0f);
		AddGroupProperty(groupName, "CrossFadeDuration", 3.0f);
		AddGroupProperty(groupName, "RandomPauseDurationMin", 0.0f);
		AddGroupProperty(groupName, "RandomPauseDurationMax", 0.0f);

		CrossFadeInSound(groupName, defaultSoundName);

		auto getDebugDescriptionFunc = [&](std::string localGroupName) {
			std::stringstream ss;
			std::string localSoundName = GetActiveSound(localGroupName);
			ss << "Track: " << localSoundName;

			auto punAudio = GetAudioComponent(localGroupName);

			ss << std::fixed << std::showpoint << std::setprecision(1);
			float duration = punAudio->audio->Sound->GetDuration();
			ss << ", Duration: " << fmod(punAudio->playTime(this), duration) << "/" << duration;

			ss << std::setprecision(2);
			ss << ", Volume:";
			ss << "[TargetFollow: " << GetHiddenPropertyRef(localGroupName, "CurrentVolume") << ", ";
			if (_groupToGetTargetVolumeFunc.find(localGroupName) != _groupToGetTargetVolumeFunc.end()) {
				ss << "Target: " << _groupToGetTargetVolumeFunc[localGroupName]() << ", ";
			}
			ss << "Random: " << GetHiddenPropertyRef(localGroupName, "RandomVolume") << ", ";
			ss << "Final: " << punAudio->audio->VolumeMultiplier << ", ";
			ss << (punAudio->audio->IsPlaying() ? "On" : "Off") << "]";

			return ss.str();
		};
		_debugDescriptionFuncs[groupName] = getDebugDescriptionFunc;

		if (getPreferredSoundFunc) {
			_groupToGetPreferredSoundFunc[groupName] = getPreferredSoundFunc;
		}
	}

	void PunLog(std::string str) {
		if (_uiInterface) {
			_uiInterface->PunLog(str);
		}
	}

public:
	/*
	 * SoundAssets
	 */
	UPROPERTY() UPunGameInstance* gameInstance = nullptr;
	
private:
	bool HasSoundAssets(std::string soundName) {
		return gameInstance->SoundNameToAssets.Contains(ToFString(soundName));
	}

	USoundAssets* GetSoundAssets(std::string soundName) {
		PUN_CHECK(gameInstance->SoundNameToAssets.Contains(ToFString(soundName)));
		return gameInstance->SoundNameToAssets[ToFString(soundName)];
	}
	USoundAsset* GetSoundAsset(std::string soundName, int32 index) {
		PUN_CHECK(gameInstance->SoundNameToAssets.Contains(ToFString(soundName)));
		PUN_CHECK(index < gameInstance->SoundNameToAssets[ToFString(soundName)]->assets.Num());
		return gameInstance->SoundNameToAssets[ToFString(soundName)]->assets[index];
	}

	// unordered_map can be used here since USoundWave* is an import which means no gc
	//static std::unordered_map<std::string, USoundAssets> _soundNameToAssets;

	void AddPunAudio(UFireForgetAudioComponent* punAudio, UAudioComponent* audio)
	{
		punAudio->audio = audio;
		punAudios.Add(punAudio);
	}
	void RemovePunAudio(int32 i)
	{
		if (punAudios[i]->audio) {
			punAudios[i]->audio->bAutoDestroy = true;
			punAudios[i]->audio->Stop();
		}
		punAudios.RemoveAt(i);
	}
	
public:
	UPROPERTY() TArray<UFireForgetAudioComponent*> punAudios;

	std::string isolateSound = "None";
	
	float IsolateVolume(std::string groupName, std::string soundName)
	{
		if (isolateSound != "None" && isolateSound != groupName && isolateSound != soundName) {
			return 0.0f;
		}
		return 1.0f;
	}

private:
	bool shouldLoadFromPak() { return _isLoadingFromPak && !_isPackagingData; }

private:
	float _lastUpdateTime;

private:
	float _deltaTime;
	GameRandObj soundRand;

	IGameUIDataSource* _dataSource = nullptr;
	IGameUIInterface* _uiInterface = nullptr;

	IPlayerSoundSettings* _playerSettings = nullptr;
	const UObject* _worldContext = nullptr;

	bool _isPackagingData = false;
	bool _isLoadingFromPak = false;
	
	bool _bFoundSoundFiles = false; // TODO: use?

	bool _engineImportInConstructor = true;
	bool _forceUseEngineImport = true;
	TMap<FString, TArray<USoundWave*>> soundNameToWaves;

	TArray<FString> _packagingKeys;
	TArray<PunWaveModInfo> _packagingWaveInfo;
	TArray<TArray<uint8>> _packagingDataCompressed;
	TArray<TArray<uint8>> _packagingData;
	
};

