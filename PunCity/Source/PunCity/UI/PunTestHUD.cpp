// Pun Dumnernchanvanit's


#include "PunTestHUD.h"
#include "MainMenuUI.h"

APunTestHUD::APunTestHUD()
{
	//static ConstructorHelpers::FClassFinder<UUserWidget> mainMenuUIAsset(TEXT("/Game/UI/MainMenu/MainMenuUIWidgetTest"));
	//check(mainMenuUIAsset.Class);
	//_mainMenuClass = mainMenuUIAsset.Class;
}

void APunTestHUD::BeginPlay()
{
	Super::BeginPlay();

	//_mainMenuUI = Cast<UMainMenuUI>(CreateWidget<UUserWidget>(GetWorld(), _mainMenuClass));
	//check(_mainMenuUI);
	//_mainMenuUI->AddToViewport();
	//_mainMenuUI->Init();
}