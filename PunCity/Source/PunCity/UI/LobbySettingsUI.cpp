// Pun Dumnernchanvanit's


#include "LobbySettingsUI.h"
#include "PunCity/MainMenuPlayerController.h"

AMainMenuPlayerController* ULobbySettingsUI::GetFirstController()
{
	auto firstController = Cast<AMainMenuPlayerController>(GetWorld()->GetFirstPlayerController());
	check(UGameplayStatics::GetPlayerControllerID(firstController) == 0);
	return firstController;
}

bool ULobbySettingsUI::isServer()
{
	return GetFirstController()->GetLocalRole() == ROLE_Authority;
}

void ULobbySettingsUI::SendMapSettings()
{
	if (_isPreLobby) {
		return;
	}
	
	auto firstController = GetFirstController();
	if (firstController->GetLocalRole() == ROLE_Authority) {
		PunSerializedData blob(true);
		serverMapSettings.Serialize(blob);
		firstController->ExecuteAllControllers([&](AMainMenuPlayerController* controller) {
			controller->SetMapSettings(blob);
		});
	}
}

void ULobbySettingsUI::OnLobbyMapSeedInputBoxTextCommitted(const FText& text, ETextCommit::Type CommitMethod)
{
	PUN_DEBUG2("OnLobbyMapSeedInputBoxTextCommitted %s", *text.ToString());
	serverMapSettings.mapSeed = text.ToString();

	SendMapSettings();

	//GetFirstController()->ExecuteAllControllers([&](AMainMenuPlayerController* controller) {
	//	controller->SetMapSeed(text.ToString());
	//});
}

void ULobbySettingsUI::OnLobbyMapSizeDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.mapSizeEnumInt = static_cast<int32>(GetMapSizeEnumFromString(sItem));
	RefreshAICountDropdown();

	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}
void ULobbySettingsUI::OnLobbySeaLevelDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.mapSeaLevel = GetEnumFromName<MapSeaLevelEnum>(sItem, MapSettingsLevelNames);

	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}
void ULobbySettingsUI::OnLobbyMoistureDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.mapMoisture = GetEnumFromName<MapMoistureEnum>(sItem, MapMoistureNames);

	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}
void ULobbySettingsUI::OnLobbyTemperatureDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.mapTemperature = GetEnumFromName<MapTemperatureEnum>(sItem, MapSettingsLevelNames);

	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}
void ULobbySettingsUI::OnLobbyMountainDensityDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.mapMountainDensity = GetEnumFromName<MapMountainDensityEnum>(sItem, MapSettingsLevelNames);

	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}


void ULobbySettingsUI::OnLobbyAICountDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.aiCount = FCString::Atoi(*sItem);
	SendMapSettings();

	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}
void ULobbySettingsUI::OnLobbyDifficultyDropdownChanged(FString sItem, ESelectInfo::Type seltype)
{
	serverMapSettings.difficultyLevel = GetDifficultyLevelFromString(sItem);
	SendMapSettings();


	if (seltype != ESelectInfo::Type::Direct) {
		gameInstance()->Spawn2DSound("UI", "DropdownChange");
	}
}