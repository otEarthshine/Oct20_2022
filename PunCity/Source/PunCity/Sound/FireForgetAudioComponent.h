// Pun Dumnernchanvanit's

#pragma once

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include <string>
#include "Sound/SoundWaveProcedural.h"
#include "PunCity/GameRand.h"
#include "Async/Async.h"
#include "PunCity/Simulation/GameCoordinate.h"

#include "PunCity/PunSTLContainerOverride.h"

#include "FireForgetAudioComponent.generated.h"

/**
 * 2D or 3D sound
 * - Handles seamless switching between imported USoundWave and USoundWaveProcedural
 * - Handles USoundWaveProcedural termination once the duration is up...
 * - Handles fade for USoundWaveProcedural
 */
//class IPunAudioSystem
//{
//public:
//	virtual USoundWave* GetSound(std::string soundName) = 0;
//	virtual TArray<uint8>& GetRawPCM(std::string soundName) = 0;
//	virtual void PunLog(std::string text) = 0;
//};

// Keep track of ProceduralAudio startTime so we can terminate it...
//class FireAndForgetProceduralAudio
//{	
//	float startTime = 0.0f;
//	bool isDone = false;
//	bool isStopped = false;
//	
//	UAudioComponent* audio = nullptr;
//};

//class FireAndForgetAudioSystem
//{
//public:
//	void Init(IPunAudioSystem* audioSystem)
//	{
//		_audioSystem = audioSystem;
//	}
//	
//	FireAndForgetAudio SpawnSound2D(const UObject* context, std::string soundName)
//	{
//		FireAndForgetAudio fireAudio;
//		fireAudio.audio = UGameplayStatics::SpawnSound2D(context, _audioSystem->GetSound(soundName));
//		fireAudio.startTime = UGameplayStatics::GetAudioTimeSeconds(context);
//		fireAudio.isProcedural = Cast<USoundWaveProcedural>(fireAudio.audio) != nullptr;
//		fireAudio.isDone = false;
//
//		// For procedural sound, also need to add bind underflow
//		if (fireAudio.isProcedural)
//		{
//			USoundWaveProcedural* soundProc = CastChecked<USoundWaveProcedural>(fireAudio.audio);
//			
//			std::string soundNameBind = soundName;
//			float startTime = UGameplayStatics::GetAudioTimeSeconds(context);
//			soundProc->OnSoundWaveProceduralUnderflow.BindLambda([this, soundNameBind, startTime](USoundWaveProcedural* soundLamb, const int32 SamplesNeeded)
//			{
//				if (UGameplayStatics::GetAudioTimeSeconds(context) - startTime > )
//				{
//					
//				}
//				TArray<uint8>& rawPCM = audioSystem->GetRawPCM(soundName);
//				soundLamb->QueueAudio(rawPCM.GetData(), rawPCM.Num());
//
//				AsyncTask(ENamedThreads::GameThread, [&]() {
//					audioSystem->PunLog("QueueAudio(BindLambda)");
//				});
//			});
//		}
//	}
//
//	
//
//private:
//	IPunAudioSystem* _audioSystem = nullptr;
//
//	std::vector<FireAndForgetAudio> 
//};

/*
 * Name is misleading...
 * - this is not a component
 * - this is used for more than just fireAndForget sounds
 */
UCLASS()
class PROTOTYPECITY_API UFireForgetAudioComponent : public UObject
{
	GENERATED_BODY()
public:
	float startTime = 0.0f;
	float duration = 0.0f;
	int32 byteCount = -1;

	//FThreadSafeBool bPlayStarted = false;
	//FThreadSafeBool isDone = false;

	std::string soundName;
	std::string groupName;
	int32 assetIndex = -1;

	bool isLooping = true;

	WorldAtom2 spotAtom;
	float spotHeight = -1.0f;
	bool shouldUpdateSpot = true;

	UPROPERTY() UAudioComponent* audio;
	UPROPERTY() USoundWave* sound;

	// Building
	bool isBuildingSound = false;
	int32 buildingId = -1;
	int32 regionId = -1;

	// Building random sound
	bool hasOneShotSound = false;
	float nextRandomSoundPlayTime = -1.0f;
	//UPROPERTY() UAudioComponent* randomAudio;

	// Others
	bool isFadingOut = false;
	float fadeStartTime = -1.0f;
	float fadeEndTime = -1.0f;

public:
	float playTime(const UObject* WorldContextObject) { return UGameplayStatics::GetAudioTimeSeconds(WorldContextObject) - startTime; }

	bool isFinished(const UObject* WorldContextObject) { return playTime(WorldContextObject) > audio->Sound->GetDuration(); }
};
