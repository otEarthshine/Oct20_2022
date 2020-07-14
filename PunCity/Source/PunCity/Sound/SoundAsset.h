// Pun Dumnernchanvanit's

#pragma once

#include "UObject/NoExportTypes.h"
#include "OpusAudioInfo.h"
#include "PunCity/Simulation/GameSimulationInfo.h"
#include "SoundCompressor.h"
#include "PunCity/PoolArray.h"
#include "PunCity/PunUtils.h"

#include "SoundAsset.generated.h"


struct PunWaveModInfo
{
	int32 channelCount;
	int32 sizeOfSample;
	int32 numSamples;
	int32 numFrames;

	int32 samplesPerSec;
	int32 sampleDataSize;
	float duration;

	PunWaveModInfo() {}

	PunWaveModInfo(FWaveModInfo waveInfo) :
		channelCount(*waveInfo.pChannels),
		sizeOfSample((*waveInfo.pBitsPerSample) / 8),
		numSamples(waveInfo.SampleDataSize / sizeOfSample),
		numFrames(numSamples / channelCount),
		samplesPerSec(*waveInfo.pSamplesPerSec),
		sampleDataSize(*waveInfo.pWaveDataSize),
		duration((float)numFrames / samplesPerSec)
	{}

	FSoundQualityInfo qualityInfo() const
	{
		FSoundQualityInfo result;
		result.Quality = 100;
		result.NumChannels = channelCount;
		result.SampleRate = samplesPerSec;
		result.SampleDataSize = sampleDataSize;
		result.Duration = duration;
		result.bStreaming = false;
		return result;
	}

	friend FArchive& operator<<(FArchive& Ar, PunWaveModInfo& waveInfo)
	{
		Ar << waveInfo.channelCount;
		Ar << waveInfo.sizeOfSample;
		Ar << waveInfo.numSamples;
		Ar << waveInfo.numFrames;

		Ar << waveInfo.samplesPerSec;
		Ar << waveInfo.sampleDataSize;
		Ar << waveInfo.duration;

		PUN_CHECK(waveInfo.duration > 0);
		return Ar;
	}
};

// Need this for empty feed which Num() might not be the number of bytes we want to play
struct PunAudioData
{
	uint8* data = nullptr;
	int32 byteCount = 0;
};

static TArray<uint8> emptyFeed;

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API USoundAsset : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY() USoundWave* sound = nullptr; // Sound imported from sound asset

	bool isRawData = false; // Use rawData vs. USoundWave
	UPROPERTY() UPoolArray* compressedData; // For most part, a sound asset is compressed
	UPROPERTY() UPoolArray* data; // the 
	PunWaveModInfo waveInfo;

	std::string soundName;
	int32 assetIndex = -1;

	TFuture<uint8> bUncompressedDataReady;
	FThreadSafeBool bUsedUncompressedData = false;

	PunAudioData GetData()
	{
		// if the data isn't ready, give an empty feed (20 sec)
		if (!isPermanentUncompressed() && !bUncompressedDataReady.IsReady()) 
		{
			PUN_LOG("GetData emptyFeed");
			int32 bytePerSec = waveInfo.samplesPerSec * waveInfo.sizeOfSample;
			int32 byteCount = bytePerSec * 20;
			return { emptyFeed.GetData(), byteCount };
		}
		
		PUN_CHECK(data != nullptr);
		return { data->data.GetData(), data->data.Num() };
	}

	bool isPermanentUncompressed() {
		return compressedData->data.Num() == 0;
	}

	float GetDuration() {
		return isRawData ? waveInfo.duration : sound->GetDuration();
	}

	std::string ToString() {
		std::stringstream ss;
		ss << "[SoundAsset: " << soundName << " assetIndex:" << assetIndex << " rawPCM:" << data->data.Num() << "]";
		return ss.str();
	}

};
