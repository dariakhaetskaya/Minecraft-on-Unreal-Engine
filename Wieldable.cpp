// Fill out your copyright notice in the Description page of Project Settings.


#include "Wieldable.h"
#include "MCUECharacter.h"
#include "Runtime/Engine/Classes/Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWieldable::AWieldable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WieldableMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WieldableMesh"));

	PickupTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PickTrigger"));

	PickupTrigger->SetGenerateOverlapEvents(true);
	PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWieldable::OnRadiusEnter);
	PickupTrigger->SetupAttachment(WieldableMesh);
	MaterialType = EMaterial::None;
	ToolType = ETool::Unarmed;

	bIsActive = true;

}

// Called when the game starts or when spawned
void AWieldable::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWieldable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator rotation = WieldableMesh->GetComponentRotation();
	rotation.Yaw += 1.f;
	WieldableMesh->SetRelativeRotation(rotation);
	   
}

void AWieldable::OnRadiusEnter(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsActive)
	{
		AMCUECharacter* Character = Cast<AMCUECharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		Character->FP_WieldedItem->SetSkeletalMesh(WieldableMesh->SkeletalMesh);
		Character->AddItemToInventory(this);
		OnPickedUp();
	}
}

void AWieldable::OnPickedUp()
{
	WieldableMesh->SetVisibility(false);
	bIsActive = false;
}

void AWieldable::OnUsed()
{
	Destroy();
}
 
