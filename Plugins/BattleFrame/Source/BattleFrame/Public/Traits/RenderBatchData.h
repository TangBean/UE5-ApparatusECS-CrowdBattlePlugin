#pragma once

#include "CoreMinimal.h"

// Niagara Systems
#include "NiagaraSystem.h" 
#include "NiagaraComponent.h"
#include "SubjectHandle.h"
#include "BitMask.h"

#include "RenderBatchData.generated.h"


USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAgentRenderBatchData
{
    GENERATED_BODY()

    private:

    mutable std::atomic<bool> LockFlag{ false };

    public:

    void Lock() const
    {
        while (LockFlag.exchange(true, std::memory_order_acquire));
    }

    void Unlock() const
    {
        LockFlag.store(false, std::memory_order_release);
    }

    // Renderer
    UNiagaraComponent* SpawnedNiagaraSystem = nullptr;

    // Offset
    FVector OffsetLocation = FVector::ZeroVector;
    FRotator OffsetRotation = FRotator::ZeroRotator;
    FVector Scale = { 1.0f, 1.0f, 1.0f };

    // Pooling
    TArray<FTransform> Transforms;
    FBitMask ValidTransforms;
    TArray<int32> FreeTransforms;

    // Transform
    TArray<FVector> LocationArray;
    TArray<FQuat> OrientationArray;
    TArray<FVector> ScaleArray;

    // Anim
    TArray<FVector4> AnimIndex_PauseFrame_Playrate_MatFx_Array; // Dynamic params 0
    TArray<FVector4> AnimTimeStamp_Array; // Dynamic params 1
    TArray<FVector4> AnimLerp0_AnimLerp1_Team_Dissolve_Array; // Particle color channel

    // HealthBar
    TArray<FVector> HealthBar_Opacity_CurrentRatio_TargetRatio_Array;

    // TextPopUp
    TArray<FVector> Text_Location_Array;
    TArray<FVector4> Text_Value_Style_Scale_Offset_Array;

    // Other
    TArray<bool> InsidePool_Array;


    FAgentRenderBatchData(){};

    FAgentRenderBatchData(const FAgentRenderBatchData& Data)
    {
        LockFlag.store(Data.LockFlag.load());

        SpawnedNiagaraSystem = Data.SpawnedNiagaraSystem;

        Transforms=Data.Transforms;
        ValidTransforms=Data.ValidTransforms;
        FreeTransforms=Data.FreeTransforms;

        LocationArray=Data.LocationArray;
        OrientationArray=Data.OrientationArray;
        ScaleArray = Data.ScaleArray;

        AnimLerp0_AnimLerp1_Team_Dissolve_Array = Data.AnimLerp0_AnimLerp1_Team_Dissolve_Array;

        AnimIndex_PauseFrame_Playrate_MatFx_Array = Data.AnimIndex_PauseFrame_Playrate_MatFx_Array;
        AnimTimeStamp_Array = Data.AnimTimeStamp_Array;

        HealthBar_Opacity_CurrentRatio_TargetRatio_Array = Data.HealthBar_Opacity_CurrentRatio_TargetRatio_Array;

        Text_Location_Array = Data.Text_Location_Array;
        Text_Value_Style_Scale_Offset_Array = Data.Text_Value_Style_Scale_Offset_Array;

        InsidePool_Array = Data.InsidePool_Array;
    }

    FAgentRenderBatchData& operator=(const FAgentRenderBatchData& Data)
    {
        LockFlag.store(Data.LockFlag.load());

        SpawnedNiagaraSystem = Data.SpawnedNiagaraSystem;

        Transforms = Data.Transforms;
        ValidTransforms = Data.ValidTransforms;
        FreeTransforms = Data.FreeTransforms;

        LocationArray = Data.LocationArray;
        OrientationArray = Data.OrientationArray;
        ScaleArray = Data.ScaleArray;

        AnimLerp0_AnimLerp1_Team_Dissolve_Array = Data.AnimLerp0_AnimLerp1_Team_Dissolve_Array;

        AnimIndex_PauseFrame_Playrate_MatFx_Array = Data.AnimIndex_PauseFrame_Playrate_MatFx_Array;
        AnimTimeStamp_Array = Data.AnimTimeStamp_Array;

        HealthBar_Opacity_CurrentRatio_TargetRatio_Array = Data.HealthBar_Opacity_CurrentRatio_TargetRatio_Array;

        Text_Location_Array = Data.Text_Location_Array;
        Text_Value_Style_Scale_Offset_Array = Data.Text_Value_Style_Scale_Offset_Array;

        InsidePool_Array = Data.InsidePool_Array;

        return *this;
    }
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FFxRenderBatchData
{
    GENERATED_BODY()

private:

    mutable std::atomic<bool> LockFlag{ false };

public:

    void Lock() const
    {
        while (LockFlag.exchange(true, std::memory_order_acquire));
    }

    void Unlock() const
    {
        LockFlag.store(false, std::memory_order_release);
    }

    // Renderer
    UNiagaraComponent* SpawnedNiagaraSystem = nullptr;

    // Pooling_Attached
    FBitMask ValidTransforms_Attached;
    TArray<int32> FreeTransforms_Attached;
    TArray<float> CoolDowns_Attached;
    TArray<bool> LocationEventArray_Attached;

    // Transform_Attached
    TArray<FTransform> Transforms_Attached;
    TArray<FVector> LocationArray_Attached;
    TArray<FQuat> OrientationArray_Attached;
    TArray<FVector> ScaleArray_Attached;

    // Transform_Burst
    TArray<FVector> LocationArray_Burst;
    TArray<FQuat> OrientationArray_Burst;
    TArray<FVector> ScaleArray_Burst;

    FORCEINLINE void ResetBurstData()
    {
        LocationArray_Burst.Empty();
        OrientationArray_Burst.Empty();
        ScaleArray_Burst.Empty();
    }
    FFxRenderBatchData() {};

    FFxRenderBatchData(const FFxRenderBatchData& Data)
    {
        LockFlag.store(Data.LockFlag.load());

        SpawnedNiagaraSystem = Data.SpawnedNiagaraSystem;

        ValidTransforms_Attached = Data.ValidTransforms_Attached;
        FreeTransforms_Attached = Data.FreeTransforms_Attached;
        CoolDowns_Attached = Data.CoolDowns_Attached;
        LocationEventArray_Attached = Data.LocationEventArray_Attached;

        Transforms_Attached = Data.Transforms_Attached;
        LocationArray_Attached = Data.LocationArray_Attached;
        OrientationArray_Attached = Data.OrientationArray_Attached;
        ScaleArray_Attached = Data.ScaleArray_Attached;

        LocationArray_Burst = Data.LocationArray_Burst;
        OrientationArray_Burst = Data.OrientationArray_Burst;
        ScaleArray_Burst = Data.ScaleArray_Burst;
    }

    FFxRenderBatchData& operator=(const FFxRenderBatchData& Data)
    {
        LockFlag.store(Data.LockFlag.load());

        SpawnedNiagaraSystem = Data.SpawnedNiagaraSystem;

        ValidTransforms_Attached = Data.ValidTransforms_Attached;
        FreeTransforms_Attached = Data.FreeTransforms_Attached;
        CoolDowns_Attached = Data.CoolDowns_Attached;
        LocationEventArray_Attached = Data.LocationEventArray_Attached;

        Transforms_Attached = Data.Transforms_Attached;
        LocationArray_Attached = Data.LocationArray_Attached;
        OrientationArray_Attached = Data.OrientationArray_Attached;
        ScaleArray_Attached = Data.ScaleArray_Attached;

        LocationArray_Burst = Data.LocationArray_Burst;
        OrientationArray_Burst = Data.OrientationArray_Burst;
        ScaleArray_Burst = Data.ScaleArray_Burst;

        return *this;
    }
};