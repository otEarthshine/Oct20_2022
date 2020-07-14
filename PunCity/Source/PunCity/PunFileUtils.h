// Pun Dumnernchanvanit's

#pragma once

#include "Misc/FileHelper.h"
#include "PunCity/PunUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/AES.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "PunCity/PunTimer.h"


/**
 * 
 */
class PROTOTYPECITY_API PunFileUtils
{
public:

	template <typename Func>
	static bool LoadFile(FString path, Func serializeFunc, bool shouldCompress = false, FAES::FAESKey* aesKey = nullptr)
	{
		TArray<uint8> binary;
		//if (!FFileHelper::LoadFileToArray(binary, *path)) {
		//	_LOG(PunSaveLoad, "Load file from path: %s", *path);
		//	return false;
		//}

		auto tryDecrypt = [&]()
		{
			if (aesKey)
			{
				PUN_LOG("DecryptData: num:%d", binary.Num());
				FAES::DecryptData(binary.GetData(), binary.Num(), *aesKey);

				PUN_LOG("DecryptData Before pad removal: num:%d first:%d end:%d", binary.Num(), binary[0], binary[binary.Num() - 1]);
				uint8 padAmount = binary[binary.Num() - 1];
				for (int32 i = 0; i < padAmount + 1; i++) { // +1 includes the signal byte
					binary.RemoveAt(binary.Num() - 1);
				}

				PUN_LOG("DecryptData: pad:%d num:%d", padAmount, binary.Num());
			}
		};


		if (shouldCompress)
		{
			TArray<uint8> binaryCompressed;
			if (!FFileHelper::LoadFileToArray(binaryCompressed, *path)) {
				_LOG(PunSaveLoad, "Invalid Load file from path: %s", *path);
				return false;
			}
			_LOG(PunSaveLoad, "Loaded GameData Compressed %d", binaryCompressed.Num());
			PUN_CHECK(binaryCompressed.Num() > 0);

			// Decompress File 
			FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(binaryCompressed, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
			if (Decompressor.GetError()) {
				LOG_ERROR(PunSaveLoad, "File Was Not Compressed ");
				return false;
			}

			//Decompress
			Decompressor << binary;

			binaryCompressed.Empty();
			Decompressor.FlushCache();
		}
		else
		{
			if (!FFileHelper::LoadFileToArray(binary, *path)) {
				_LOG(PunSaveLoad, "Invalid Load file from path: %s", *path);
				return false;
			}
		}

		PUN_CHECK(binary.Num() > 0);
		_LOG(PunSaveLoad, "Loaded binary size:%d", binary.Num());
		

		FMemoryReader LoadArchive(binary, true);
		LoadArchive.Seek(0);
		LoadArchive.SetIsSaving(false);
		LoadArchive.SetIsLoading(true);

		serializeFunc(LoadArchive);

		LoadArchive.FlushCache();
		binary.Empty();
		LoadArchive.Close();
		
		return true;
	}

	template <typename Func>
	static bool SaveFile(FString path, Func serializeFunc, bool shouldCompress = false, FAES::FAESKey* aesKey = nullptr)
	{
		FBufferArchive SaveArchive;
		SaveArchive.SetIsSaving(true);
		SaveArchive.SetIsLoading(false);
		
		auto tryEncrypt = [&]()
		{
			if (aesKey)
			{
				PUN_LOG("encrypted: num:%d", SaveArchive.Num());

				// Pad to 16 bytes and append the number of bytes padded
				int32 remainderBytes = SaveArchive.Num() % 16;

				// 0 byte.. pad 15 .. 1 signal
				// 1 byte.. pad 14 .. 1 signal
				// 5 byte.. pad 10 .. 1 signal
				// 14 byte.. pad 1 .. 1 signal
				// 15 byte.. pad 0 .. 1 signal
				// 
				uint8 padAmount = 15 - remainderBytes;
				for (int32 i = 0; i < padAmount; i++) {
					SaveArchive.Add(0);
				}
				SaveArchive.Add(padAmount);
				PUN_LOG("encrypted: pad:%d,%d save:%d num:%d", padAmount, SaveArchive.Last(), SaveArchive[0], SaveArchive.Num());

				FAES::EncryptData(SaveArchive.GetData(), SaveArchive.Num(), *aesKey);
			}
		};

		serializeFunc(SaveArchive);


		bool succeed = false;
		
		if (shouldCompress)
		{
			TArray<uint8> CompressedData;
			FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, NAME_Zlib, ECompressionFlags::COMPRESS_BiasSpeed);
			{
				SCOPE_TIMER("Save Compress");

				Compressor << SaveArchive;
				Compressor.Flush();
				_LOG(PunSaveLoad, "Save GameData Compressed size:%d", CompressedData.Num());
			}

			// Encrypt after compression
			tryEncrypt();
			
			{
				SCOPE_TIMER("Save to file");
				succeed = FFileHelper::SaveArrayToFile(CompressedData, *path);
			}

			Compressor.FlushCache();
			CompressedData.Empty();
		}
		else
		{
			tryEncrypt();
			
			succeed = FFileHelper::SaveArrayToFile(SaveArchive, *path);
		}

		

		//bool succeed = FFileHelper::SaveArrayToFile(SaveArchive, *path);
		SaveArchive.FlushCache();
		SaveArchive.Empty();

		if (!succeed) {
			PUN_LOG("Save file to path:%s", *path);
		}
		return succeed;
	}

	
};
