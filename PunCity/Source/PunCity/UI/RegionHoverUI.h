// Pun Dumnernchanvanit's

#pragma once

#include "CoreMinimal.h"

#include "BattleFieldUI.h"
#include "PunCity/UI/PunBoxWidget.h"
#include "RegionHoverUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API URegionHoverUI : public UPunWidget
{
	GENERATED_BODY()
public:
	// Battle
	UPROPERTY(meta = (BindWidget)) UBattleFieldUI* BattlefieldUI;

	// Province Overlay
	UPROPERTY(meta = (BindWidget)) UOverlay* ProvinceOverlay;

	UPROPERTY(meta = (BindWidget)) USizeBox* IconSizeBox;
	UPROPERTY(meta = (BindWidget)) UImage* IconImage;

	UPROPERTY(meta = (BindWidget)) UPunBoxWidget* PunBox;
	
	UPROPERTY(meta = (BindWidget)) UTextBlock* IncomeText;
	UPROPERTY(meta = (BindWidget)) UTextBlock* IncomeCount;
	//UPROPERTY(meta = (BindWidget)) UHorizontalBox* UpkeepBox;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* UpkeepCount;
	//UPROPERTY(meta = (BindWidget)) UHorizontalBox* BorderUpkeepBox;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* BorderUpkeepText;
	//UPROPERTY(meta = (BindWidget)) UTextBlock* BorderUpkeepCount;

	UPROPERTY(meta = (BindWidget)) UIconTextPairWidget* IconPair1;

	int32 provinceId = -1;
	
	void UpdateBattlefieldUI(int32 provinceIdIn, ProvinceClaimProgress claimProgress)
	{
		provinceId = provinceIdIn;

		SetChildHUD(BattlefieldUI);
		BattlefieldUI->UpdateUI(provinceId, claimProgress);
	}

	void UpdateProvinceOverlayInfo(int32 provinceIdIn)
	{
		provinceId = provinceIdIn;
		
		auto& sim = simulation();
		int32 provincePlayerId = sim.provinceOwnerPlayer(provinceIdIn);
		bool unlockedInfluence = sim.unlockedInfluence(playerId());

		//UpkeepText->SetVisibility(ESlateVisibility::Collapsed);
		//UpkeepBox->SetVisibility(ESlateVisibility::Collapsed);
		//BorderUpkeepText->SetVisibility(ESlateVisibility::Collapsed);
		//BorderUpkeepBox->SetVisibility(ESlateVisibility::Collapsed);
		
		IconSizeBox->SetVisibility(ESlateVisibility::Collapsed);

#define LOCTEXT_NAMESPACE "RegionHoverUI"
		const FText incomeText = LOCTEXT("Income:", "Income:");
		const FText upkeepText = LOCTEXT("Upkeep:", "Upkeep:");
		
		// Already own this province, Show real income/upkeep
		if (provincePlayerId == playerId())
		{
			SetText(IncomeText, incomeText);
			SetTextNumber(IncomeCount, sim.GetProvinceIncome100(provinceIdIn) / 100.0f, 1);
			
			//if (unlockedInfluence) {
			//	SetText(UpkeepText, upkeepText);
			//	SetTextNumber(UpkeepCount, sim.GetProvinceUpkeep100(provinceIdIn, provincePlayerId) / 100.0f, 1);
			//	UpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//	UpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			//	if (!sim.provinceInfoSystem().provinceOwnerInfo(provinceId).isSafe) 
			//	{
			//		SetText(BorderUpkeepText, GetInfluenceIncomeName(InfluenceIncomeEnum::UnsafeProvinceUpkeep));
			//		SetTextNumber(BorderUpkeepCount, 10, 1);
			//			
			//		BorderUpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//		BorderUpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//	}
			//}
		}
		else
		{
			SetText(IncomeText, incomeText);
			SetTextNumber(IncomeCount, sim.GetProvinceIncome100(provinceIdIn) / 100.0f, 1);

			//if (unlockedInfluence) {
			//	SetText(UpkeepText, upkeepText);
			//	SetTextNumber(UpkeepCount, sim.GetProvinceBaseUpkeep100(provinceIdIn) / 100.0f, 1);
			//	UpkeepText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//	UpkeepBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			//}
		}

		// Protected vs Unprotected
		int32 originTownId = sim.provinceOwnerTownSafe(provinceId);
		if (sim.townPlayerId(originTownId) == playerId())
		{
			const ProvinceOwnerInfo& provinceOwnerInfo = sim.provinceInfoSystem().provinceOwnerInfo(provinceId);
			IconPair1->SetImage(nullptr);
			IconPair1->SetText(provinceOwnerInfo.isSafe ? LOCTEXT("Protected", "Protected") : LOCTEXT("Unprotected", "Unprotected"), FText());
			IconPair1->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else {
			IconPair1->SetVisibility(ESlateVisibility::Collapsed);
		}
#undef LOCTEXT_NAMESPACE
		

		GeoresourceSystem& georesourceSys = sim.georesourceSystem();
		GeoresourceNode node = georesourceSys.georesourceNode(provinceId);
		ProvinceSystem& provinceSys = sim.provinceSystem();
		bool isMountain = provinceSys.provinceMountainTileCount(provinceId) > 0;
		SetChildHUD(PunBox);
		
		if (node.HasResource())
		{
			// Oil Invisible before research
			bool shouldShowGeoresource = true;
			if (node.georesourceEnum == GeoresourceEnum::Oil) {
				if (!sim.IsResearched(playerId(), TechEnum::Petroleum)) {
					if (!PunSettings::IsOn("ShowAllResourceNodes")) {
						shouldShowGeoresource = false;
					}
				}
			}

			if (shouldShowGeoresource)
			{
				ResourceEnum resourceEnum = node.info().resourceEnum;

				SetGeoresourceImage(IconImage, resourceEnum, assetLoader(), this);

				IconSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

				if (node.depositAmount > 0) {
					PunBox->AddIconPair(FText(), node.info().resourceEnum, TEXT_NUM(node.depositAmount));
				}
			}
		}
		
		if (isMountain) {
			PunBox->AddIconPair(FText(), ResourceEnum::Stone, TEXT_NUM(node.stoneAmount));
		}

		PunBox->AfterAdd();
	}

	
};
