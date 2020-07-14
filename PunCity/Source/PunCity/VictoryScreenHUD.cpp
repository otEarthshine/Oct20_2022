// Pun Dumnernchanvanit's


#include "VictoryScreenHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/AudioComponent.h"
#include "PunCity/PunGameInstance.h"

AVictoryScreenHUD::AVictoryScreenHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> finder(TEXT("/Game/UI/VictoryScreen"));
	check(finder.Class);
	_victoryScreenClass = finder.Class;
}

void AVictoryScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	PUN_DEBUG2("PunLobbyHUD BeginPlay");

	_victoryScreen = Cast<UVictoryScreen>(CreateWidget<UUserWidget>(GetWorld(), _victoryScreenClass));
	check(_victoryScreen);
	_victoryScreen->AddToViewport();
	_victoryScreen->Init();
	_victoryScreen->SetHUD(this, UIEnum::None);
}


void AVictoryScreenHUD::Tick(float DeltaTime)
{
	_victoryScreen->Tick();
}