

#pragma once

#include "CoreMinimal.h"
#include "Buildable/BuildableInserter.h"
#include "Animation/AnimInstance.h"
#include "InserterAnim.generated.h"

/**
 * 
 */

class UInserterAnim;
USTRUCT(Blueprintable)
struct INSERTER_API FInserterAnimation
{
	GENERATED_BODY()

public:
	FInserterAnimation();
	FInserterAnimation(UInserterAnim * Building, AFGBuildable * BuildingTarget);

	// Starting Location we Lerp From
	UPROPERTY(BlueprintReadonly)
	FVector StartItemLocation;
	// Float Z Rotation Angle Start
	float StartRotation;
	
	// Target Location we Lerp To
	UPROPERTY(BlueprintReadonly)
	FVector TargetLocation;
	// Float Z Rotation Angle Target
	float LookAtRotation;
	FVector RestingLocation;


};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginMoveSound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndMoveSound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayGrabSound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndGrabSound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAnimOnItemPickup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAnimOnItemDrop);

UCLASS()
class INSERTER_API UInserterAnim : public UAnimInstance
{
	GENERATED_BODY()

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	// SOUND BEGIN

	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnBeginMoveAnim")
		FOnBeginMoveSound OnBeginMoveSound;

	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnEndMoveAnim")
		FOnEndMoveSound OnEndMoveSound;


	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnStartGrabSound")
		FOnPlayGrabSound OnStartGrabSound;

	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnEndGrabAnim")
		FOnEndGrabSound OnEndGrabSound;

	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnAnimItemPickup")
		FAnimOnItemPickup OnAnimItemPickup;
	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnAnimItemDrop")
		FAnimOnItemDrop OnAnimItemDrop;
	// SOUND END
	
	UFUNCTION(BlueprintCallable)
		void OnPreGrabEvent(AFGBuildable * GrabBuilding);
	UFUNCTION(BlueprintCallable)
		void OnDrop(AFGBuildable * GrabBuilding);
	UFUNCTION(BlueprintCallable)
		void OnAnimFinished();
	UFUNCTION(BlueprintCallable)
		void OnPreDropEvent(AFGBuildable * GrabBuilding);
	UFUNCTION(BlueprintCallable)
		void OnGrab(AFGBuildable * GrabBuilding);
	
	UFUNCTION()
		float netFunc_SetBaseRotation(const float NewValue) { BaseRotation = NewValue; return BaseRotation; };
	UFUNCTION()
		bool netFunc_SetHeadLocation(const float X, const float Y, const float Z) { HeadLocation.X = X; HeadLocation.Y = Y; HeadLocation.Z = Z; return true; };
	UFUNCTION()
		bool netFunc_SetHeadRotation(const float Roll, const float Pitch, const float Yaw) { HeadRotation.Roll = Roll; HeadRotation.Pitch = Pitch; HeadRotation.Yaw = Yaw; return true; };
	UFUNCTION()
		int32 netFunc_SetAnimState(int32 NewValue) { AnimState = static_cast<ESplitterAnimState>(NewValue); return AnimState; };

public:
	UPROPERTY(BlueprintReadWrite)
		float BaseRotation = 0.f;
	// IK Bone Positions (X=150.000000,Y=0.000000,Z=100.000000)
	UPROPERTY(BlueprintReadWrite)
		FVector HeadLocation = FVector();
	// Inserter Head Rotation
	UPROPERTY(BlueprintReadWrite)
		FRotator HeadRotation = FRotator();

	UPROPERTY(BlueprintReadOnly)
		FInserterAnimation Anim = FInserterAnimation();
	
	UPROPERTY(BlueprintReadWrite)
		bool HasPower = false;


	UPROPERTY()
	ABuildableInserter* nOwner = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
		TEnumAsByte< ESplitterAnimState> AnimState = ESplitterAnimState::EInserter_Anim_NONE;

};
