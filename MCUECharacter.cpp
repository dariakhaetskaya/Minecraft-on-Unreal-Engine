// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCUECharacter.h"
#include "MCUEProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AMCUECharacter

AMCUECharacter::AMCUECharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_WieldedItem = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_WieldedItem->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_WieldedItem->bCastDynamicShadow = false;
	FP_WieldedItem->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_WieldedItem->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_WieldedItem);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;

	Reach = 250.f;
}

void AMCUECharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_WieldedItem->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

void AMCUECharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckForBlocks();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMCUECharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	//PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AMCUECharacter::OnFire);

	// Enable touchscreen input
	if (EnableTouchscreenMovement(PlayerInputComponent) == false) {
		InputComponent->BindAction("Interact", IE_Pressed, this, &AMCUECharacter::OnHit);
		InputComponent->BindAction("Interact", IE_Released, this, &AMCUECharacter::EndHit);
	}

	//PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AMCUECharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AMCUECharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMCUECharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMCUECharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMCUECharacter::LookUpAtRate);

	InputComponent->BindAction("InventoryUp", IE_Pressed, this, &AMCUECharacter::MoveUpInventorySlot);
	InputComponent->BindAction("InventoryDown", IE_Pressed, this, &AMCUECharacter::MoveDownInventorySlot);
	InputComponent->BindAction("ExitGame", IE_Pressed, this, &AMCUECharacter::ExitGame);
}

bool AMCUECharacter::AddItemToInventory(AWieldable* Item)
{
	if (Item != NULL)
	{
		const int32 AvailableSlot = Inventory.Find(nullptr);

		if (AvailableSlot != INDEX_NONE)
		{
			Inventory[AvailableSlot] = Item;
			return true;
		}

		else return false;
	}

	return false;
}

int32 AMCUECharacter::GetCurrentInventorySlot()
{
	return CurrentInventorySlot;
}

UTexture2D* AMCUECharacter::GetThumbnailAtInventorySlot(uint8 Slot)
{
	if (Inventory[Slot] != NULL)
	{
		return Inventory[Slot]->PickupThumbnail;
	}
	else return nullptr;
}

void AMCUECharacter::OnFire()
{

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AMCUECharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMCUECharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AMCUECharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AMCUECharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMCUECharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMCUECharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMCUECharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AMCUECharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMCUECharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AMCUECharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AMCUECharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

void AMCUECharacter::MoveUpInventorySlot()
{
	CurrentInventorySlot = FMath::Abs((CurrentInventorySlot + 1) % NUM_OF_INVENTORY_SLOTS);
}

void AMCUECharacter::MoveDownInventorySlot()
{
	if (CurrentInventorySlot == 0)
	{
		CurrentInventorySlot = 9;
		return;
	}

	CurrentInventorySlot = FMath::Abs((CurrentInventorySlot - 1) % NUM_OF_INVENTORY_SLOTS);
}

void AMCUECharacter::OnHit()
{
	PlayHitAnim();

	if (CurrentBlock != nullptr) {
		bIsBreaking = true;

		float TimeBetweenBreaks = ((CurrentBlock->Resistance) / 100.f) / 2;

		GetWorld()->GetTimerManager().SetTimer(BlockBreakingHandle, this, &AMCUECharacter::BreakBlock, TimeBetweenBreaks, true);
		GetWorld()->GetTimerManager().SetTimer(HitAnimHandle, this, &AMCUECharacter::PlayHitAnim, 0.4f, true);
	}
}

void AMCUECharacter::EndHit()
{
	GetWorld()->GetTimerManager().ClearTimer(BlockBreakingHandle);
	GetWorld()->GetTimerManager().ClearTimer(HitAnimHandle);

	bIsBreaking = false;

}

void AMCUECharacter::PlayHitAnim()
{
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AMCUECharacter::CheckForBlocks()
{
	FHitResult LinetraceHit;

	FVector StartTrace = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndTrace = (FirstPersonCameraComponent->GetForwardVector() * Reach) + StartTrace;

	FCollisionQueryParams CQP;
	CQP.AddIgnoredActor(this); // so the raycast ignores the player

	GetWorld()->LineTraceSingleByChannel(LinetraceHit, StartTrace, EndTrace, ECollisionChannel::ECC_WorldDynamic, CQP);

	ABlock* PotentialBlock = Cast<ABlock>(LinetraceHit.GetActor());

	if (PotentialBlock != CurrentBlock && CurrentBlock != nullptr)
	{
		CurrentBlock->ResetBlock();
	}

	if (PotentialBlock == NULL) {
		CurrentBlock = nullptr;
		return;
	}
	else {
		if (CurrentBlock != nullptr && !bIsBreaking)
		{
			CurrentBlock->ResetBlock();
		}
		CurrentBlock = PotentialBlock;
	}

}

void AMCUECharacter::BreakBlock()
{
	if (bIsBreaking && CurrentBlock != nullptr && !CurrentBlock->IsPendingKill()) {
		CurrentBlock->Break();
	}
}

void AMCUECharacter::ExitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, 0);
}

void AMCUECharacter::SetTool(ETool NewToolType)
{
	ToolType = NewToolType;
}

void AMCUECharacter::SetMaterial(EMaterial NewMaterialType)
{
	MaterialType = NewMaterialType;
}

EMaterial AMCUECharacter::GetMaterial()
{
	return MaterialType;
}

ETool AMCUECharacter::GetTool()
{
	return ToolType;
}
