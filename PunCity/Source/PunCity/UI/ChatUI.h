// Pun Dumnernchanvanit's

#pragma once

#include "PunCity/UI/PunWidget.h"

#include "PunRichText.h"
#include "HiddenSettingsRow.h"

#include "PunCity/Sound/SoundSystemComponent.h"

#include "ChatUI.generated.h"

/**
 * 
 */
UCLASS()
class PROTOTYPECITY_API UChatUI : public UPunWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget)) UOverlay* ChatOverlay;
	UPROPERTY(meta = (BindWidget)) UEditableTextBox* ChatInputBox;
	UPROPERTY(meta = (BindWidget)) UScrollBox* ChatBox;
	//UPROPERTY(meta = (BindWidget)) UVerticalBox* ChatBoxWhenInactive;

	UPROPERTY(meta = (BindWidget)) UButton* ChatMinimizeButton;
	UPROPERTY(meta = (BindWidget)) UImage* ChatMinimizeImage;
	UPROPERTY(meta = (BindWidget)) USizeBox* ChatSizeBox;

	UPROPERTY(meta = (BindWidget)) UTextBlock* FontExampleChat;

	// Debug
	UPROPERTY(meta = (BindWidget)) UOverlay* DebugOverlay;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TopLeftTextDebug;
	
	
	void PunInit()
	{
		ChatInputBox->OnTextCommitted.AddDynamic(this, &UChatUI::ChatCommited);
		ChatInputBox->ClearKeyboardFocusOnCommit = true;
		ChatInputBox->SelectAllTextWhenFocused = true;
		ChatInputBox->SetText(FText::FromString(FString("")));
		ChatInputBox->SetVisibility(ESlateVisibility::Visible);
		ChatBox->ClearChildren();
		//ChatBoxWhenInactive->ClearChildren();

		HiddenSettingsResetSoundButton->OnClicked.AddDynamic(this, &UChatUI::HiddenSettingsResetSound);
		HiddenSettingsSaveButton->OnClicked.AddDynamic(this, &UChatUI::HiddenSettingsSave);
		HiddenSettingsLoadButton->OnClicked.AddDynamic(this, &UChatUI::HiddenSettingsLoad);
		HiddenSettingsCloseButton->OnClicked.AddDynamic(this, &UChatUI::HiddenSettingsClose);
		HiddenSettingsBox->ClearChildren();
		HiddenSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);

		PunLogBox->ClearChildren();
		PunLogCloseButton->OnClicked.AddDynamic(this, &UChatUI::PunLogClose);
		PunLogOverlay->SetVisibility(ESlateVisibility::Collapsed);

		HiddenSettingsIsolateSoundDropdown->OnSelectionChanged.AddDynamic(this, &UChatUI::OnIsolateSoundDropdownChanged);

		GetAnimations(Animations);
		check(Animations.Num() > 0);

		ChatMinimizeButton->OnClicked.AddDynamic(this, &UChatUI::ChatToggleMinimize);

		// Debug
		DebugOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	void Tick();

	void TickDebugUI();

	void ScrollChatToEnd()
	{
		ChatBox->ScrollWidgetIntoView(ChatBox->GetChildAt(ChatBox->GetChildrenCount() - 1), false, EDescendantScrollDestination::Center);
	}

	void RefreshChatUI()
	{
		auto& chatSystem = simulation().chatSystem();

		int32 lastChatNum = ChatBox->GetChildrenCount();
		
		ChatBox->ClearChildren();
		//ChatBoxWhenInactive->ClearChildren();

		// Note: last messages are the most recent
		TArray<FSendChat> messages = chatSystem.messages();

		// Lowest Child in ChatBox is the most recent..
		for (int32 i = 0; i < messages.Num(); i++) {
			ChatBox->AddChild(AddChatRow(messages[i]));
		}

		//for (int32 i = std::max(0, messages.Num() - 5); i < messages.Num(); i++) {
		//	ChatBoxWhenInactive->AddChild(AddChatRow(messages[i]));
		//}

		// Not self, make sound
		if (messages.Num() > 0 && messages.Last().playerId != playerId()) {
			Spawn2DSound("UI", "Chat");
		}

		chatSystem.needRefreshChatUI = false;

		// Isn't chat focused, and the chat length just changed... show the inactive chat
		if (!networkInterface()->IsChatFocus() && ChatBox->GetChildrenCount() > lastChatNum) {
			_chatFadeOutStartTime = UGameplayStatics::GetTimeSeconds(this);
		}

		ScrollChatToEnd();

		//GetWorld()->GetTimerManager().ClearTimer(timerHandle);
		//GetWorld()->GetTimerManager().SetTimer(timerHandle, timerDelegate, 10.0f, false);
	}

	bool IsHoveredOnScrollUI() {
		return ChatOverlay->IsHovered() || 
				HiddenSettingsOverlay->IsHovered();
	}
	void CheckChildrenPointerOnUI() {
		CheckPointerOnUI(ChatOverlay);
		//CheckPointerOnUI(ChatInputBox);
		CheckPointerOnUI(HiddenSettingsOverlay);
	}

	void OnSetFocusChat()
	{
		//_isChatFocused = true;
		//ChatBox->ScrollToEnd();
		ScrollChatToEnd();
		
		//RefreshChatBoxState();
	}
	void OnSetFocusGame()
	{
		//_isChatFocused = false;
		//RefreshChatBoxState();
		_chatFadeOutStartTime = UGameplayStatics::GetTimeSeconds(this);
	}

	void RefreshChatBoxState()
	{
		//if (_isChatFocused) {
		//	//ChatInputBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		//	ChatBox->SetVisibility(ESlateVisibility::Visible);

		//	//ChatBoxWhenInactive->SetVisibility(ESlateVisibility::Collapsed);
		//}
		//else 
		//{
		//	//ChatInputBox->SetVisibility(ESlateVisibility::Hidden);
		//	ChatBox->SetVisibility(ESlateVisibility::Collapsed);

		//	// TODO: fade out faster if have ExclusiveUI open
		//	
		//	float fadeOutTimeElapsed = UGameplayStatics::GetTimeSeconds(this) - _chatFadeOutStartTime;
		//	if (fadeOutTimeElapsed < 10.0f) {
		//		ChatBoxWhenInactive->SetRenderOpacity(std::min(1.0f, -0.5f * (fadeOutTimeElapsed - 10.0f)));
		//		ChatBoxWhenInactive->SetVisibility(ESlateVisibility::HitTestInvisible);
		//	} else { 
		//		ChatBoxWhenInactive->SetVisibility(ESlateVisibility::Collapsed);
		//		_chatFadeOutStartTime = -1.0f;
		//	}
		//}
	}

	int32 SafeStoi(std::string str)
	{
		int32 value = 1;
		//try {
			value = std::stoi(str);
		//}
		//catch (std::invalid_argument& e) {
		//	// if no conversion could be performed
		//	PUN_LOG("%s", ToTChar(std::string(e.what())));
		//	return 1;
		//}
		return value;
	}
	int32 SafeStoi(std::wstring str) {
		// TODO: make it safe eventually?
		return std::stoi(str);
	}

	UFUNCTION() void ChatToggleMinimize() {
		if (ChatSizeBox->HeightOverride > 299) {
			ChatSizeBox->SetHeightOverride(130);
			ChatMinimizeImage->SetRenderScale(FVector2D(1, 1));
		} else {
			ChatSizeBox->SetHeightOverride(300);
			ChatMinimizeImage->SetRenderScale(FVector2D(1, -1));
		}
	}

	UFUNCTION() void ChatCommited(const FText& Text, ETextCommit::Type CommitMethod)
	{
		//PUN_LOG("ChatCommited");
		if (CommitMethod != ETextCommit::OnEnter) {
			return;
		}

		networkInterface()->SetFocusGame();

		if (Text.ToString().IsEmpty()) {
			return;
		}

		_LOG(LogNetworkInput, "ChatCommited %d, %s", playerId(), *Text.ToString());

		// Send chat
		{
			auto command = std::make_shared<FSendChat>();
			command->playerId = playerId();
			command->message = Text.ToString();
			networkInterface()->SendNetworkCommand(command);

			// Add chat right away
			//ChatBoxWhenInactive->AddChild(AddChatRow(*command));
			ChatBox->AddChild(AddChatRow(*command));
			ScrollChatToEnd();
			
			Spawn2DSound("UI", "Chat");
		}

		ChatInputBox->SetText(FText::FromString(FString("")));

		// If typed Alistair... show the hidden menu
		if (Text.ToString() == FString("Alistair")) {
			HiddenSettingsOverlay->SetVisibility(ESlateVisibility::Visible);
			RefreshHiddenSettingsUI();
		}
		if (Text.ToString() == FString("PunLog")) {
			PunLogOverlay->SetVisibility(ESlateVisibility::Visible);
		}

		// Parse command
		std::wstring message = ToWString(Text.ToString());
		std::vector<std::wstring> commandAndParams;
		std::wstring currentString;
		bool isInQuotation = false;
		for (size_t i = 0; i < message.size(); i++)
		{
			if (message[i] == '"') {
				isInQuotation = !isInQuotation;
			}
			else if (!isInQuotation && message[i] == ' ') {
				commandAndParams.push_back(currentString);
				currentString.clear();
			}
			else {
				currentString.push_back(message[i]);
			}
		}
		if (currentString.size() > 0) {
			commandAndParams.push_back(currentString);
			currentString.clear();
		}

		// Cheat command
		if (commandAndParams.size() >= 2)
		{
			bool isCheatCommand = (commandAndParams[0] == TEXT("Cheat"));
			bool isDebugCommand = false;
#if !UE_BUILD_SHIPPING
			isDebugCommand = (commandAndParams[0] == TEXT("Debug"));
#endif

			if (isCheatCommand || isDebugCommand)
			{
				for (int i = 0; i < _countof(CheatName); i++) {
					if (ToFString(CheatName[i]).Equals(ToFString(commandAndParams[1]))) {
						auto command = std::make_shared<FCheat>();
						command->cheatEnum = static_cast<CheatEnum>(i);

						if (commandAndParams.size() >= 3) {
							if (command->cheatEnum == CheatEnum::PunTog ||
								command->cheatEnum == CheatEnum::PunGet) {
								command->stringVar1 = FString(commandAndParams[2].c_str());
							}
							else {
								command->var1 = FCString::Atoi(commandAndParams[2].c_str());
								if (commandAndParams.size() >= 4) {
									command->var2 = FCString::Atoi(commandAndParams[3].c_str());
								}
							}
						}

						networkInterface()->SendNetworkCommand(command);
					}
				}
			}
		}

		// AddWildCards
		if (commandAndParams.size() >= 1 && commandAndParams[0] == TEXT("AddWildCard")) {
			int32 addCount = commandAndParams.size() >= 2 ? SafeStoi(commandAndParams[1]) : 3;
			for (int32 i = 0; i < addCount; i++) {
				simulation().cardSystem(playerId()).AddCardToHand2(CardEnum::WildCard);
			}
		}

		// AddResource
		if (commandAndParams.size() >= 3 && commandAndParams[0] == TEXT("AddResource"))
		{
			ResourceEnum resourceEnum = FindResourceEnumByName(commandAndParams[1]);
			if (resourceEnum == ResourceEnum::None) {
				return;
			}

			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddResource");
			command->var1 = static_cast<int32>(resourceEnum);
			command->var2 = FCString::Atoi(commandAndParams[2].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		// AddMoney
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddMoney"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddMoney");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		// AddInfluence
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddInfluence"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddInfluence");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		// AddCard
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddCard"))
		{
			CardEnum cardEnum = FindCardEnumByName(commandAndParams[1]);
			if (cardEnum == CardEnum::None) {
				return;
			}
			
			int32 addCount = commandAndParams.size() >= 3 ? FCString::Atoi(commandAndParams[2].c_str()) : 1;
			if (addCount <= 0) {
				return;
			}

			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddCard");
			command->var1 = static_cast<int32>(cardEnum);
			command->var2 = addCount;
			networkInterface()->SendNetworkCommand(command);
		}

		// AddImmigrants
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddImmigrants"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddImmigrants");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		// AddAIImmigrants
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddAIImmigrants"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddAIImmigrants");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}
		// AddAIMoney
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("AddAIMoney"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("AddAIMoney");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		// HouseLevel
		if (commandAndParams.size() >= 2 && commandAndParams[0] == TEXT("HouseLevel"))
		{
			auto command = make_shared<FCheat>();
			command->cheatEnum = GetCheatEnum("HouseLevel");
			command->var1 = FCString::Atoi(commandAndParams[1].c_str());
			networkInterface()->SendNetworkCommand(command);
		}

		if (commandAndParams.size() >= 1 && commandAndParams[0] == TEXT("PleaseCrash"))
		{
			UObject* nullObj = nullptr;
			nullObj->BeginDestroy();
			checkNoEntry();
		}

		// Test
		if (commandAndParams.size() >= 3 && commandAndParams[0] == TEXT("TestAchievement"))
		{
			std::wstring achievementId = commandAndParams[1];
			int32 percent = FCString::Atoi(commandAndParams[2].c_str());
			gameInstance()->UpdateAchievementProgress(FString(achievementId.c_str()), percent);
		}

		if (commandAndParams.size() >= 3 && commandAndParams[0] == TEXT("TestUpdateSteamStats"))
		{
			PUN_DEBUG2("TestUpdateSteamStats");
			std::wstring statName = commandAndParams[1];
			int32 statValue = FCString::Atoi(commandAndParams[2].c_str());
			gameInstance()->UpdateSteamStats(FString(statName.c_str()), statValue);
		}
		if (commandAndParams.size() >= 1 && commandAndParams[0] == TEXT("TestQuerySteamStats"))
		{
			PUN_DEBUG2("QuerySteamStats");
			gameInstance()->QuerySteamStats();
		}
		if (commandAndParams.size() >= 1 && commandAndParams[0] == TEXT("TestGetSteamStats"))
		{
			PUN_DEBUG2("GetSteamStats");
			gameInstance()->GetSteamStats();
		}
		if (commandAndParams.size() >= 1 && commandAndParams[0] == TEXT("TestResetSteamStats"))
		{
			PUN_DEBUG2("ResetSteamStats");
			gameInstance()->ResetSteamStats();
		}

		//if (commandAndParams.size() >= 2 && commandAndParams[0] == "WorkToCost")
		//{
		//	WorkRevenueToCost_Base = std::stoi(commandAndParams[1]);
		//}


		// Debug
		if (commandAndParams.size() >= 1)
		{
			auto addToggleCommand = [&](FString commandString) {
				std::wstring commandStdstr = ToWString(commandString);
				if (commandAndParams[0] == commandStdstr)
				{
					PunSettings::Toggle(commandString);

					networkInterface()->SendNetworkCommand(FSendChat::SystemMessage(commandString + " " + FString::FromInt(PunSettings::Get(commandString))));
					return;
				}
			};

			addToggleCommand("ForceClickthrough");
			addToggleCommand("ShowDebugExtra");
		}
		
		/*
		 * No param commands
		 */
		if (commandAndParams.size() == 1)
		{



		}
	}

public:
	/*
	 * PunLogBox
	 */

	UPROPERTY(meta = (BindWidget)) UOverlay* PunLogOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* PunLogBox;
	UPROPERTY(meta = (BindWidget)) UButton* PunLogCloseButton;

	void PunLog(std::string text)
	{
		auto widget = AddWidget<UPunRichText>(UIEnum::PunRichText);
		widget->SetText(text);
		PunLogBox->AddChild(widget);
	}

	UFUNCTION() void PunLogClose() {
		PunLogOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}


public:
	/*
	 * HiddenSettings
	 */
	UPROPERTY(meta = (BindWidget)) UOverlay* HiddenSettingsOverlay;
	UPROPERTY(meta = (BindWidget)) UScrollBox* HiddenSettingsBox;
	UPROPERTY(meta = (BindWidget)) UButton* HiddenSettingsResetSoundButton;
	UPROPERTY(meta = (BindWidget)) UButton* HiddenSettingsSaveButton;
	UPROPERTY(meta = (BindWidget)) UButton* HiddenSettingsLoadButton;
	UPROPERTY(meta = (BindWidget)) UButton* HiddenSettingsCloseButton;
	UPROPERTY(meta = (BindWidget)) UTextBlock* HiddenSettingsInfoText;
	UPROPERTY(meta = (BindWidget)) UComboBoxString* HiddenSettingsIsolateSoundDropdown;

	UPROPERTY(meta = (BindWidget)) UVerticalBox* HiddenSettingsGroupChooser;

	UFUNCTION() void HiddenSettingsClose() {
		HiddenSettingsOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	UFUNCTION() void HiddenSettingsSave() {
#if AUDIO_ALL
		dataSource()->soundSystem()->SaveOrLoadJSONFromFile(true);
#endif
	}
	UFUNCTION() void HiddenSettingsLoad() {
#if AUDIO_ALL
		dataSource()->soundSystem()->SaveOrLoadJSONFromFile(false);
		RefreshHiddenSettingsUI();
#endif
	}
	UFUNCTION() void HiddenSettingsResetSound() {
#if AUDIO_ALL
		dataSource()->soundSystem()->ResetSound();
#endif
	}

	UFUNCTION() void OnIsolateSoundDropdownChanged(FString sItem, ESelectInfo::Type seltype)
	{
		if (seltype == ESelectInfo::Type::Direct) {
			return;
		}

		PUN_LOG("OnDropDownChanged: %s ... %d", *sItem, (int)seltype);
		if (sItem.IsEmpty()) {
			return;
		}

#if AUDIO_ALL
		dataSource()->soundSystem()->isolateSound = ToStdString(sItem);
#endif
	}

	void RefreshHiddenSettingsUI();

	void UpdateHiddenSettingsDebugInfo()
	{
#if !AUDIO_ALL
		return;
#endif
		
		auto soundSystem = dataSource()->soundSystem();

		// ZoomHeight and Time update
		{
			for (auto& pair : GroupDebugRows) {
				std::string groupName = ToStdString(pair.Key);
				pair.Value->SetText(soundSystem->GetdebugStr(groupName));
			}

			std::stringstream ss;
			ss << std::fixed << std::setprecision(3);
			ss << "ZoomHeightRatio:" << soundSystem->zoomHeightFraction();
#if WITH_EDITOR
			ss << std::setprecision(0);
			ss << ", Zoom Distance(tiles):" << dataSource()->zoomDistance() / 10;
			ss << ", Zoom Step:" << GetZoomStepFromAmount(dataSource()->zoomDistance());
#endif
			ss << std::setprecision(2);
			ss << ", Time:" << UGameplayStatics::GetAudioTimeSeconds(this);
			SetText(HiddenSettingsInfoText, ss.str());
		}


		// PlayCount update
		if (SoundDebugRows.Num() > 0)
		{
			std::unordered_map<std::string, std::unordered_map<std::string, float>>& soundToSoundPropertyNameToValue = soundSystem->groupToSoundToSoundPropertyNameToValue[_chosenGroupName];
	
			for (auto& soundIt : soundToSoundPropertyNameToValue) {
				std::stringstream ss;
				ss << "     PlayCount: " << std::noshowpoint << soundSystem->GetSoundPropertyRef(_chosenGroupName, soundIt.first, "PlayCount");
				SoundDebugRows[ToFString(soundIt.first)]->SetText(ss.str());
			}
		}
	}

	void CallBack1(UPunWidget* punWidgetCaller, CallbackEnum callbackEnum) override
	{
		_chosenGroupName = _sortedGroupName[punWidgetCaller->callbackVar1];
		RefreshHiddenSettingsUI();
	}

private:
	UPunRichText* AddChatRow(const FSendChat& message)
	{
		auto widget = AddWidget<UPunRichText>(UIEnum::PunRichText_Chat);

		const int32 chatWrapSize = 260; // 270
		FSlateFontInfo fontInfo = FontExampleChat->Font;

		// System message
		if (message.isSystemMessage)
		{
			//std::string messageStd = ToStdString(message.message);
			//std::string wrappedMessage = widget->WrapString(messageStd, chatWrapSize, &fontInfo);
			//widget->SetRichText(ToFString(wrappedMessage));
			widget->SetRichText(message.message);
			widget->PunRichText->SetAutoWrapText(false);
			return widget;
		}

		
		FString playerNameF = networkInterface()->playerNameF(message.playerId);
		
		FString trimmedName = TrimStringF_Dots(playerNameF, 8);
		trimmedName.Append(": ");
		int32 namePartLength = trimmedName.Len();

		trimmedName.Append(message.message);
		//FString wrappedMessage = widget->WrapStringF(trimmedName, chatWrapSize, &fontInfo);

		FString finalMessage;
		finalMessage.Append("<ChatName>")
					.Append(trimmedName.Left(namePartLength))
					.Append("</>"); // Name
		finalMessage.Append(trimmedName.RightChop(namePartLength)); // Message

		widget->SetRichText(finalMessage);
		widget->PunRichText->SetAutoWrapText(false);
		
		return widget;
	};

private:
	UPROPERTY() TMap<FString, UPunRichText*> GroupDebugRows;
	UPROPERTY() TMap<FString, UPunRichText*> SoundDebugRows;
	
	std::string _chosenGroupName;
	std::vector<std::string> _sortedGroupName;

	//bool _isChatFocused = false;
	float _chatFadeOutStartTime = -1;
};
