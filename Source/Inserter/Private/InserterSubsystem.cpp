


#include "InserterSubsystem.h"

#include "FGGameState.h"

AInserterSubsystem::AInserterSubsystem() : Super() {
	this->PrimaryActorTick.TickGroup = TG_PrePhysics; 
	this->PrimaryActorTick.EndTickGroup = TG_PrePhysics; 
	this->PrimaryActorTick.bTickEvenWhenPaused = false; 
	this->PrimaryActorTick.bCanEverTick = true; 
	this->PrimaryActorTick.bStartWithTickEnabled = true;
	this->PrimaryActorTick.bAllowTickOnDedicatedServer = true; 
	this->PrimaryActorTick.TickInterval = 0;
	this->bAlwaysRelevant = true;
	this->bReplicates = true;
}
void AInserterSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
}

void AInserterSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{

}

void AInserterSubsystem::PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{

}

void AInserterSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
}

void AInserterSubsystem::GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects)
{
}

bool AInserterSubsystem::NeedTransform_Implementation()
{
	return false;
}

bool AInserterSubsystem::ShouldSave_Implementation() const
{
	return false;
}

void AInserterSubsystem::BeginPlay()
{
	Super::BeginPlay();
	bGetCheatNoPower = Cast<AFGGameState>(GetWorld()->GetGameState())->GetCheatNoPower();
}

void AInserterSubsystem::Tick(float DT)
{
	TickInserters(DT);
}

AInserterSubsystem * AInserterSubsystem::Get(UWorld * World)
{
	if (World)
	{
		TArray<AActor *> Arr;
		UGameplayStatics::GetAllActorsOfClass(World,AInserterSubsystem::StaticClass(),Arr);
		if (Arr.IsValidIndex(0))
		{
			AInserterSubsystem* Out = Cast<AInserterSubsystem>(Arr[0]);
			return Out;
		}
		else
		{
			// need to change this to a BP Path
			return Cast< AInserterSubsystem>(World->SpawnActor(AInserterSubsystem::StaticClass()));
		}
	}
	else
		return nullptr;
}

AInserterSubsystem * AInserterSubsystem::GetInserterSubsystem(UObject * WorldContext)
{
	return Get(WorldContext->GetWorld());
}

// Function Researchers Call when they gain Power
void AInserterSubsystem::RegisterInserter(ABuildableInserter * Inserter)
{
	if (Inserter)
	{
		if (!nInserter.Contains(Inserter))
		{
			nInserter.Add(Inserter);
		}
		else
		{
			//
		}
	}
}

// Function Researchers Call when they lose Power
void AInserterSubsystem::UnregisterInserter(ABuildableInserter * Inserter)
{
	if (!nInserter.Contains(Inserter))
	{
		nInserter.Remove(Inserter);
	}
	else
	{
		//
	}
}

// Inserter Tick 
void AInserterSubsystem::TickInserters(float dt)
{
	for (ABuildableInserter* i : nInserter)
	{
		if (!i->IsPendingKill())
		{
			if (i->IsActorBeingDestroyed())
			{
				nInserter.Remove(i);
				break;
			}
			if (i->HasPower() || bGetCheatNoPower)
			{
				i->InserterTick(dt);
			}
		}
		else
		{
			nInserter.Remove(i);
		}
	}
}
