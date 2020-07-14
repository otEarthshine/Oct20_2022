// Pun Dumnernchanvanit's


#include "PunLoadingScreenHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "MainMenuUI.h"

APunLoadingScreenHUD::APunLoadingScreenHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> uiAsset(TEXT("/Game/UI/MainMenu/LoadingScreenWidget"));
	check(uiAsset.Class);
	_uiClass = uiAsset.Class;
}

void APunLoadingScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	_loadingScreenUI = Cast<UMainMenuUI>(CreateWidget<UUserWidget>(GetWorld(), _uiClass));
	check(_loadingScreenUI);
	_loadingScreenUI->AddToViewport();
	_loadingScreenUI->Init();
}
