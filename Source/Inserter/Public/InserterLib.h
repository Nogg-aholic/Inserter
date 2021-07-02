

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "Buildable/BuildableInserter.h"
#include "Buildable/BuildableInserterFilter.h"
#include "Buildables/FGBuildable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InserterLib.generated.h"

/**
 * 
 */
UCLASS()
class INSERTER_API UInserterLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()



	public:


	static UFGInventoryComponent* TryGetOutputInventory(AFGBuildable* Building);
	static UFGInventoryComponent* TryGetInputInventory(AFGBuildable* Building);
	static int32 CanGrabFromBelt(AFGBuildableConveyorBelt* Belt, ABuildableInserter* Inserter, FVector& Location, FConveyorBeltItem& Item);
	static bool GrabItem(UFGInventoryComponent* Inventory, const bool bOnlyCheck, AFGBuildable* BuildingTarget, ABuildableInserter* Inserter);

	UFUNCTION(BlueprintCallable)
	static UFGInventoryComponent* GetInventoryFromReplicationActor(UFGReplicationDetailInventoryComponent* Actor);
	UFUNCTION(BlueprintCallable)
	static FVector FindOffsetClosestToLocationForInserter(FVector Location, AFGBuildableConveyorBase* Belt);
	static int32 GetAllItemsByClass(UFGInventoryComponent* Inventory, FInventoryStack& Stack, int32 GrabStackSize);
};
