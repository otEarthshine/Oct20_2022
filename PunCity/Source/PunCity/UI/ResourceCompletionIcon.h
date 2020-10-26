// Pun Dumnernchanvanit's

#pragma once

#include "Blueprint/UserWidget.h"
#include "PunWidget.h"
#include "ResourceCompletionIcon.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UResourceCompletionIcon : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* ResourceImage;
	UPROPERTY(meta = (BindWidget)) UImage* ResourcePauseImage;

	bool sameAsLast() { return _sameAsLast; }
	void BeforeSameCheck()
	{
		_sameAsLast = true;
		_lastStringIndex = 0;
		_lastFloatIndex = 0;
		_lastIntIndex = 0;
	}
	std::string SameString(const std::string& newString)
	{
		if (_lastStringIndex >= _lastStrings.size()) {
			_lastStrings.push_back(newString);
			_sameAsLast = false;
		} 
		else if (_lastStrings[_lastStringIndex] != newString) {
			_lastStrings[_lastStringIndex] = newString;
			_sameAsLast = false;
		}
		_lastStringIndex++;
		return newString;
	}
	float SameFloat(float newFloat)
	{
		if (_lastFloatIndex >= _lastFloats.size()) {
			_lastFloats.push_back(newFloat);
			_sameAsLast = false;
		}
		else if (_lastFloats[_lastFloatIndex] != newFloat) {
			_lastFloats[_lastFloatIndex] = newFloat;
			_sameAsLast = false;
		}
		_lastFloatIndex++;
		return newFloat;
	}
	int32 SameInt(int32 newInt)
	{
		if (_lastIntIndex >= _lastInts.size()) {
			_lastInts.push_back(newInt);
			_sameAsLast = false;
		}
		else if (_lastInts[_lastIntIndex] != newInt) {
			_lastInts[_lastIntIndex] = newInt;
			_sameAsLast = false;
		}
		_lastIntIndex++;
		return newInt;
	}

	void SetIsPaused(bool isPaused)
	{
		ResourcePauseImage->SetVisibility(isPaused ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		ResourceImage->GetDynamicMaterial()->SetScalarParameterValue("IsPaused", isPaused);
	}

	int32 _lastStringIndex;
	int32 _lastFloatIndex;
	int32 _lastIntIndex;
	std::vector<std::string> _lastStrings;
	std::vector<float> _lastFloats;
	std::vector<int32> _lastInts;

	bool _sameAsLast;
};
