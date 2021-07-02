#pragma once

#include "CoreMinimal.h"
#include "Buildable/BuildableInserter.h"
#include "BuildableInserterFilter.generated.h"



/**
 * 
 */

USTRUCT(Blueprintable)
struct INSERTER_API FInserterBuildingProfile
{
	GENERATED_BODY()

public:
	FInserterBuildingProfile();
	UPROPERTY(BlueprintReadWrite,savegame)
		TArray<TSubclassOf<class UFGItemDescriptor>> FilterClasses;
	UPROPERTY(BlueprintReadWrite, savegame)
		AFGBuildable * Owner;
};


UCLASS()
class INSERTER_API ABuildableInserterFilter : public ABuildableInserter
{
	GENERATED_BODY()
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool Filter_ShouldIgnore(TSubclassOf<class UFGItemDescriptor> inClass, AFGBuildable * BuildingTarget) override;

	void SyncBuildingProfiles();

	/*
	 Items that should be Picked up exclusivly
	*/


public:


	UFUNCTION(BlueprintCallable)
		bool ToggleInserterFilter(TSubclassOf<class UFGItemDescriptor> inClass);

	UFUNCTION(BlueprintCallable)
		bool ToggleBuildingFilter(AFGBuildable * BuildingTarget, TSubclassOf<class UFGItemDescriptor> inClass);
	UFUNCTION(BlueprintCallable)
		bool GetBuildingFilter(AFGBuildable* BuildingTarget, TSubclassOf<class UFGItemDescriptor> inClass);
	
	UFUNCTION(BlueprintCallable)
		void ResetFilters();


	UPROPERTY(BlueprintReadOnly, savegame, Replicated)
		TArray<TSubclassOf<class UFGItemDescriptor>> FilterClasses;

	UPROPERTY(BlueprintReadOnly, savegame, Replicated)
		TArray<AFGBuildable *> FGBuildableArray;
	UPROPERTY(BlueprintReadOnly, savegame, Replicated)
		TArray<FInserterBuildingProfile> InserterBuildingProfiles;

};
