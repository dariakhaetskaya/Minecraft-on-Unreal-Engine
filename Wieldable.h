// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Wieldable.generated.h"

	UENUM()
		enum class ETool : uint8
	{
		Unarmed,
		Pickaxe,
		Axe,
		Shovel,
		Sword
	};

	UENUM()
		enum class EMaterial : uint8
	{
		None = 1,
		Wooden = 2,
		Stone = 4,
		Iron = 6,
		Diamond = 8,
		Golden = 12
	};

UCLASS()
class MCUE_API AWieldable : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWieldable();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		ETool ToolType;

	UPROPERTY(EditAnywhere)
		EMaterial MaterialType;

	UPROPERTY(EditAnywhere)
		USkeletalMeshComponent* WieldableMesh;
	
	UPROPERTY(EditAnywhere)
		UShapeComponent* PickupTrigger;

	UFUNCTION()
		void OnRadiusEnter(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditDefaultsOnly)
		UTexture2D* PickupThumbnail;

	bool bIsActive;

	void OnPickedUp();

	void OnUsed();

};
