// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCUEGameMode.h"
#include "MCUEHUD.h"
#include "MCUECharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "MCUECharacter.h"
#include "Kismet/GameplayStatics.h"
//#include <Runtime/Engine/Private/GameplayStatics.cpp>

void AMCUEGameMode::BeginPlay()
{
	ApplyHUDChanges();
}

void AMCUEGameMode::ApplyHUDChanges()
{
	// remove the previous hud since we are applying a new one 
	if (CurrentWidget != nullptr)
	{
		CurrentWidget->RemoveFromParent();
	}

	// check the hud state and apply the hud corresponding to whatever hud should be open
	switch(HUDState)
	{
		case EHUDState::HS_Ingame:
		{
			ApplyHUD(IngameHUDClass, false, false);
		}

		case EHUDState::HS_Inventory:
		{
			ApplyHUD(InventoryHUDClass, true, true);
		}

		case EHUDState::HS_Craft_Menu:
		{
			ApplyHUD(CraftMenuHUDClass, true, true);
		}

		default:
		{
			ApplyHUD(IngameHUDClass, false, false);
		}
	}
}

EHUDState AMCUEGameMode::GetHUDState()
{
	return EHUDState();
}

void AMCUEGameMode::ChangeHUDState(EHUDState NewState)
{
	HUDState = NewState;
	ApplyHUDChanges();
}

bool AMCUEGameMode::ApplyHUD(TSubclassOf<class UUserWidget> WidgetToApply, bool ShowMouseCursor, bool EnableClickEvents)
{
	// Get a ref to the player and the controller
	AMCUECharacter* MyCharacter = Cast<AMCUECharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	APlayerController* MyController = GetWorld()->GetFirstPlayerController();
	
	if (WidgetToApply != nullptr) 
	{
		MyController->bShowMouseCursor = ShowMouseCursor;
		MyController->bEnableClickEvents = EnableClickEvents;

		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetToApply);

		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
			return true;
		}
		else {
			return false;
		}
	} else{
		return false;
	}
}

AMCUEGameMode::AMCUEGameMode() 
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AMCUEHUD::StaticClass();
	HUDState = EHUDState::HS_Ingame;
}
