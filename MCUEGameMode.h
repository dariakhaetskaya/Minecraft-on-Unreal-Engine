// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MCUEGameMode.generated.h"

UENUM()
enum class EHUDState : uint8
{
	HS_Ingame,
	HS_Inventory,
	HS_Craft_Menu
};

UCLASS(minimalapi)
class AMCUEGameMode : public AGameModeBase
{
	GENERATED_BODY()

	virtual void BeginPlay() override;

	//checks the hud state and then calls applyhud to apply whatever hud we're using
	void ApplyHUDChanges();

	// hud state getter
	EHUDState GetHUDState();

	UFUNCTION(BlueprintCallable, Category = "HUD Functions")
	void ChangeHUDState(EHUDState NewState);

	// applies the hud to the screen. true if successful, false otherwise
	bool ApplyHUD(TSubclassOf<class UUserWidget> WidgetToApply, bool ShowMouseCursor, bool EnableClickEvents);

public:
	AMCUEGameMode();

protected:
	// the current hudstate
	EHUDState HUDState;

	// the hud to be shown in game
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blueptint Widgets", Meta = (BlueprintProtected = "true"))
		TSubclassOf<class UUserWidget> IngameHUDClass;

	// the hud to be shown in the inventory
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blueptint Widgets", Meta = (BlueprintProtected = "true"))
		TSubclassOf<class UUserWidget> InventoryHUDClass;

	// the hud to be shown in the crafting menu
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Blueptint Widgets", Meta = (BlueprintProtected = "true"))
		TSubclassOf<class UUserWidget> CraftMenuHUDClass;

	// the current hud being display on the screen
	class UUserWidget* CurrentWidget;
};



