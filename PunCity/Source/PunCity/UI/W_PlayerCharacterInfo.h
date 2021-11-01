// Pun Dumnernchanvanit's

#pragma once

#include "PunWidget.h"
#include "W_PlayerCharacterInfo.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UW_PlayerCharacterInfo : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UImage* LogoForeground;
	UPROPERTY(meta = (BindWidget)) UImage* LogoBackground;
	UPROPERTY(meta = (BindWidget)) UImage* CharacterImage;

	UPROPERTY(meta = (BindWidget)) UTextBlock* FactionName;
	UPROPERTY(meta = (BindWidget)) UTextBlock* PlayerName;

	void UpdatePlayerInfo(const FPlayerInfo& playerInfo, const TArray<UTexture2D*>& playerLogos, const TArray<UTexture2D*>& playerCharacters)
	{
		LogoForeground->GetDynamicMaterial()->SetTextureParameterValue("Logo", playerLogos[playerInfo.logoIndex]);
		LogoForeground->GetDynamicMaterial()->SetVectorParameterValue("ColorForeground", playerInfo.logoColorForeground);
		LogoBackground->GetDynamicMaterial()->SetVectorParameterValue("ColorBackground", playerInfo.logoColorBackground);
		
		CharacterImage->GetDynamicMaterial()->SetTextureParameterValue("Character", playerCharacters[playerInfo.characterIndex]);

		FactionName->SetText(GetFactionInfoInt(playerInfo.factionIndex).name);
		PlayerName->SetText(playerInfo.name);
	}
	
};
