

#pragma once

#include "CoreMinimal.h"
#include "FGSubsystem.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGSaveInterface.h"
#include "Buildable/BuildableInserter.h"
#include "InserterSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class INSERTER_API AInserterSubsystem : public AFGSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()
public:
	AInserterSubsystem();

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray< UObject* >& out_dependentObjects) override;
	virtual bool NeedTransform_Implementation() override;
	virtual bool ShouldSave_Implementation() const override;
	// End IFSaveInterface

	virtual void BeginPlay() override;

	virtual void Tick(float DT) override;
public:
	static AInserterSubsystem* Get(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Inserters", Meta = (DefaultToSelf = "worldContext"))
		static AInserterSubsystem* GetInserterSubsystem(UObject* WorldContext);

	void RegisterInserter(class ABuildableInserter* Inserter);

	void UnregisterInserter(class ABuildableInserter* Inserter);

private:

	void TickInserters(float dt);

	UPROPERTY()
		TArray<ABuildableInserter* > nInserter;


	bool bGetCheatNoPower = false;
};
