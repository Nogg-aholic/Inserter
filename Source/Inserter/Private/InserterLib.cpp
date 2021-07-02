


#include "InserterLib.h"



#include "Buildable/BuildableInserterFilter.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableGeneratorNuclear.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "Buildables/FGBuildableStorage.h"

UFGInventoryComponent* UInserterLib::TryGetOutputInventory(AFGBuildable* Building)
{
	if (!Building)
		return nullptr;

	if (const AFGBuildableManufacturer * Build = Cast< AFGBuildableManufacturer>(Building))
	{
		return Build->GetOutputInventory();
	}
	else if (AFGBuildableStorage * BuildStorage = Cast< AFGBuildableStorage>(Building))
	{
		return BuildStorage->GetStorageInventory();
	}
	else if (const AFGBuildableResourceExtractor * BuildExtractor = Cast< AFGBuildableResourceExtractor>(Building))
	{
		return BuildExtractor->GetOutputInventory();
	}
	else if (const AFGBuildableGeneratorNuclear * BuildGenN = Cast< AFGBuildableGeneratorNuclear>(Building))
	{
		return BuildGenN->GetWasteInventoryAccessor();
	}
	else if (const AFGBuildableGeneratorFuel * BuildGen = Cast< AFGBuildableGeneratorFuel>(Building))
	{
		return BuildGen->GetFuelInventory();
	}
	else if (const AFGBuildableFactory * BuildableFactory = Cast<AFGBuildableFactory>(Building))
	{
		TArray<UFGInventoryComponent*> InventoryComponents ; Building->GetComponents(InventoryComponents);
		
		if (InventoryComponents.Contains(BuildableFactory->GetPotentialInventory()))
		{
			InventoryComponents.Remove(BuildableFactory->GetPotentialInventory());
		}
		
		if (!InventoryComponents.IsValidIndex(0))
			return nullptr;
		
		if (!InventoryComponents.IsValidIndex(1))
		{
			if (InventoryComponents.IsValidIndex(0))
				return InventoryComponents[0];
			else
				return nullptr;
		}
		return InventoryComponents[1];
	}
	return nullptr;
}

UFGInventoryComponent* UInserterLib::TryGetInputInventory(AFGBuildable* Building)
{
	if (AFGBuildableManufacturer * Manufacturer = Cast< AFGBuildableManufacturer>(Building))
	{
		return Manufacturer->GetInputInventory();
	}
	if (AFGBuildableStorage * Storage = Cast< AFGBuildableStorage>(Building))
	{
		return Storage->GetStorageInventory();
	}
	else if(AFGBuildableGeneratorFuel* Generator = Cast<AFGBuildableGeneratorFuel>(Building))
	{
		return Generator->GetFuelInventory();
	}
	else if (const AFGBuildableFactory * BuildableFactory = Cast<AFGBuildableFactory>(Building))
	{
		TArray<UFGInventoryComponent*> InventoryComponents ; Building->GetComponents(InventoryComponents);
		
		if (InventoryComponents.Contains(BuildableFactory->GetPotentialInventory()))
		{
			InventoryComponents.Remove(BuildableFactory->GetPotentialInventory());
		}
		
		if (!InventoryComponents.IsValidIndex(0))
			return nullptr;
		
		return InventoryComponents[0];
	}

	return nullptr;
}


int32 UInserterLib::CanGrabFromBelt(AFGBuildableConveyorBelt* Belt, ABuildableInserter* Inserter, FVector & Location, FConveyorBeltItem& Item)
{
	if (!Belt || !Inserter || Belt->mItems.Num() == 0)
		return -1;

	const int32 Index = Belt->FindItemClosestToLocation(Inserter->GetActorLocation());
	if (Index >= 0 && Belt->mItems.IsValidIndex(Index))
	{
		Item = Belt->Factory_PeekItemAt(Index);
		if (!Item.Item.IsValid())
			return -1;
		if (Inserter->Filter_ShouldIgnore(Item.Item.ItemClass, Belt))
			return -1;

		FVector Rot;
		Belt->GetLocationAndDirectionAtOffset(Item.Offset, Location, Rot);
		if (FVector::Dist2D(Inserter->GetActorLocation(), Location) > Inserter->Range)
			return -1;
		else
			return Index;
	}
	return -1;
}

UFGInventoryComponent * UInserterLib::GetInventoryFromReplicationActor(UFGReplicationDetailInventoryComponent* Actor)
{
	if(!Actor)
		return nullptr;
	
	return Actor->GetActiveInventoryComponent();
}

FVector UInserterLib::FindOffsetClosestToLocationForInserter(FVector Location, AFGBuildableConveyorBase* Belt)
{
	if (Belt)
	{
		FVector Out_Location;
		FVector Out_Direction;
		Belt->GetLocationAndDirectionAtOffset(Belt->FindOffsetClosestToLocation(Location), Out_Location, Out_Direction);
		return Out_Location;
	}
	else
		return FVector();
}

int32 UInserterLib::GetAllItemsByClass(UFGInventoryComponent* Inventory, FInventoryStack & Stack, int32 GrabStackSize)
{
	const TSubclassOf<class UFGItemDescriptor> ItemClass = Stack.Item.ItemClass;
	if (Stack.NumItems > GrabStackSize)
	{
		Inventory->Remove(ItemClass, GrabStackSize);
		return GrabStackSize;
	}
	else
	{
		if (Inventory->HasItems(ItemClass,GrabStackSize))
		{
			Inventory->Remove(ItemClass, GrabStackSize);
			return GrabStackSize;
		}
		else
		{
			const int32 GrabAmount = FMath::Clamp(GrabStackSize, 0, Inventory->GetNumItems(ItemClass));
			Inventory->Remove(ItemClass, GrabAmount);
			return GrabAmount;
		}
	}
}
