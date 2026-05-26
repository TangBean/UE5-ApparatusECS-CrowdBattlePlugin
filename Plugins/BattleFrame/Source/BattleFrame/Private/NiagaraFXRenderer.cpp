/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "NiagaraFXRenderer.h"
#include "Traits/TraitIncludes.h"
#include "BattleFrameBattleControl.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

ANiagaraFXRenderer::ANiagaraFXRenderer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

// Called when the game starts or when spawned
void ANiagaraFXRenderer::BeginPlay()
{
    Super::BeginPlay();

    CurrentWorld = GetWorld();

    if (CurrentWorld)
    {
        Mechanism = UMachine::ObtainMechanism(CurrentWorld);
        BattleControl = Cast<ABattleFrameBattleControl>(UGameplayStatics::GetActorOfClass(CurrentWorld, ABattleFrameBattleControl::StaticClass()));

        if (Mechanism && BattleControl)
        {
            Initialized = true;
        }
    }

    AddRenderBatch();
}

void ANiagaraFXRenderer::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE_STR("NiagaraFxRendererTick");

    Super::Tick(DeltaTime);

    float SafeDeltaTime = FMath::Clamp(DeltaTime, 0, 0.0333f);

    if (Initialized)
    {
        // Burst Fx
        auto FirstRenderBatch = SpawnedRenderBatches[0];
        auto FirstData = FirstRenderBatch.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
        FirstData->ResetBurstData();

        FFilter BurstFxFilter = FFilter::Make<FLocated, FDirected, FScaled, FIsBurstFx>();
        BurstFxFilter += TraitType;
        BurstFxFilter += SubType;

        Mechanism->Operate<FUnsafeChain>(BurstFxFilter,
            [&](FSubjectHandle Subject,
                const FLocated& Located,
                const FDirected& Directed,
                const FScaled& Scaled)
            {
                FirstData->LocationArray_Burst.Add(Located.Location);
                FirstData->OrientationArray_Burst.Add(Directed.Direction.ToOrientationQuat());
                FirstData->ScaleArray_Burst.Add(Scaled.RenderScale);

                Subject.HasTrait<FIsAttachedFx>() ? Subject.RemoveTrait<FIsBurstFx>() : Subject.Despawn();
            });

        // Register New Attached Fx
        FFilter NewAttachedFxFilter = FFilter::Make<FLocated, FDirected, FScaled, FIsAttachedFx>().Exclude<FRendering>();
        NewAttachedFxFilter += TraitType;
        NewAttachedFxFilter += SubType;

        Mechanism->Operate<FUnsafeChain>(NewAttachedFxFilter,
            [&](FSubjectHandle Subject,
                const FLocated& Located,
                const FDirected& Directed,
                const FScaled& Scaled)
            {
                //UE_LOG(LogTemp, Warning, TEXT("RegisterNew"));
                FQuat Rotation = Directed.Direction.ToOrientationQuat();
                FVector Scale = FVector(Scaled.RenderScale);

                FTransform SubjectTransform(Rotation, Located.Location, Scale);

                FSubjectHandle RenderBatch = FSubjectHandle();
                FFxRenderBatchData* Data = nullptr;

                // Get the first render batch that has a free trans slot
                for (const FSubjectHandle Renderer : SpawnedRenderBatches)
                {
                    FFxRenderBatchData* CurrentData = Renderer.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();

                    if (CurrentData->FreeTransforms_Attached.Num() > 0 || CurrentData->Transforms_Attached.Num() < RenderBatchSize)
                    {
                        RenderBatch = Renderer;
                        Data = Renderer.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
                        break;
                    }
                }

                if (!Data)// all current batches are full
                {
                    RenderBatch = AddRenderBatch();// add a new batch
                    Data = RenderBatch.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
                }

                int32 NewInstanceId;

                if (!Data->FreeTransforms_Attached.IsEmpty())
                {
                    // Pick an index which is no longer in cooldown
                    NewInstanceId = Data->FreeTransforms_Attached.Pop();
                    Data->Transforms_Attached[NewInstanceId] = SubjectTransform; // Update the corresponding transform

                    Data->LocationArray_Attached[NewInstanceId] = SubjectTransform.GetLocation();
                    Data->OrientationArray_Attached[NewInstanceId] = SubjectTransform.GetRotation();
                    Data->ScaleArray_Attached[NewInstanceId] = SubjectTransform.GetScale3D();

                    Data->CoolDowns_Attached[NewInstanceId] = PoollingCoolDown;
                    Data->LocationEventArray_Attached[NewInstanceId] = true;
                }
                else
                {
                    Data->Transforms_Attached.Add(SubjectTransform);
                    NewInstanceId = Data->Transforms_Attached.Num() - 1;

                    Data->LocationArray_Attached.Add(SubjectTransform.GetLocation());
                    Data->OrientationArray_Attached.Add(SubjectTransform.GetRotation());
                    Data->ScaleArray_Attached.Add(SubjectTransform.GetScale3D());

                    Data->CoolDowns_Attached.Add(PoollingCoolDown);
                    Data->LocationEventArray_Attached.Add(true);
                }

                Subject.SetTrait(FRendering{ NewInstanceId, RenderBatch });
            });

        // Pooling info
        for (const FSubjectHandle Renderer : SpawnedRenderBatches)
        {
            auto CurrentData = Renderer.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
            CurrentData->ValidTransforms_Attached.Reset();
        }

        // Update Existing Attached Fx
        FFilter ExistingAttachedFxFilter = FFilter::Make<FLocated, FDirected, FScaled, FIsAttachedFx, FRendering>();
        ExistingAttachedFxFilter += TraitType;
        ExistingAttachedFxFilter += SubType;

        auto Chain = Mechanism->EnchainSolid(ExistingAttachedFxFilter);
        UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);
        ProjectileCount = Chain->IterableNum();

        Chain->OperateConcurrently(
            [&](FSolidSubjectHandle Subject,
                const FLocated& Located,
                const FDirected& Directed,
                const FScaled& Scaled,
                const FRendering& Rendering)
            {
                FQuat Rotation = Directed.Direction.ToOrientationQuat();
                FVector Scale = FVector(Scaled.RenderScale);

                FTransform SubjectTransform(Rotation, Located.Location, Scale);

                int32 InstanceId = Rendering.InstanceId;

                FFxRenderBatchData* Data = Rendering.Renderer.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
                    
                Data->Lock();
                Data->ValidTransforms_Attached[InstanceId] = true;
                Data->Transforms_Attached[InstanceId] = SubjectTransform;
                Data->LocationArray_Attached[InstanceId] = SubjectTransform.GetLocation();
                Data->OrientationArray_Attached[InstanceId] = SubjectTransform.GetRotation();
                Data->ScaleArray_Attached[InstanceId] = SubjectTransform.GetScale3D();
                Data->Unlock();

            }, ThreadsCount, BatchSize);

        for (const FSubjectHandle Renderer : SpawnedRenderBatches)
        {
            auto CurrentData = Renderer.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();

            CurrentData->FreeTransforms_Attached.Reset();

            // Loop through all InValidTransforms
            for (int32 i = CurrentData->ValidTransforms_Attached.IndexOf(false); i < CurrentData->Transforms_Attached.Num(); i = CurrentData->ValidTransforms_Attached.IndexOf(false, i + 1))
            {
                CurrentData->LocationEventArray_Attached[i] = false;
                CurrentData->ScaleArray_Attached[i] = FVector::ZeroVector;
                CurrentData->Transforms_Attached[i].SetScale3D(FVector::ZeroVector);
                CurrentData->LocationArray_Attached[i] = FVector(0, 0, -10000);
                CurrentData->Transforms_Attached[i].SetLocation(FVector(0, 0, -10000));

                if (CurrentData->CoolDowns_Attached[i] <= 0) // If done cooling then it's a free slot
                {
                    CurrentData->FreeTransforms_Attached.Add(i);
                }
                else
                {
                    CurrentData->CoolDowns_Attached[i] -= SafeDeltaTime;
                }
            }

            TRACE_CPUPROFILER_EVENT_SCOPE_STR("Sending");
            // send to niagara
            if (CurrentData->SpawnedNiagaraSystem)
            {
                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("LocationArray_Burst"),
                    CurrentData->LocationArray_Burst
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayQuat(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("OrientationArray_Burst"),
                    CurrentData->OrientationArray_Burst
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("ScaleArray_Burst"),
                    CurrentData->ScaleArray_Burst
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("LocationArray_Attached"),
                    CurrentData->LocationArray_Attached
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayQuat(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("OrientationArray_Attached"),
                    CurrentData->OrientationArray_Attached
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("ScaleArray_Attached"),
                    CurrentData->ScaleArray_Attached
                );

                UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayBool(
                    CurrentData->SpawnedNiagaraSystem,
                    FName("LocationEventArray_Attached"),
                    CurrentData->LocationEventArray_Attached
                );
            }
        }

        IdleCheck();
    }
}

void ANiagaraFXRenderer::IdleCheck()
{
    //TRACE_CPUPROFILER_EVENT_SCOPE_STR("IdleCheck");
    TArray<FSubjectHandle> CachedSpawnedRenderBatches = SpawnedRenderBatches;

    // Only remove render batches if we have more than one
    if (CachedSpawnedRenderBatches.Num() > 1)
    {
        for (const FSubjectHandle RenderBatch : CachedSpawnedRenderBatches)
        {
            FFxRenderBatchData* CurrentData = RenderBatch.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();

            if (CurrentData->FreeTransforms_Attached.Num() == CurrentData->Transforms_Attached.Num()) // current render batch is completely empty
            {
                RemoveRenderBatch(RenderBatch);
            }
        }
    }
}

FSubjectHandle ANiagaraFXRenderer::AddRenderBatch()
{
    //TRACE_CPUPROFILER_EVENT_SCOPE_STR("AddRenderBatch");
    FSubjectHandle RenderBatch = Mechanism->SpawnSubject(FFxRenderBatchData());
    SpawnedRenderBatches.Add(RenderBatch);

    FFxRenderBatchData* NewData = RenderBatch.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();

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

    NewData->SpawnedNiagaraSystem = System;

    return RenderBatch;
}

void ANiagaraFXRenderer::RemoveRenderBatch(FSubjectHandle RenderBatch)
{
    //TRACE_CPUPROFILER_EVENT_SCOPE_STR("RemoveRenderBatch");
    FFxRenderBatchData* RenderBatchData = RenderBatch.GetTraitPtr<FFxRenderBatchData, EParadigm::Unsafe>();
    RenderBatchData->SpawnedNiagaraSystem->DestroyComponent();
    SpawnedRenderBatches.Remove(RenderBatch);
    RenderBatch->Despawn();
}
