


#include "InserterAnim.h"
FInserterAnimation::FInserterAnimation()
{
	StartRotation = 0.f;
	StartItemLocation = FVector(0, 0, 0);
	LookAtRotation = 0.f;
	RestingLocation = FVector(0, 0, 0);
}


FInserterAnimation::FInserterAnimation(UInserterAnim * Building, AFGBuildable * BuildingTarget)
{
	if (!Building || !Building->nOwner)
	{
		StartRotation = 0.f;
		StartItemLocation = FVector(0, 0, 0);
		LookAtRotation = 0.f;
		RestingLocation = FVector(0, 0, 0);
		return;
	}
		
	if (Cast<AFGBuildableConveyorBase>(BuildingTarget))
	{
		// Pulling the Target location up by 120 cm 
		// this isnt ideal still, but its close enough to the actual Belt for now
		TargetLocation = Building->nOwner->GetActorTransform().InverseTransformPosition(Building->nOwner->BeltTargetLocation) + FVector(0, 0, 120.f);
	}
	else if (Building->nOwner == BuildingTarget)
	{
		// if the Target is ourselfs we use this static Transform as Target 
		TargetLocation = FVector(200.f, 0.f, 120.f);
	}
	else if(BuildingTarget)
	{
		// Target Location for Buildings always needs Inverse Transform 
		// we move 200 cm UP again since the Building has its Pivot on the Ground
		TargetLocation = Building->nOwner->GetActorTransform().InverseTransformPosition(BuildingTarget->GetActorLocation()+FVector(0,0,200.f));
		float Len; FVector Dir;
		TargetLocation.ToDirectionAndLength(Dir,Len);
		if(Len > 900.f)
		{
			TargetLocation = Dir * 900.f;
		}
	}
	else
	{
		TargetLocation = Building->nOwner->GetActorTransform().InverseTransformPosition(FVector(200.f, 0.f, 250.f));
	}

	StartRotation = Building->BaseRotation;
	StartItemLocation = Building->HeadLocation;
	LookAtRotation = UKismetMathLibrary::FindLookAtRotation(FVector(0, 0, 0), TargetLocation).Yaw;
	// Resting location will be Rotated by the Inserters Rotation
	RestingLocation = Building->nOwner->RestingTransform.GetLocation().RotateAngleAxis((LookAtRotation), FVector(0, 0, 1).GetSafeNormal());
}

void UInserterAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	nOwner = Cast<ABuildableInserter>(GetOwningActor());
	if(nOwner)
	{
		HeadLocation = FVector(175.f, 0.f, 350.f);
	}
}


void UInserterAnim::OnAnimFinished()
{
	if (nOwner)
	{
		OnEndGrabSound.Broadcast();

		AnimState = ESplitterAnimState::EInserter_Anim_NONE;
		Anim.StartItemLocation = HeadLocation;
	}
}
void UInserterAnim::OnPreDropEvent(AFGBuildable * GrabBuilding)
{
	if (nOwner && GrabBuilding)
	{
		Anim = FInserterAnimation(this, GrabBuilding);
		AnimState = ESplitterAnimState::EInserter_Anim_Rotate;
		OnBeginMoveSound.Broadcast();
	}
}

void UInserterAnim::OnPreGrabEvent(AFGBuildable * GrabBuilding)
{
	if (nOwner && GrabBuilding)
	{
		Anim = FInserterAnimation(this, GrabBuilding);
		AnimState = ESplitterAnimState::EInserter_Anim_Rotate;
		OnBeginMoveSound.Broadcast();
	}
}


void UInserterAnim::OnDrop(AFGBuildable * GrabBuilding)
{
	if (nOwner)
	{
		OnStartGrabSound.Broadcast();
		OnAnimItemDrop.Broadcast();
		AnimState = ESplitterAnimState::EInserter_Anim_Recover;
	}
}

void UInserterAnim::OnGrab(AFGBuildable * GrabBuilding)
{
	if (nOwner)
	{
		OnStartGrabSound.Broadcast();
		OnAnimItemPickup.Broadcast();
		AnimState = ESplitterAnimState::EInserter_Anim_Recover;
	}
}

// Anim Tick all visual stuff
void UInserterAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!nOwner)
		return;
	
	switch (AnimState)
	{
	case ESplitterAnimState::EInserter_Anim_Rotate:
	{
		// Rotation is 1/2 of the Move Time
		if (nOwner->Movement_Progress < 0.5f && nOwner->Movement_Progress > 0.f)
		{
			BaseRotation = FMath::InterpEaseInOut(Anim.StartRotation, Anim.LookAtRotation,FMath::Clamp(nOwner->Movement_Progress * 2.f, 0.01f, 1.0f),1.05f);
			HeadLocation = Anim.StartItemLocation.RotateAngleAxis((BaseRotation - Anim.StartRotation), FVector(0, 0, 1).GetSafeNormal());
		}
		// if we are above 1/2 of the Time
		else if (nOwner->Movement_Progress >= 0.5f)
		{
			OnEndMoveSound.Broadcast();
			OnStartGrabSound.Broadcast();

			Anim.StartItemLocation = HeadLocation;
			AnimState = ESplitterAnimState::EInserter_Anim_Move;
		}
		break;

	}
	case ESplitterAnimState::EInserter_Anim_Move:
	{
		// from above 1/2 to 1 
		if (nOwner->Movement_Progress < 1.f && nOwner->Movement_Progress > 0.5f)
		{
			BaseRotation = Anim.LookAtRotation;
			HeadLocation = FMath::InterpEaseInOut(Anim.StartItemLocation, Anim.TargetLocation, FMath::Clamp((nOwner->Movement_Progress - 0.499f) * 2.f, 0.01f, 1.f), 1.05f);
		}
		else if (nOwner->Movement_Progress >= 1.f)
		{
			AnimState = ESplitterAnimState::EInserter_Anim_NONE;
		}
		break;
	}
	case ESplitterAnimState::EInserter_Anim_Recover:
	{
		// Recovery from 0 - 1 Progress
		if (FMath::Clamp(nOwner->Movement_Progress, 0.f, 1.01f) <= 1.f)
		{
			// simply moving the Head to the Rotated Resting Location
			HeadLocation = FMath::InterpEaseInOut(Anim.TargetLocation, Anim.RestingLocation, FMath::Clamp(nOwner->Movement_Progress, 0.01f, 1.f), 1.05f);
		}
		else if (nOwner->Movement_Progress >= 1.f)
		{
			AnimState = ESplitterAnimState::EInserter_Anim_NONE;
		}
		break;
	}
	default:
		break;

	}
}
