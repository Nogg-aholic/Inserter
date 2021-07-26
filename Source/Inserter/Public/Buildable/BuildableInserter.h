#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableConveyorAttachment.h"
#include "Buildables/FGBuildableConveyorBase.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "../Engine/Classes/Kismet/KismetMathLibrary.h"
#include "FGInventoryLibrary.h"

#include "BuildableInserter.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum EInserterActionState
{
	EInserter_NoState UMETA(description = "Null"),
	EInserter_Ready UMETA(description = "Ready for a Pickup"),
	EInserter_Moving UMETA(description = "Either in Drop or Pickup Movement"),
	EInserter_Waiting UMETA(description = "Either waiing for Drop or Pickup"),
	EInserter_DropOrPickup UMETA(description = "Either in Drop or Pickup Action"),
	EInserter_Recovering UMETA(description = "Either in Recovering from Drop or Pickup Movement"),
	EInserter_MAX UMETA(description = "Invalid"),

};

UENUM(BlueprintType)
enum ESplitterAnimState
{
	EInserter_Anim_NONE,
	EInserter_Anim_Rotate,
	EInserter_Anim_Move,
	EInserter_Anim_Recover,
};


UENUM(BlueprintType)
enum EInserterEvent
{
	EInserter_BeforeGrab,
    EInserter_BeforeDrop,
    EInserter_Drop,
    EInserter_Pickup,
	EInserter_Finished,
};

UENUM(BlueprintType)
enum EInserterInventoryIndex
{
	EInserter_ConveyorIn,
	EInserter_HoldingItem,
	EInserter_ConveyorOut,
	EInserter_DroppedItem,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInserterEvent, EInserterEvent, Event, int32, Index);

UCLASS()
class INSERTER_API ABuildableInserter : public AFGBuildableConveyorAttachment
{
	GENERATED_BODY()
public:
	ABuildableInserter();
	virtual void BeginPlay() override;
protected:
	friend class UInserterLib;
	friend class UInserterAnim;
	friend struct FInserterAnimation;
	friend class AInserterSubsystem;
	// Overrides 
	virtual void Factory_CollectInput_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float CalcProductionCycleTimeForPotential(float potential) const override;
	virtual void OnRep_ReplicationDetailActor() override;
	virtual void OnBuildableReplicationDetailStateChange(bool newStateIsActive) override;
	virtual class AFGReplicationDetailActor* GetOrCreateReplicationDetailActor() override;
	virtual UClass* GetReplicationDetailActorClass() const override { return AFGReplicationDetailActor_BuildableFactory::StaticClass(); };

	virtual void InserterTick(float dt);

	UFUNCTION(BlueprintCallable)
	void StartTimeForState(EInserterActionState State);
	bool StoreGrabbedItemForConveyor(const FInventoryStack& Stack, bool bOnlyCheck);
	void RestoreFromFailedAction();

	virtual bool PlaceItem(UFGInventoryComponent * Inventory, bool bOnlyCheck, AFGBuildable * BuildingTarget);
	
	int32 RemoveItemsFromInventory(UFGInventoryComponent* Inventory, FInventoryStack& Stack);

	virtual bool GrabItem(UFGInventoryComponent* Inventory, bool bOnlyCheck, AFGBuildable* BuildingTarget);

	bool GrabFromBuilding(AFGBuildable * BuildingTarget, bool bOnlyCheck);

	virtual bool Filter_ShouldIgnore(TSubclassOf<class UFGItemDescriptor> inClass, AFGBuildable * BuildingTarget) { return false; }

	FInventoryItem GrabFromOffset(AFGBuildableConveyorBelt * Belt, bool bOnlyCheck);

	/*
				Please Override
	*/

	virtual int8 GetPickupTarget();

	virtual bool CanExecutePickup();
	
	virtual int32 GetDropTarget();

	virtual bool CanExecuteDrop();

	UFUNCTION(BlueprintPure)
    FVector GetBeltLocation() const;
	// Called on Remotes to replicate the Position Vector for Belt Grabs
	UFUNCTION(BlueprintCallable)
    void SetBeltLocation(const FVector New);
	
public:
	UFUNCTION(BlueprintPure)
	UFGInventoryComponent* GetInventory() const;

	UFUNCTION(BlueprintPure)
	static TSubclassOf<class UFGRecipe> GetBuiltWithRecipe(AFGBuildable* Building);

	UFUNCTION(BlueprintPure)
	bool IsHoldingItem() const;
	
	// Tries to add an Input to the Inserter, this will fail if the Building is further than doubles its Range
	// this can be slightly abused but a collision trace for the Buildings Edge isnt implemented for now.
	UFUNCTION(BlueprintPure)
		bool netFunc_addInput(AFGBuildable * BuildingTarget);
	// Tries to add an Output to the Inserter, this will fail if the Building is further than doubles its Range
	// "
	UFUNCTION(BlueprintPure)
		bool netFunc_addOutput(AFGBuildableFactory * BuildingTarget);

	// Set the Target of the Inserter directly ( this is overwritten Automatically while Automatic = true )
	UFUNCTION(BlueprintPure)
		bool netFunc_setSelectedInput(AFGBuildable * BuildingTarget);
	UFUNCTION(BlueprintPure)
		bool netFunc_setSelectedOutput(AFGBuildableFactory * BuildingTarget);

	
	UPROPERTY(BlueprintReadWrite,savegame, Replicated)
	bool netProp_Enabled = true;
	
	// If you want to Set Selected In and Outputs manually this must be false
	UPROPERTY(BlueprintReadWrite, savegame, Replicated)
	bool netProp_Automatic = true;

	UPROPERTY(BlueprintReadWrite, savegame, Replicated)
	bool netProp_UseAnimation = true;

	UPROPERTY(savegame, BlueprintReadWrite)
	TEnumAsByte<EInserterActionState>  InserterState = EInserterActionState::EInserter_NoState;

	UPROPERTY(Replicated, savegame, BlueprintReadWrite)
	TArray< AFGBuildable * > InputsInRange;
	UPROPERTY(Replicated, savegame, BlueprintReadWrite)
	TArray< AFGBuildableFactory * > OutputsInRange;

	UPROPERTY(BlueprintReadWrite)
	int32 SelectedInput = -1;
	UPROPERTY(BlueprintReadWrite)
	int32 SelectedOutput = -1;

	UPROPERTY(BlueprintReadOnly, savegame)
	FItemAmount GrabbedItem = FItemAmount();

private:
	FVector BeltTargetLocation = FVector(0,0,0);
	float Movement_Progress = 0.f;
	float Waiting = 0.f;
	FThreadSafeBool bInputSwap;

protected:

	
	UPROPERTY(BlueprintAssignable, Category = "Animation", DisplayName = "OnInserterEvent")
	FOnInserterEvent OnInserterEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 GrabStackSize = 1;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 BufferSize = 10;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float GrabTime = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Range = 900;

	// The Position the Inserter goes to for Rotations
	UPROPERTY(BlueprintReadWrite)
	FTransform RestingTransform = FTransform();

};



