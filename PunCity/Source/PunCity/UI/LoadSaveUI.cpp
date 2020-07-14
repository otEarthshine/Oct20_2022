// Pun Dumnernchanvanit's


#include "LoadSaveUI.h"
#include "EscMenuUI.h"
#include "PunCity/PunGameInstance.h"

void ULoadSaveUI::PunInit(UPunWidget* parent)
{
	_callbackParent = parent;
	BackButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickBackButton);
	LoadGameButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickSaveLoadGameButton);
	DeleteGameButton->OnClicked.AddDynamic(this, &ULoadSaveUI::OnClickDeleteGameButton);

	SelectedSavePlayerNameEditable->OnTextChanged.AddDynamic(this, &ULoadSaveUI::OnSaveNameChanged);
	SelectedSavePlayerNameEditable->OnTextCommitted.AddDynamic(this, &ULoadSaveUI::OnSaveNameCommited);

	SaveSelectionList->ClearChildren();

	SavingBlur->SetVisibility(ESlateVisibility::Collapsed);
	LoadingBlur->SetVisibility(ESlateVisibility::Collapsed);
}

void ULoadSaveUI::OnClickBackButton()
{
	Spawn2DSound("UI", "UIWindowClose");
	_callbackParent->CallBack2(_callbackParent, CallbackEnum::CloseLoadSaveUI);
}
void ULoadSaveUI::OnClickSaveLoadGameButton()
{
	// Load game:
	//  Server travel
	//  Load information into simulation / Camera position

	if (_isSavingGame) {
		FString nameString = SelectedSavePlayerNameEditable->Text.ToString();
		bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
		if (isSaveButtonEnabled) 
		{
			if (saveSystem().HasExistingSave(SelectedSavePlayerNameEditable->Text.ToString())) {
				_callbackParent->CallBack2(_callbackParent, CallbackEnum::SaveGameOverrideConfirm);
			} else {
				SaveGameDelayed();
			}
		}
	}
	else  {
		if (activeIndex != -1) {
			LoadGameDelayed();
		}
	}

	Spawn2DSound("UI", "ButtonClick");
}
void ULoadSaveUI::OnClickDeleteGameButton()
{
	if (activeIndex != -1)
	{
		const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
		int32 oldSaveListCount = saveList.Num();
		PUN_CHECK(activeIndex < oldSaveListCount)
		
		saveSystem().DeleteSave(saveList[activeIndex].folderPath);

		RefreshSaveSelectionList(SaveActiveIndex_SelectFirstAvailable);
	}

	Spawn2DSound("UI", "ButtonClick");
}

void ULoadSaveUI::OnSaveNameChanged(const FText& text)
{
	// Disable save button if there is no name...
	bool isSaveButtonEnabled = SelectedSavePlayerNameEditable->Text.ToString().Len() > 0;
	SetButtonEnabled(LoadGameButton, isSaveButtonEnabled ? ButtonStateEnum::Enabled : ButtonStateEnum::Disabled);
}

void ULoadSaveUI::OnSaveNameCommited(const FText& text, ETextCommit::Type CommitMethod)
{
	// if text is no longer the same as activeIndex, release the highlight
	const TArray<GameSaveInfo>& saveList = saveSystem().saveList();
	FString newName = text.ToString();
	if (activeIndex != -1 && saveList[activeIndex].name != newName) {
		TArray<UWidget*> children = SaveSelectionList->GetAllChildren();
		PUN_CHECK(activeIndex < children.Num());
		CastChecked<UPunSelectButton>(children[activeIndex])->SetHighlight(false);
	}
}