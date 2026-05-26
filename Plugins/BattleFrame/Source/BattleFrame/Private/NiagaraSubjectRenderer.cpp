/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "NiagaraSubjectRenderer.h"
#include "BattleFrameFunctionLibraryRT.h"
#include "HAL/Platform.h"
#include "Traits/SubType.h"
#include "Traits/Animation.h"
#include "Traits/RenderBatchData.h"
#include "Traits/Rendering.h"
#include "Traits/Collider.h"
#include "Traits/HealthBar.h"
#include "BattleFrameBattleControl.h"


// Sets default values
ANiagaraSubjectRenderer::ANiagaraSubjectRenderer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// Called when the game starts or when spawned
void ANiagaraSubjectRenderer::BeginPlay()
{
	Super::BeginPlay();

	CurrentWorld = GetWorld();

	if (CurrentWorld)
	{
		Mechanism = UMachine::ObtainMechanism(CurrentWorld);
		BattleControl = Cast<ABattleFrameBattleControl>(UGameplayStatics::GetActorOfClass(CurrentWorld, ABattleFrameBattleControl::StaticClass()));

		if (Mechanism && BattleControl && SubType.Index >= 0)
		{
			Initialized = true;
		}
	}
}

void ANiagaraSubjectRenderer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CurrentWorld = GetWorld();

	if (CurrentWorld)
	{
		BattleControl = Cast<ABattleFrameBattleControl>(UGameplayStatics::GetActorOfClass(CurrentWorld, ABattleFrameBattleControl::StaticClass()));

		if (BattleControl)
		{
			BattleControl->ExistingRenderers.Remove(SubType.Index);
		}
	}
}

// Called every frame
void ANiagaraSubjectRenderer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float SafeDeltaTime = FMath::Clamp(DeltaTime, 0, 0.0333f);

	if (Initialized)
	{
		Register();
		IdleCheck();
	}
}

void ANiagaraSubjectRenderer::Register()
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("RenderRegister");

	FFilter Filter = FFilter::Make<FAgent, FCollider, FLocated, FDirected, FScaled, FAnimation, FHealthBar, FAnimation, FActivated>().Exclude<FRendering>();
	UBattleFrameFunctionLibraryRT::IncludeSubTypeTraitByIndex(SubType.Index, Filter);

	Mechanism->Operate<FUnsafeChain>(Filter,
		[&](const FSubjectHandle Subject,
			const FCollider& Collider,
			const FLocated& Located,
			const FDirected& Directed,
			const FScaled& Scaled,
			const FHealthBar& HealthBar,
			const FAnimation& Animation,
			const FAnimating& Animating)
		{
			FQuat Rotation = Directed.Direction.ToOrientationQuat();

			FVector FinalScale(Scale);
			FinalScale = FinalScale * Scaled.RenderScale * Scaled.JiggleMultiplier;

			float FinalRadius = Collider.Radius * Scaled.Scale;

			FTransform SubjectTransform(Rotation * OffsetRotation.Quaternion(),Located.Location + OffsetLocation - FVector(0, 0, FinalRadius), FinalScale);

			FSubjectHandle RenderBatch = FSubjectHandle();
			FAgentRenderBatchData* Data = nullptr;

			// Get the first render batch that has a free trans slot
			for (const FSubjectHandle Renderer : SpawnedRenderBatches)
			{
				FAgentRenderBatchData* CurrentData = Renderer.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();

				if (CurrentData->FreeTransforms.Num() > 0 || CurrentData->Transforms.Num() < RenderBatchSize)
				{
					RenderBatch = Renderer;
					Data = Renderer.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();
					break;
				}
			}

			if (!Data)// all current batches are full
			{
				RenderBatch = AddRenderBatch();// add a new batch
				Data = RenderBatch.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();
			}

			int32 NewInstanceId;

			// Check if FreeTransforms has any members
			if (!Data->FreeTransforms.IsEmpty())
			{
				NewInstanceId = Data->FreeTransforms.Pop(); // Reuse an existing instance ID
				Data->Transforms[NewInstanceId] = SubjectTransform; // Update the corresponding transform

				Data->LocationArray[NewInstanceId] = SubjectTransform.GetLocation();
				Data->OrientationArray[NewInstanceId] = SubjectTransform.GetRotation();
				Data->ScaleArray[NewInstanceId] = SubjectTransform.GetScale3D();

				// Dynamic params 0, encode multiple values into a single float
				float Elem0 = ABattleFrameBattleControl::EncodeAnimationIndices(Animating.AnimIndex0, Animating.AnimIndex1, Animating.AnimIndex2);
				float Elem1 = ABattleFrameBattleControl::EncodePauseFrames(Animating.AnimPauseFrame0, Animating.AnimPauseFrame1, Animating.AnimPauseFrame2);
				float Elem2 = ABattleFrameBattleControl::EncodePlayRates(Animating.AnimPlayRate0, Animating.AnimPlayRate1, Animating.AnimPlayRate2);
				float Elem3 = ABattleFrameBattleControl::EncodeStatusEffects(Animating.HitGlow, Animating.IceFxInterped, Animating.FireFxInterped, Animating.PoisonFxInterped);

				Data->AnimIndex_PauseFrame_Playrate_MatFx_Array[NewInstanceId] = FVector4(Elem0, Elem1, Elem2, Elem3);

				// Dynamic params 1
				Data->AnimTimeStamp_Array[NewInstanceId] = FVector4(Animating.AnimCurrentTime0 - Animating.AnimOffsetTime0, Animating.AnimCurrentTime1 - Animating.AnimOffsetTime1, Animating.AnimCurrentTime2 - Animating.AnimOffsetTime2, 0);

				// Pariticle color
				Data->AnimLerp0_AnimLerp1_Team_Dissolve_Array[NewInstanceId] = FVector4(Animating.AnimLerp0, Animating.AnimLerp1, Animating.Team, Animating.Dissolve);

				Data->HealthBar_Opacity_CurrentRatio_TargetRatio_Array[NewInstanceId] = FVector(HealthBar.Opacity, HealthBar.CurrentRatio, HealthBar.TargetRatio);

				Data->InsidePool_Array[NewInstanceId] = false;
			}
			else
			{
				// Add new instance and get its ID
				Data->Transforms.Add(SubjectTransform);
				NewInstanceId = Data->Transforms.Num() - 1;

				// SubjectTransform
				Data->LocationArray.Add(SubjectTransform.GetLocation());
				Data->OrientationArray.Add(SubjectTransform.GetRotation());
				Data->ScaleArray.Add(SubjectTransform.GetScale3D());

				// Dynamic params 0, encode multiple values into a single float
				float Elem0 = ABattleFrameBattleControl::EncodeAnimationIndices(Animating.AnimIndex0, Animating.AnimIndex1, Animating.AnimIndex2);
				float Elem1 = ABattleFrameBattleControl::EncodePauseFrames(Animating.AnimPauseFrame0, Animating.AnimPauseFrame1, Animating.AnimPauseFrame2);
				float Elem2 = ABattleFrameBattleControl::EncodePlayRates(Animating.AnimPlayRate0, Animating.AnimPlayRate1, Animating.AnimPlayRate2);
				float Elem3 = ABattleFrameBattleControl::EncodeStatusEffects(Animating.HitGlow, Animating.IceFxInterped, Animating.FireFxInterped, Animating.PoisonFxInterped);

				Data->AnimIndex_PauseFrame_Playrate_MatFx_Array.Add(FVector4(Elem0, Elem1, Elem2, Elem3));

				// Dynamic params 1
				Data->AnimTimeStamp_Array.Add(FVector4(Animating.AnimCurrentTime0 - Animating.AnimOffsetTime0, Animating.AnimCurrentTime1 - Animating.AnimOffsetTime1, Animating.AnimCurrentTime2 - Animating.AnimOffsetTime2, 0));

				// Pariticle color
				Data->AnimLerp0_AnimLerp1_Team_Dissolve_Array.Add(FVector4(Animating.AnimLerp0, Animating.AnimLerp1, Animating.Team, Animating.Dissolve));

				Data->HealthBar_Opacity_CurrentRatio_TargetRatio_Array.Add(FVector(HealthBar.Opacity, HealthBar.CurrentRatio, HealthBar.TargetRatio));

				Data->InsidePool_Array.Add(false);
			}

			Subject.SetTrait(FRendering{ NewInstanceId, RenderBatch });
		}
	);
}

void ANiagaraSubjectRenderer::IdleCheck()
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("IdleCheck");
	TArray<FSubjectHandle> CachedSpawnedRenderBatches = SpawnedRenderBatches;

	for (const FSubjectHandle RenderBatch : CachedSpawnedRenderBatches)
	{
		FAgentRenderBatchData* CurrentData = RenderBatch.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();

		if (CurrentData->FreeTransforms.Num() == CurrentData->Transforms.Num())// current render batch is completely empty
		{
			RemoveRenderBatch(RenderBatch);
		}
	}

	if (SpawnedRenderBatches.IsEmpty())
	{
		this->Destroy();// remove this renderer
	}
}

FSubjectHandle ANiagaraSubjectRenderer::AddRenderBatch()
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("AddRenderBatch");
	FSubjectHandle RenderBatch = Mechanism->SpawnSubject(FAgentRenderBatchData());
	SpawnedRenderBatches.Add(RenderBatch);

	FAgentRenderBatchData* NewData = RenderBatch.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();

	NewData->Scale = Scale;
	NewData->OffsetLocation = OffsetLocation;
	NewData->OffsetRotation = OffsetRotation;

	auto System = UNiagaraFunctionLibrary::SpawnSystemAtLocation
	(
		GetWorld(),
		NiagaraSystemAsset,
		GetActorLocation(),
		FRotator::ZeroRotator, // rotation
		FVector(1), // scale
		false, // auto destroy
		true, // auto activate
		ENCPoolMethod::None,
		true
	);

	System->SetVariableStaticMesh(TEXT("StaticMesh"), StaticMeshAsset);

	NewData->SpawnedNiagaraSystem = System;

	return RenderBatch;
}

void ANiagaraSubjectRenderer::RemoveRenderBatch(FSubjectHandle RenderBatch)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("RemoveRenderBatch");
	FAgentRenderBatchData* RenderBatchData = RenderBatch.GetTraitPtr<FAgentRenderBatchData, EParadigm::Unsafe>();
	RenderBatchData->SpawnedNiagaraSystem->DestroyComponent();
	SpawnedRenderBatches.Remove(RenderBatch);
	RenderBatch->Despawn();
}