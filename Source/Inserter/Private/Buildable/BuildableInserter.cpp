
#include "Buildable/BuildableInserter.h"
#include "FactoryGame.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "FGFactoryConnectionComponent.h"
#include "InserterLib.h"
#include "InserterSubsystem.h"
#include "Buildables/FGBuildableConveyorBelt.h"


ABuildableInserter::ABuildableInserter()
{
}

void ABuildableInserter::Factory_CollectInput_Implementation()
{
	if (!mInputs.IsValidIndex(0) || bInputSwap || !HasAuthority())
		return;

	if (OutputsInRange.Contains(this) && mBufferInventory->IsSomethingOnIndex(EInserter_DroppedItem))
	{

		if (!mBufferInventory->IsSomethingOnIndex(EInserter_ConveyorOut))
		{
			FInventoryStack DropItem; mBufferInventory->GetStackFromIndex(EInserter_DroppedItem, DropItem);
			mBufferInventory->RemoveFromIndex(EInserter_DroppedItem,mBufferInventory->AddStackToIndex(EInserter_ConveyorOut, DropItem, true));
		}
	}

	FInventoryStack Stack; mBufferInventory->GetStackFromIndex(EInserter_ConveyorIn, Stack);


	if (Stack.HasItems())
	{
		if (!InputsInRange.Contains(this))
		{
			if (!mBufferInventory->IsSomethingOnIndex(EInserter_ConveyorOut))
			{
				Stack.NumItems = 1;
				mBufferInventory->AddStackToIndex(EInserter_ConveyorOut, Stack, true);
				mBufferInventory->RemoveFromIndex(EInserter_ConveyorIn, 1);
			}
		}
		else
		{
			if (Stack.NumItems >= BufferSize)
			{
				if (!mBufferInventory->IsSomethingOnIndex(EInserter_ConveyorOut))
				{
					Stack.NumItems = 1;
					mBufferInventory->AddStackToIndex(EInserter_ConveyorOut, Stack, true);
					mBufferInventory->RemoveFromIndex(EInserter_ConveyorIn, 1);
				}
			}
		}
	}

	if (mInputs[0]->IsConnected() && Stack.NumItems < BufferSize)
	{
		FInventoryItem Out; float Offset;
		if (mInputs[0]->Factory_GrabOutput(Out, Offset, Stack.HasItems() ? Stack.Item.ItemClass : nullptr))
		{
			mBufferInventory->AddStackToIndex(EInserter_ConveyorIn, FInventoryStack(Out), false);
		}
	}
}


void ABuildableInserter::BeginPlay()
{
	Super::BeginPlay();
	BeltTargetLocation = GetActorLocation();
	// Both Server and Remote have their separate Update Arrays
	AInserterSubsystem::Get(this->GetWorld())->RegisterInserter(this);
	
	if (!HasAuthority())
		return;

	if(mBufferInventory)
	{
		if (mBufferInventory->GetIsReplicated())
		{
			UE_LOG(LogTemp, Error, TEXT("WAS REPLICATING"));
			mBufferInventory->SetIsReplicated(false);
		}
		if (mBufferInventory->GetSizeLinear() != 4)
		{
			mBufferInventory->Resize(4);
		}
		// Inventory Slot Used for Connections will be 0 
		FOR_EACH_ORDERED_FACTORY_INLINE_COMPONENTS(con)
		{

			if (con->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
			{
				mInputs.Add(con);
				con->SetInventory(mBufferInventory);
				con->SetInventoryAccessIndex(EInserter_ConveyorIn);
			}
			else
			{
				mOutputs.Add(con);
				con->SetInventory(mBufferInventory);
				con->SetInventoryAccessIndex(EInserter_ConveyorOut);
			}
		}
		
		if (mBufferInventory)
		{
			mBufferInventory->AddArbitrarySlotSize(EInserter_ConveyorIn, BufferSize+1);
			mBufferInventory->AddArbitrarySlotSize(EInserter_HoldingItem, GrabStackSize);
			mBufferInventory->AddArbitrarySlotSize(EInserter_ConveyorOut, GrabStackSize);
			mBufferInventory->AddArbitrarySlotSize(EInserter_DroppedItem, GrabStackSize);
		}
	}
}

void ABuildableInserter::InserterTick(float dt)
{
	if (!netProp_Enabled)
		return;

	const bool bHost = HasAuthority();
	switch (InserterState)
	{
	case EInserterActionState::EInserter_Ready: {
		if (!bHost)
		{
			Movement_Progress = 0;
			break;
		}

		if (mPendingPotential != mCurrentPotential)
		{
			mCurrentPotential = mPendingPotential;
			
		}
		if (IsHoldingItem())
		{
			if (netProp_Automatic)
			{
				SelectedOutput = GetDropTarget();
			}
			// if we have a valid Output we have to Move
			if (OutputsInRange.IsValidIndex(SelectedOutput))
			{
				if (netProp_UseAnimation)
					OnInserterEvent.Broadcast(EInserterEvent::EInserter_BeforeDrop, SelectedOutput);
				
				StartTimeForState(EInserterActionState::EInserter_Moving);
			}
		}
		else
		{
			if (netProp_Automatic)
			{
				SelectedInput = GetPickupTarget();
			}
			if (InputsInRange.IsValidIndex(SelectedInput))
			{
				if (netProp_UseAnimation)
					OnInserterEvent.Broadcast(EInserterEvent::EInserter_BeforeGrab, SelectedInput);
				
				StartTimeForState(EInserterActionState::EInserter_Moving);
			}
		}
		break;
	}
	case EInserterActionState::EInserter_Moving: {
		// Moving is Rotation and Moving and occupies 3/4 of the Time.
		// Added Delta needs to be adjusted in this case
		const float AddedDt = (1.33333f)* (1 / (GrabTime/ mCurrentPotential));
		Movement_Progress = FMath::Clamp(Movement_Progress + (AddedDt*dt), 0.f, 1.f);
		// if we are done we go into Waiting Mode
		if (Movement_Progress >= 1)
		{
			StartTimeForState(EInserterActionState::EInserter_Waiting);
		}
		break;
	}
	case EInserterActionState::EInserter_Waiting: {
		if (!bHost)
		{
			Movement_Progress = 0;
			break;
		}
		// if we are actually waiting for longer than 5 Seconds we notify the Anim to Recover
		// we do not have to do this if we arent waiting because Ready State will take care of this
		// ( we need Recovery-Time between Actions )	
		if (IsHoldingItem())
		{
			if (CanExecuteDrop() || Waiting > 5.f)
			{
				if (netProp_UseAnimation)
					OnInserterEvent.Broadcast(EInserterEvent::EInserter_Drop,SelectedOutput);
				
				Waiting = 0.f;
				StartTimeForState(EInserterActionState::EInserter_Recovering);
			}
			else
			{
				Waiting += dt;
			}
		}
		else
		{
			if (CanExecutePickup() || Waiting > 5.f)
			{
				if (netProp_UseAnimation)
					OnInserterEvent.Broadcast(EInserterEvent::EInserter_Pickup,SelectedInput);
				
				Waiting = 0.f;
				StartTimeForState(EInserterActionState::EInserter_Recovering);
			}
			else
			{
				Waiting += dt;
			}
		}
		break;
	}
	case EInserterActionState::EInserter_Recovering: {
		// Recovery is 1/4 of the Grab Time
		const float AddedDt = 3 * (1/(GrabTime/ mCurrentPotential));
		Movement_Progress = FMath::Clamp(Movement_Progress + (AddedDt*dt), 0.f, 1.f);
		if (Movement_Progress >= 1)
		{
			if (netProp_UseAnimation && bHost)
				OnInserterEvent.Broadcast(EInserterEvent::EInserter_Finished,-1);
			StartTimeForState(EInserterActionState::EInserter_Ready);
		}
		break;
	}
	default: break;
	}
}
float ABuildableInserter::CalcProductionCycleTimeForPotential(float potential) const
{
	return (GrabTime / mCurrentPotential);
}

void ABuildableInserter::OnRep_ReplicationDetailActor()
{
}

void ABuildableInserter::OnBuildableReplicationDetailStateChange(bool newStateIsActive)
{
	return AFGBuildableFactory::OnBuildableReplicationDetailStateChange(newStateIsActive);
}

AFGReplicationDetailActor* ABuildableInserter::GetOrCreateReplicationDetailActor()
{
	return nullptr;
}


void ABuildableInserter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Refs to Actors we need for Position Calculation
	DOREPLIFETIME(ABuildableInserter, InputsInRange);
	DOREPLIFETIME(ABuildableInserter, OutputsInRange);
	// Configurable Values that do not frequently change
	DOREPLIFETIME(ABuildableInserter, netProp_Enabled);
	DOREPLIFETIME(ABuildableInserter, netProp_Automatic);
	DOREPLIFETIME(ABuildableInserter, netProp_UseAnimation);
}

void ABuildableInserter::StartTimeForState(const EInserterActionState State)
{
	Movement_Progress = 0.f;
	InserterState = State;
}

bool ABuildableInserter::StoreGrabbedItemForConveyor(const FInventoryStack & Stack, bool bOnlyCheck)
{
	bInputSwap = true;
	if (!mBufferInventory->IsSomethingOnIndex(EInserter_DroppedItem))
	{
		if (bOnlyCheck)
		{
			bInputSwap = false;
			return true;
		}
			
		const int32 AmountRemoved = mBufferInventory->AddStackToIndex(EInserter_DroppedItem,Stack, true);
		mBufferInventory->RemoveFromIndex(EInserter_HoldingItem, AmountRemoved);
		const int32 AmountLeft = GrabbedItem.Amount - AmountRemoved;
		GrabbedItem.Amount = FMath::Clamp(AmountLeft, 0, GrabStackSize);
		bInputSwap = false;
		return true;
	}
	bInputSwap = false;
	return false;
}

void ABuildableInserter::RestoreFromFailedAction()
{
	if (mBufferInventory->IsSomethingOnIndex(EInserter_HoldingItem))
	{
		// Recover holding Item
		FInventoryStack Stack;
		mBufferInventory->GetStackFromIndex(EInserter_HoldingItem, Stack);
		GrabbedItem = FItemAmount(Stack.Item.ItemClass,Stack.NumItems);
		GrabbedItem.Amount = FMath::Clamp(Stack.NumItems, 1, GrabStackSize);

	}
	StartTimeForState(EInserterActionState::EInserter_Recovering);
	if (netProp_UseAnimation)
		OnInserterEvent.Broadcast(EInserterEvent::EInserter_Finished, -1);
}

// The actual place Function unaware of a Building
bool ABuildableInserter::PlaceItem(UFGInventoryComponent* Inventory, const bool bOnlyCheck, AFGBuildable * BuildingTarget) {

	// something went wrong 
	if (!mBufferInventory || !Inventory || !GrabbedItem.ItemClass || GrabbedItem.Amount <= 0 || !mBufferInventory->IsSomethingOnIndex(EInserter_HoldingItem))
	{
		RestoreFromFailedAction();
		return false;
	}

	// create the Stack we want to Add 
	FInventoryStack Stack = FInventoryStack(GrabbedItem.Amount, GrabbedItem.ItemClass);

	if (Filter_ShouldIgnore(GrabbedItem.ItemClass, BuildingTarget))
		return false;

	if(Inventory == mBufferInventory && StoreGrabbedItemForConveyor(Stack,bOnlyCheck))
	{
		return true;
	}
	else
	{
		
		// if it has enough space for at least 1 Item we proceed
		if (Inventory->HasEnoughSpaceForStack(FInventoryStack(1, GrabbedItem.ItemClass)))
		{
			// if we only intend to check if its possible to place an Item , we stop here
			if (bOnlyCheck)
				return true;
			if(Cast<AFGBuildableConveyorAttachment>(BuildingTarget))
			{
				if (!Inventory->IsEmpty())
					return false;
				Stack.NumItems = 1;
			}
			const int32 AmountToRemove = Inventory->AddStack(Stack, true);
			mBufferInventory->RemoveFromIndex(EInserter_HoldingItem, AmountToRemove);
			const int32 AmountLeft = GrabbedItem.Amount - AmountToRemove;
			GrabbedItem.Amount = FMath::Clamp(AmountLeft, 0, GrabStackSize);
			return true;
		}	
	}


	if(SelectedOutput == -1)
		return false;
	// Assuming we checked a Building and we couldnt place it , reset the Selected input to be picked freshly.
	if (OutputsInRange.Num() > 1 || !OutputsInRange.IsValidIndex(SelectedOutput))
	{
		SelectedOutput = -1;	
	}

	return false;
}



int32 ABuildableInserter::RemoveItemsFromInventory(UFGInventoryComponent* Inventory,FInventoryStack & Stack)
{
	const TSubclassOf<class UFGItemDescriptor> ItemClass = Stack.Item.ItemClass;
	const int32 GrabAmount =  UInserterLib::GetAllItemsByClass(Inventory,Stack,GrabStackSize);
	if (GrabAmount > 0)
	{
		Stack.NumItems = GrabAmount;
		GetBufferInventory()->AddStackToIndex(EInserter_HoldingItem, Stack, false);
		GrabbedItem.ItemClass = ItemClass;
		GrabbedItem.Amount = GrabAmount;
		return GrabAmount;
	}
	return 0;
}

// Actual Grab Function
bool ABuildableInserter::GrabItem(UFGInventoryComponent* Inventory, const bool bOnlyCheck, AFGBuildable * BuildingTarget)
{
	if (!Inventory)
		return false;

	if (!Inventory->IsEmpty())
	{
		// Its still possible that we will Grab from different Indexes
		// but we need a Class from this Inventory that matches possibly existing Filters
		FInventoryStack Stack; int32 Index = Index = Inventory->GetFullestStackIndex();
		Inventory->GetStackFromIndex(Index, Stack);

		if (Filter_ShouldIgnore(Stack.Item.ItemClass, BuildingTarget))
			return false;
		// if we are only Peeking its enough to try to Grab from this Inventory
		if(bOnlyCheck || RemoveItemsFromInventory(Inventory,Stack) > 0)
			return true;
	}
	
	if(SelectedInput == -1)
		return false;
	
	if (InputsInRange.Num() > 1 || !InputsInRange.IsValidIndex(SelectedInput))
		SelectedInput = -1;
	
	return false;
}

// This doesnt prevent the Inserter from grabbing from a Belt we are connected to , it needs to be checked outside of this Function !
bool ABuildableInserter::GrabFromBuilding(AFGBuildable  *  BuildingTarget, const bool bOnlyCheck)
{
	// Cant Grab if we have items already

	// Belt is the only thing that is different here
	AFGBuildableConveyorBelt * Belt = Cast< AFGBuildableConveyorBelt>(BuildingTarget);
	if (Belt && !IsHoldingItem())
	{
		// if its a belt we grab from an Offset instead of a Inventory
		const FInventoryItem Grabbed = GrabFromOffset(Belt, bOnlyCheck);
		if (Grabbed.IsValid())
		{
			// if this was successful and we only wanted to check we exit here
			if (bOnlyCheck)
				return true;
			// Successful Grab will be added to BufferInventory
			const FInventoryStack Stack = FInventoryStack(Grabbed);
			mBufferInventory->AddStackToIndex(EInserter_HoldingItem, Stack, false);
			GrabbedItem = FItemAmount(Stack.Item.ItemClass,Stack.NumItems);
			return true;
		}

		return false;
	}

	return GrabItem(UInserterLib::TryGetOutputInventory(BuildingTarget), bOnlyCheck, BuildingTarget);
}

// Function for grabbing Items from Belts
FInventoryItem ABuildableInserter::GrabFromOffset(AFGBuildableConveyorBelt * Belt, bool bOnlyCheck)
{
	FConveyorBeltItem Item;
	FVector WorldPos; 
	const int32 Index = UInserterLib::CanGrabFromBelt(Belt, this,WorldPos,Item);
	if(Index != -1)
	{
		if (!bOnlyCheck)
			Belt->Factory_RemoveItemAt(Index);
		BeltTargetLocation = WorldPos;
	}
	return Item.Item;
}

//  finds the Pickuptarget
int8 ABuildableInserter::GetPickupTarget()
{

	if (InputsInRange.Num() > 0)
	{
		int32 Ind = 0 ;
		for (AFGBuildable* i : InputsInRange)
		{
			if (!i || i->IsPendingKill())
			{
				InputsInRange.Remove(i);
				break;
			}
		
			if (GrabFromBuilding(i, true))
			{
				SelectedInput = Ind;
				return SelectedInput;
			}
			Ind = Ind+1;
		}
	}
	return -1;
}

// Function to check if we can Pickup Items NOW
bool ABuildableInserter::CanExecutePickup()
{
	if (InputsInRange.IsValidIndex(SelectedInput) && GrabFromBuilding(InputsInRange[SelectedInput], false))
		return true;
	else
	{
		StartTimeForState(EInserterActionState::EInserter_Recovering);
		if (netProp_UseAnimation)
			OnInserterEvent.Broadcast(EInserterEvent::EInserter_Finished, -1);
		return false;
	}
}

// Chooses a Valid Drop Target
int32 ABuildableInserter::GetDropTarget()
{
	if (OutputsInRange.Num() > 0)
	{
		int32 Ind = 0;
		for (AFGBuildableFactory* i :  OutputsInRange)
		{
			if (!i || i->IsPendingKill())
			{
				OutputsInRange.Remove(i);
				return -1;
			}
			if (PlaceItem(UInserterLib::TryGetInputInventory(i), true, i))
			{
				SelectedOutput = Ind;
				return SelectedOutput;
			}
			Ind = Ind + 1;
		}
	}
	return -1;
}


// Checks if we can actually Drop an Item NOW 
bool ABuildableInserter::CanExecuteDrop()
{
	// we need a Building set for this
	if (OutputsInRange.IsValidIndex(SelectedOutput))
	{
		AFGBuildableFactory * Buildable = OutputsInRange[SelectedOutput];
		if(Buildable == this && PlaceItem(mBufferInventory, false, this) || PlaceItem(UInserterLib::TryGetInputInventory(Buildable), false, Buildable))
			return true;
	}
	else
	{
		StartTimeForState(EInserterActionState::EInserter_Recovering);

		if (netProp_UseAnimation)
			OnInserterEvent.Broadcast(EInserterEvent::EInserter_Finished, -1);
	}

	return false;
}

UFGInventoryComponent* ABuildableInserter::GetInventory()
{

	if (!HasAuthority())
	{
		return nullptr;
	}
	else
		return mBufferInventory;
	return nullptr;
}

TSubclassOf<UFGRecipe> ABuildableInserter::GetBuiltWithRecipe(AFGBuildable* Building)
{
	if (!Building)return nullptr;
	return Building->GetBuiltWithRecipe();
}

bool ABuildableInserter::IsHoldingItem() const
{
	if (HasAuthority())
	{
		if (mBufferInventory) { return mBufferInventory->IsSomethingOnIndex(EInserter_HoldingItem); }
		else return false;
	}
	return GrabbedItem.Amount > 0;
}

// FIN Function to set a Building as viable InputInRange
bool ABuildableInserter::netFunc_addInput(AFGBuildable * BuildingTarget)
{
	if (BuildingTarget)
	{
		if (InputsInRange.Contains(BuildingTarget))
			return true;

		// this is kinda stupid but i dont want to cause a trace from here incase someone tries to use this too often..
		if (FVector::Dist2D(BuildingTarget->GetActorLocation(), GetActorLocation()) > Range)
		{
			return false; 
		}

		InputsInRange.Add(BuildingTarget);
		return true;
	}
	return false;
}

// FIN FUNCTION to set a Building as viable Output
bool ABuildableInserter::netFunc_addOutput(AFGBuildableFactory * BuildingTarget)
{
	if (BuildingTarget)
	{
		if (OutputsInRange.Contains(BuildingTarget))
			return true;

		// this is kinda stupid but i dont want to cause a trace from here incase someone tries to use this too often..
		if (FVector::Dist2D(BuildingTarget->GetActorLocation(), GetActorLocation()) > Range)
		{
			return false;
		}

		OutputsInRange.Add(BuildingTarget);
		return true;
	}
	return false;
}

// FIN Function to directly set a Building as Input
bool ABuildableInserter::netFunc_setSelectedInput(AFGBuildable * BuildingTarget)
{
	if (!BuildingTarget)
		return false;

	if (InputsInRange.Contains(BuildingTarget))
	{
		SelectedInput = InputsInRange.Find(BuildingTarget);
		return true;
	}
	else
		return false;
}

// FIN Function to directly set a Building as Output
bool ABuildableInserter::netFunc_setSelectedOutput(AFGBuildableFactory * BuildingTarget)
{
	if (!BuildingTarget)
		return false;

	if (OutputsInRange.Contains(BuildingTarget))
	{
		SelectedOutput = OutputsInRange.Find(BuildingTarget);
		return true;
	}
	else
		return false;
}

FVector ABuildableInserter::GetBeltLocation() const
{
	return BeltTargetLocation;
}

void ABuildableInserter::SetBeltLocation(const FVector New)
{
	BeltTargetLocation = New;
}