

#include "Buildable/BuildableInserterFilter.h"
#include "FactoryGame.h"

FInserterBuildingProfile::FInserterBuildingProfile(): Owner(nullptr)
{
} ;

void ABuildableInserterFilter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABuildableInserterFilter, FilterClasses);
	DOREPLIFETIME(ABuildableInserterFilter, FGBuildableArray);
	DOREPLIFETIME(ABuildableInserterFilter, InserterBuildingProfiles);

}

// Function to check if an ItemDescriptor should be Ignored 
bool ABuildableInserterFilter::Filter_ShouldIgnore(TSubclassOf<class UFGItemDescriptor> inClass, AFGBuildable * BuildingTarget)
{
	if (FilterClasses.Num() > 0)
	{
		if (FilterClasses.Contains(inClass))
		{
			if (FGBuildableArray.Contains(BuildingTarget))
			{
				if (InserterBuildingProfiles.IsValidIndex(FGBuildableArray.Find(BuildingTarget)))
				{
					if (InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Contains(inClass))
						return false;
					else
						return true;
				}
				else
				{
					SyncBuildingProfiles();
					return true;

				}
			}
			else
			{
				return false;
			}
		}
		else
			return true;
	}
	else if (FGBuildableArray.Contains(BuildingTarget))
	{
		if (InserterBuildingProfiles.IsValidIndex(FGBuildableArray.Find(BuildingTarget)))
		{
			if (InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Contains(inClass))
				return false;
			else
				return true;
		}
		else
		{
			SyncBuildingProfiles();
			return true;
		}
			
	}
	return false;
}

void ABuildableInserterFilter::SyncBuildingProfiles()
{
	FGBuildableArray.Remove(nullptr);
	FGBuildableArray.Shrink();
	TArray<FInserterBuildingProfile> Copy;
	for (auto i : InserterBuildingProfiles)
	{
		Copy.Add(i);
	}
	InserterBuildingProfiles.Empty();
	InserterBuildingProfiles.AddUninitialized(FGBuildableArray.Num());
	UE_LOG(LogTemp, Error, TEXT("SychBuildingProfiles"));
	for (auto i : Copy)
	{
		if (i.Owner && FGBuildableArray.Contains(i.Owner))
		{
			InserterBuildingProfiles[FGBuildableArray.Find(i.Owner)].FilterClasses = i.FilterClasses;
		}
	}
}

bool ABuildableInserterFilter::ToggleInserterFilter(TSubclassOf<UFGItemDescriptor> inClass)
{
	if (FilterClasses.Contains(inClass))
	{
		FilterClasses.Remove(inClass);
		return false;
	}
	else
	{
		FilterClasses.Add(inClass);
		return true;
	}
	FilterClasses.Shrink();
}


// Function to Toggle a Building Filter 
// We keep 2 Arrays Index Synced here so they are saveable 
bool ABuildableInserterFilter::ToggleBuildingFilter(AFGBuildable * BuildingTarget, TSubclassOf<class UFGItemDescriptor> inClass)
{
	if (FGBuildableArray.Contains(nullptr))
	{
		SyncBuildingProfiles();
	}

	if (FGBuildableArray.Contains(BuildingTarget))
	{
		if (InserterBuildingProfiles.IsValidIndex((FGBuildableArray.Find(BuildingTarget))))
		{
			if (InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Contains(inClass))
			{
				InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Remove(inClass);
				if (InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Num() == 0)
				{
					InserterBuildingProfiles.RemoveAt(FGBuildableArray.Find(BuildingTarget));
					FGBuildableArray.Remove(BuildingTarget);
				}
				return false;
			}
			else
			{
				InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Add(inClass);
				return true;
			}
		}
		else
		{
			SyncBuildingProfiles();
			// this must never be reached or we fucked up 
			return false;
		}
	}
	else
	{
		FGBuildableArray.Add(BuildingTarget);
		FInserterBuildingProfile Profile = FInserterBuildingProfile();
		Profile.Owner = BuildingTarget;
		Profile.FilterClasses.Add(inClass);
		InserterBuildingProfiles.Add(Profile);
		return true;
	}
	return false;
}


// Function to Toggle a Building Filter 
// We keep 2 Arrays Index Synced here so they are saveable 
bool ABuildableInserterFilter::GetBuildingFilter(AFGBuildable* BuildingTarget, TSubclassOf<class UFGItemDescriptor> inClass)
{
	if (FGBuildableArray.Contains(nullptr))
	{
		SyncBuildingProfiles();
	}

	if (FGBuildableArray.Contains(BuildingTarget))
	{
		if (InserterBuildingProfiles.IsValidIndex((FGBuildableArray.Find(BuildingTarget))))
		{
			if (InserterBuildingProfiles[FGBuildableArray.Find(BuildingTarget)].FilterClasses.Contains(inClass))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			SyncBuildingProfiles();
			// this must never be reached or we fucked up 
			return false;
		}
	}
	else
	{
		return false;
	}
	return false;
}

void ABuildableInserterFilter::ResetFilters()
{
	FilterClasses.Empty();
	FGBuildableArray.Empty();
	InserterBuildingProfiles.Empty();
}
