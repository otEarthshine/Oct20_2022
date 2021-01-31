// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"

#include "ExclamationIcon.h"
#include "QuestScreenElement.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UQuestScreenElement : public UPunWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(meta = (BindWidget)) UTextBlock* QuestTitle;
	UPROPERTY(meta = (BindWidget)) UTextBlock* QuestDescription;
	UPROPERTY(meta = (BindWidget)) UTextBlock* QuestNumberDescription;

	UPROPERTY(meta = (BindWidget)) UImage* QuestImage;

	UPROPERTY(meta = (BindWidget)) USizeBox* QuestNumberSizeBox;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ImagesIncomplete;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ImagesComplete;
	UPROPERTY(meta = (BindWidget)) UHorizontalBox* ImagesSpecialQuest;

	UPROPERTY(meta = (BindWidget)) UImage* QuestBar;

	UPROPERTY(meta = (BindWidget)) UExclamationIcon* ExclamationIcon;

	void Setup(std::shared_ptr<Quest> quest, UPunWidget* callbackParent, CallbackEnum callbackEnum)
	{
		// Changed quest... reset this
		if (questEnum != quest->classEnum())
		{
			questEnum = quest->classEnum();
			_timesClicked = 0;
			ExclamationIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		//if (TextEquals(title, quest->questTitle()) ||
		//	TextEquals(numberDescription, quest->numberDescription()))
		//{
			title = quest->questTitle();
			numberDescription = quest->numberDescription();

			QuestTitle->SetText(title);
			QuestNumberDescription->SetText(numberDescription);
		//}

		if (fraction != quest->fraction())
		{
			fraction = quest->fraction();
			QuestBar->GetDynamicMaterial()->SetScalarParameterValue("Fraction", fraction);
		}

		_callbackParent = callbackParent;
		_callbackEnum = callbackEnum;

		// Quest image
		switch (quest->classEnum())
		{
		case QuestEnum::GatherMarkQuest: SetResourceImage(QuestImage, ResourceEnum::Wood, assetLoader()); break;
			
		case QuestEnum::CooperativeFishingQuest: SetResourceImage(QuestImage, ResourceEnum::Fish, assetLoader()); break;
		case QuestEnum::BeerQuest: SetResourceImage(QuestImage, ResourceEnum::Beer, assetLoader()); break;
		case QuestEnum::PotteryQuest: SetResourceImage(QuestImage, ResourceEnum::Pottery, assetLoader()); break;

		case QuestEnum::TradeQuest: QuestImage->SetBrushFromTexture(assetLoader()->CoinIcon); break;

		default:
			QuestImage->SetBrushFromTexture(assetLoader()->HouseIcon);
		}

		// Special Quest Mark as another color
		if (IsImportantQuest(quest->classEnum())) {
			ImagesSpecialQuest->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ImagesIncomplete->SetVisibility(ESlateVisibility::Collapsed);
			ImagesComplete->SetVisibility(ESlateVisibility::Collapsed);
		} else {
			ImagesSpecialQuest->SetVisibility(ESlateVisibility::Collapsed);
			ImagesIncomplete->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			ImagesComplete->SetVisibility(ESlateVisibility::Collapsed);
		}

		ExclamationIcon->SetShow(_timesClicked == 0);
	}

	QuestEnum questEnum = QuestEnum::None;
	FText title;
	FText numberDescription;
	float fraction = -1.0f;

private:
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY() UPunWidget* _callbackParent;
	CallbackEnum _callbackEnum;

	int32 _timesClicked = 0;
};
