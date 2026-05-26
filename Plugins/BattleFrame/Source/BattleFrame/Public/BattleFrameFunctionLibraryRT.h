// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SubjectHandle.h"
#include "Traits/SubType.h"
#include "Math/Vector2D.h"
#include "Traits/Debuff.h"
#include "Traits/AvoGroup.h"
#include "Traits/Team.h"
#include "Traits/PrimaryType.h"
#include "Traits/Transform.h"
#include "BattleFrameEnums.h"
#include "BattleFrameStructs.h"
#include "NeighborGridCell.h"
#include "ProjectileConfigDataAsset.h"
#include "AgentConfigDataAsset.h"
#include "AgentSpawner.h"
#include "BattleFrameFunctionLibraryRT.generated.h"

class ABattleFrameBattleControl;
class ANeighborGridActor;
class UNeighborGridComponent;
class UAgentConfigDataAsset;
class UProjectileConfigDataAsset;

UCLASS()
class BATTLEFRAME_API UBattleFrameFunctionLibraryRT : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

    //---------------------------------Spawning-------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Spawning", meta = (AutoCreateRefTerm = "Origin,Region,LaunchVelocity,CustomDirection,Multipliers", DisplayName = "SpawnAgentsByDataAsset", Keywords = "Spawn Agents By Data Asset"))
    static TArray<FSubjectHandle> SpawnAgentsByConfigRectangular
    (
        AAgentSpawner* AgentSpawner = nullptr,
        bool bAutoActivate = true,
        TSoftObjectPtr<UAgentConfigDataAsset> DataAsset = nullptr,
        int32 Quantity = 1, 
        int32 Team = 0, 
        UPARAM(ref) const FVector& Origin = FVector(0, 0, 0),
        UPARAM(ref) const FVector2D& Region = FVector2D(0, 0),
        UPARAM(ref) const FVector2D& LaunchVelocity = FVector2D(0, 0),
        EInitialDirection InitialDirection = EInitialDirection::FacePlayer,
        UPARAM(ref) const FVector2D& CustomDirection = FVector2D(1, 0),
        UPARAM(ref) const  FSpawnerMult& Multipliers = FSpawnerMult()
    );

    //-------------------------------Apply Dmg and Debuff-------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Damage and Debuff", meta = (AutoCreateRefTerm = "Subjects, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff", Keywords = "Apply Point Damage Debuff", DisplayName = "ApplyPointDamageAndDebuff"))
    static void ApplyPointDamageAndDebuff
    (
        TArray<FDmgResult>& DamageResults,
        ABattleFrameBattleControl* BattleControl = nullptr,
        UPARAM(ref) const FSubjectArray& Subjects = FSubjectArray(),
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FSubjectHandle DmgInstigator = FSubjectHandle(),
        UPARAM(ref) const FSubjectHandle DmgCauser = FSubjectHandle(),
        UPARAM(ref) const FVector& HitFromLocation = FVector::ZeroVector,
        UPARAM(ref) const FDamage_Point& Damage = FDamage_Point(),
        UPARAM(ref) const FDebuff_Point& Debuff = FDebuff_Point()
    );

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Damage and Debuff", meta = (AutoCreateRefTerm = "KeepCount, Origin, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff, DmgFilter", Keywords = "Apply Radial Damage Debuff"))
    static void ApplyRadialDamageAndDebuff
    (
        TArray<FDmgResult>& DamageResults,
        ABattleFrameBattleControl* BattleControl = nullptr,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        UPARAM(ref) const int32 KeepCount = -1,
        UPARAM(ref) const FVector& Origin = FVector::ZeroVector,
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FSubjectHandle DmgInstigator = FSubjectHandle(),
        UPARAM(ref) const FSubjectHandle DmgCauser = FSubjectHandle(),
        UPARAM(ref) const FVector& HitFromLocation = FVector::ZeroVector,
        UPARAM(ref) const FDamage_Radial& Damage = FDamage_Radial(),
        UPARAM(ref) const FDebuff_Radial& Debuff = FDebuff_Radial()
    );

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Damage and Debuff", meta = (AutoCreateRefTerm = "KeepCount, StartLocation, EndLocation, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff, DmgFilter", Keywords = "Apply Beam Damage Debuff"))
    static void ApplyBeamDamageAndDebuff
    (
        TArray<FDmgResult>& DamageResults,
        ABattleFrameBattleControl* BattleControl = nullptr,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        UPARAM(ref) const int32 KeepCount = -1,
        UPARAM(ref) const FVector& StartLocation = FVector::ZeroVector,
        UPARAM(ref) const FVector& EndLocation = FVector::ZeroVector,
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FSubjectHandle DmgInstigator = FSubjectHandle(),
        UPARAM(ref) const FSubjectHandle DmgCauser = FSubjectHandle(),
        UPARAM(ref) const FVector& HitFromLocation = FVector::ZeroVector,
        UPARAM(ref) const FDamage_Beam& Damage = FDamage_Beam(),
        UPARAM(ref) const FDebuff_Beam& Debuff = FDebuff_Beam()
    );

    //-----------------------------------Misc-------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Misc", meta = (DisplayName = "Sort Subjects By Distance", Keywords = "Sort Distance Subject", AutoCreateRefTerm = "TraceResults"))
    static void SortSubjectsByDistance(UPARAM(ref) TArray<FTraceResult>& TraceResults,const FVector& SortOrigin,const ESortMode SortMode = ESortMode::NearToFar);

    static void CalculateThreadsCountAndBatchSize(int32 IterableNum, int32 MaxThreadsAllowed, int32 MinBatchSizeAllowed, int32& ThreadsCount, int32& BatchSize);


    //---------------------------------Navigation-------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Navigation", meta = (DisplayName = "WorldToIndexByRadius", Keywords = "World To Index Radius", AutoCreateRefTerm = ""))
    static TArray<int32> WorldToIndexByRadius(AFlowField* FlowField, const FVector& Location, const float Radius)
    {
        TArray<int32> CoveredIndices;

        if (!FlowField) return CoveredIndices;

        if (!FlowField->bIsBeginPlay)
        {
            const float RadiusSq = Radius * Radius;
            const float HalfCellSize = FlowField->cellSize * 0.5f;
            const float CellBoundOffset = HalfCellSize + Radius;

            // 转换到相对坐标系
            FVector RelativeCenter = (Location - FlowField->actorLoc).RotateAngleAxis(-FlowField->actorRot.Yaw, FVector(0, 0, 1)) + FlowField->offsetLoc;

            // 计算网格坐标范围
            const int32 MinGridX = FMath::Clamp(FMath::FloorToInt((RelativeCenter.X - CellBoundOffset) / FlowField->cellSize), 0, FlowField->xNum - 1);
            const int32 MaxGridX = FMath::Clamp(FMath::CeilToInt((RelativeCenter.X + CellBoundOffset) / FlowField->cellSize), 0, FlowField->xNum - 1);
            const int32 MinGridY = FMath::Clamp(FMath::FloorToInt((RelativeCenter.Y - CellBoundOffset) / FlowField->cellSize), 0, FlowField->yNum - 1);
            const int32 MaxGridY = FMath::Clamp(FMath::CeilToInt((RelativeCenter.Y + CellBoundOffset) / FlowField->cellSize), 0, FlowField->yNum - 1);

            // 遍历所有可能相交的网格
            for (int32 x = MinGridX; x <= MaxGridX; ++x)
            {
                for (int32 y = MinGridY; y <= MaxGridY; ++y)
                {
                    const int32 Index = FlowField->CoordToIndex(FVector2D(x, y));

                    if (Index >= 0 && Index < FlowField->CurrentCellsArray.Num())
                    {
                        const FCellStruct& Cell = FlowField->CurrentCellsArray[Index];

                        // 计算格子边界
                        const float CellMinX = Cell.worldLoc.X - HalfCellSize;
                        const float CellMaxX = Cell.worldLoc.X + HalfCellSize;
                        const float CellMinY = Cell.worldLoc.Y - HalfCellSize;
                        const float CellMaxY = Cell.worldLoc.Y + HalfCellSize;

                        // 计算最近点并检查相交
                        const float ClosestX = FMath::Clamp(Location.X, CellMinX, CellMaxX);
                        const float ClosestY = FMath::Clamp(Location.Y, CellMinY, CellMaxY);

                        const float Dx = Location.X - ClosestX;
                        const float Dy = Location.Y - ClosestY;

                        if ((Dx * Dx + Dy * Dy) <= RadiusSq)
                        {
                            CoveredIndices.Add(Index);
                        }
                    }
                }
            }
        }

        return CoveredIndices;
    }

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Navigation", meta = (DisplayName = "WorldToGirdByRadius", Keywords = "World To Gird Radius", AutoCreateRefTerm = ""))
    static TArray<FVector2D> WorldToGirdByRadius(AFlowField* FlowField, const FVector& Location, const float Radius)
    {
        TArray<FVector2D> CoveredCoords;

        if (!FlowField) return CoveredCoords;

        if (!FlowField->bIsBeginPlay)
        {
            const float RadiusSq = Radius * Radius;
            const float HalfCellSize = FlowField->cellSize * 0.5f;
            const float CellBoundOffset = HalfCellSize + Radius;

            // 转换到相对坐标系
            FVector RelativeCenter = (Location - FlowField->actorLoc).RotateAngleAxis(-FlowField->actorRot.Yaw, FVector(0, 0, 1)) + FlowField->offsetLoc;

            // 计算网格坐标范围
            const int32 MinGridX = FMath::Clamp(FMath::FloorToInt((RelativeCenter.X - CellBoundOffset) / FlowField->cellSize), 0, FlowField->xNum - 1);
            const int32 MaxGridX = FMath::Clamp(FMath::CeilToInt((RelativeCenter.X + CellBoundOffset) / FlowField->cellSize), 0, FlowField->xNum - 1);
            const int32 MinGridY = FMath::Clamp(FMath::FloorToInt((RelativeCenter.Y - CellBoundOffset) / FlowField->cellSize), 0, FlowField->yNum - 1);
            const int32 MaxGridY = FMath::Clamp(FMath::CeilToInt((RelativeCenter.Y + CellBoundOffset) / FlowField->cellSize), 0, FlowField->yNum - 1);

            // 遍历所有可能相交的网格
            for (int32 x = MinGridX; x <= MaxGridX; ++x)
            {
                for (int32 y = MinGridY; y <= MaxGridY; ++y)
                {
                    const int32 Index = FlowField->CoordToIndex(FVector2D(x, y));

                    if (Index >= 0 && Index < FlowField->CurrentCellsArray.Num())
                    {
                        const FCellStruct& Cell = FlowField->CurrentCellsArray[Index];

                        // 计算格子边界
                        const float CellMinX = Cell.worldLoc.X - HalfCellSize;
                        const float CellMaxX = Cell.worldLoc.X + HalfCellSize;
                        const float CellMinY = Cell.worldLoc.Y - HalfCellSize;
                        const float CellMaxY = Cell.worldLoc.Y + HalfCellSize;

                        // 计算最近点并检查相交
                        const float ClosestX = FMath::Clamp(Location.X, CellMinX, CellMaxX);
                        const float ClosestY = FMath::Clamp(Location.Y, CellMinY, CellMaxY);

                        const float Dx = Location.X - ClosestX;
                        const float Dy = Location.Y - ClosestY;

                        if ((Dx * Dx + Dy * Dy) <= RadiusSq)
                        {
                            CoveredCoords.Add(FVector2D(x, y));
                        }
                    }
                }
            }
        }

        return CoveredCoords;
    }


    //----------------------------------Projectile------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Solve, Projectile, Pitch"))
    static void SolveProjectileVelocityFromPitch(bool& Succeed, FVector& LaunchVelocity, FVector FromPoint, FVector ToPoint, float Gravity, float PitchAngle);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Solve, Projectile, Speed"))
    static void SolveProjectileVelocityFromSpeed(bool& Succeed, FVector& LaunchVelocity, FVector FromPoint, FVector ToPoint, float Gravity, float Speed, bool bFavorHighArc);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Solve, Projectile, Pitch, Prediction"))
    static void SolveProjectileVelocityFromPitchWithPrediction(bool& Succeed, FVector& LaunchVelocity, FVector FromPoint, FVector ToPoint, FVector TargetVelocity, int32 Iterations, float Gravity, float PitchAngle);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Solve, Projectile, Speed, Prediction"))
    static void SolveProjectileVelocityFromSpeedWithPrediction(bool& Succeed, FVector& LaunchVelocity, FVector FromPoint, FVector ToPoint, FVector TargetVelocity, int32 Iterations, float Gravity, float Speed, bool bFavorHighArc);

    static void SpawnProjectileByConfig(bool& Successful, FSubjectHandle& SpawnedProjectile, UProjectileConfigDataAsset* ProjectileConfigDataAsset);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Set,Projectile,Movement,RuntimeData,Static"))
    static void SpawnProjectile_Static(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Set,Projectile,Movement,RuntimeData,Interped"))
    static void SpawnProjectile_Interped(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, float XOffsetMult, float YOffsetMult, float ZOffsetMult, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Set,Projectile,Movement,RuntimeData,Ballistic"))
    static void SpawnProjectile_Ballistic(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, FVector TargetVelocity, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects);

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Projectile", meta = (Keywords = "Set,Projectile,Movement,RuntimeData,Tracking"))
    static void SpawnProjectile_Tracking(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, FVector Direction, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects);

    static void GetProjectilePositionAtTime_Interped(bool& bHasArrived, FVector& CurrentLocation, FVector FromPoint, FVector& ToPoint, FSubjectHandle ToTarget, FRuntimeFloatCurve XOffset, float XOffsetMult, FRuntimeFloatCurve YOffset, float YOffsetMult, FRuntimeFloatCurve ZOffset, float ZOffsetMult, float InitialTime, float CurrentTime, float Speed);

    static void GetProjectilePositionAtTime_Ballistic(bool& bHasArrived, FVector& CurrentLocation, FVector FromPoint, FVector ToPoint, float InitialTime, float CurrentTime, float Gravity, FVector InitialVelocity);

    static void GetProjectilePositionAtTime_Tracking(bool& bHasArrived, FVector& CurrentLocation, FVector& CurrentVelocity, FVector FromPoint, FVector& ToPoint, FSubjectHandle ToTarget, float Acceleration, float MaxSpeed, float DeltaTime, float ArrivalThreshold);


    //-------------------------------Sync Trace-------------------------------

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Tracing", meta = (AutoCreateRefTerm = "TraceOrigin,CheckOrigin,SortOrigin,IgnoreSubjects,Filter,DrawDebugConfig"))
    static void SphereTraceForSubjects
    (
        bool& Hit,
        TArray<FTraceResult>& TraceResults,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        int32 KeepCount = -1,
        UPARAM(ref) const FVector& TraceOrigin = FVector(0, 0, 0),
        float Radius = 0.f,
        bool bCheckObstacle = false,
        UPARAM(ref) const FVector& CheckOrigin = FVector(0, 0, 0),
        float CheckRadius = 0.f,
        ESortMode SortMode = ESortMode::None,
        UPARAM(ref) const FVector& SortOrigin = FVector(0, 0, 0),
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FBFFilter& Filter = FBFFilter(),
        UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig = FTraceDrawDebugConfig()
    );

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Tracing", meta = (AutoCreateRefTerm = "TraceStart,TraceEnd,CheckOrigin,SortOrigin,IgnoreSubjects,Filter,DrawDebugConfig"))
    static void SphereSweepForSubjects
    (
        bool& Hit,
        TArray<FTraceResult>& TraceResults,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        int32 KeepCount = -1,
        UPARAM(ref) const FVector& TraceStart = FVector(0, 0, 0),
        UPARAM(ref) const FVector& TraceEnd = FVector(0, 0, 0),
        float Radius = 0.f,
        bool bCheckObstacle = false,
        UPARAM(ref) const FVector& CheckOrigin = FVector(0, 0, 0),
        float CheckRadius = 0.f,
        ESortMode SortMode = ESortMode::None,
        UPARAM(ref) const FVector& SortOrigin = FVector(0, 0, 0),
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FBFFilter& Filter = FBFFilter(),
        UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig = FTraceDrawDebugConfig()
    );

    UFUNCTION(BlueprintCallable, Category = "BattleFrame | Tracing", meta = (AutoCreateRefTerm = "TraceOrigin,ForwardVector,CheckOrigin,SortOrigin,IgnoreSubjects,Filter,DrawDebugConfig"))
    static void SectorTraceForSubjects
    (
        bool& Hit,
        TArray<FTraceResult>& TraceResults,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        int32 KeepCount = -1,
        UPARAM(ref) const FVector& TraceOrigin = FVector(0, 0, 0),
        float Radius = 300.f,
        float Height = 100.f,
        UPARAM(ref) const FVector& ForwardVector = FVector(1, 0, 0),
        float Angle = 360.f,
        bool bCheckObstacle = false,
        UPARAM(ref) const FVector& CheckOrigin = FVector(0, 0, 0),
        float CheckRadius = 0.f,
        ESortMode SortMode = ESortMode::None,
        UPARAM(ref) const FVector& SortOrigin = FVector(0, 0, 0),
        UPARAM(ref) const FSubjectArray& IgnoreSubjects = FSubjectArray(),
        UPARAM(ref) const FBFFilter& Filter = FBFFilter(),
        UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig = FTraceDrawDebugConfig()
    );

    //UFUNCTION(BlueprintCallable, Category = "BattleFrame | Tracing", meta = (AutoCreateRefTerm = "TraceStart,TraceEnd,DrawDebugConfig"))
    //static void SphereSweepForObstacle
    //(
    //    bool& Hit,
    //    FTraceResult& TraceResult,
    //    UNeighborGridComponent* NeighborGridComponent = nullptr,
    //    UPARAM(ref) const FVector& TraceStart = FVector(0, 0, 0),
    //    UPARAM(ref) const FVector& TraceEnd = FVector(0, 0, 0),
    //    float Radius = 0.f,
    //    UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig = FTraceDrawDebugConfig()
    //);

    //-------------------------------Connector Nodes-------------------------------

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Convert to SubjectHandles", CompactNodeTitle = "->", BlueprintAutocast), Category = "BattleFrame | Adapters")
    static TArray<FSubjectHandle> ConvertDmgResultsToSubjectHandles(const TArray<FDmgResult>& DmgResults);

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Convert to SubjectHandles", CompactNodeTitle = "->", BlueprintAutocast), Category = "BattleFrame | Adapters")
    static TArray<FSubjectHandle> ConvertTraceResultsToSubjectHandles(const TArray<FTraceResult>& TraceResults);

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Convert to SubjectArray", CompactNodeTitle = "->", BlueprintAutocast), Category = "BattleFrame | Adapters")
    static FSubjectArray ConvertDmgResultsToSubjectArray(const TArray<FDmgResult>& DmgResults);

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Convert to SubjectArray", CompactNodeTitle = "->", BlueprintAutocast), Category = "BattleFrame | Adapters")
    static FSubjectArray ConvertTraceResultsToSubjectArray(const TArray<FTraceResult>& TraceResults);

    UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Convert to SubjectArray", CompactNodeTitle = "->", BlueprintAutocast), Category = "BattleFrame | Adapters")
    static FSubjectArray ConvertSubjectHandlesToSubjectArray(const TArray<FSubjectHandle>& SubjectHandles);


    //-------------------------------Trait Setters-------------------------------

    static void SetRecordSubTypeTraitByIndex(int32 Index, FSubjectRecord& SubjectRecord);
    static void SetRecordSubTypeTraitByEnum(EESubType SubType, FSubjectRecord& SubjectRecord);
    static void RemoveSubjectSubTypeTraitByIndex(int32 Index, FSubjectHandle SubjectHandle);
    static void SetSubjectSubTypeTraitByIndex(int32 Index, FSubjectHandle SubjectHandle);
    static void IncludeSubTypeTraitByIndex(int32 Index, FFilter& Filter);

    FORCEINLINE static void RemoveSubjectAvoGroupTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
    {
        switch (Index)
        {
        default:
            SubjectHandle.RemoveTrait<FAvoGroup0>();
            break;
        case 0:
            SubjectHandle.RemoveTrait<FAvoGroup0>();
            break;
        case 1:
            SubjectHandle.RemoveTrait<FAvoGroup1>();
            break;
        case 2:
            SubjectHandle.RemoveTrait<FAvoGroup2>();
            break;
        case 3:
            SubjectHandle.RemoveTrait<FAvoGroup3>();
            break;
        case 4:
            SubjectHandle.RemoveTrait<FAvoGroup4>();
            break;
        case 5:
            SubjectHandle.RemoveTrait<FAvoGroup5>();
            break;
        case 6:
            SubjectHandle.RemoveTrait<FAvoGroup6>();
            break;
        case 7:
            SubjectHandle.RemoveTrait<FAvoGroup7>();
            break;
        case 8:
            SubjectHandle.RemoveTrait<FAvoGroup8>();
            break;
        case 9:
            SubjectHandle.RemoveTrait<FAvoGroup9>();
            break;
        }
    }

    FORCEINLINE static void SetSubjectAvoGroupTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
    {
        switch (Index)
        {
        default:
            SubjectHandle.SetTrait(FAvoGroup0());
            break;
        case 0:
            SubjectHandle.SetTrait(FAvoGroup0());
            break;
        case 1:
            SubjectHandle.SetTrait(FAvoGroup1());
            break;
        case 2:
            SubjectHandle.SetTrait(FAvoGroup2());
            break;
        case 3:
            SubjectHandle.SetTrait(FAvoGroup3());
            break;
        case 4:
            SubjectHandle.SetTrait(FAvoGroup4());
            break;
        case 5:
            SubjectHandle.SetTrait(FAvoGroup5());
            break;
        case 6:
            SubjectHandle.SetTrait(FAvoGroup6());
            break;
        case 7:
            SubjectHandle.SetTrait(FAvoGroup7());
            break;
        case 8:
            SubjectHandle.SetTrait(FAvoGroup8());
            break;
        case 9:
            SubjectHandle.SetTrait(FAvoGroup9());
            break;
        }
    };

    FORCEINLINE static void RemoveSubjectTeamTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
    {
        switch (Index)
        {
        default:
            SubjectHandle.RemoveTrait<FTeam0>();
            break;
        case 0:
            SubjectHandle.RemoveTrait<FTeam0>();
            break;
        case 1:
            SubjectHandle.RemoveTrait<FTeam1>();
            break;
        case 2:
            SubjectHandle.RemoveTrait<FTeam2>();
            break;
        case 3:
            SubjectHandle.RemoveTrait<FTeam3>();
            break;
        case 4:
            SubjectHandle.RemoveTrait<FTeam4>();
            break;
        case 5:
            SubjectHandle.RemoveTrait<FTeam5>();
            break;
        case 6:
            SubjectHandle.RemoveTrait<FTeam6>();
            break;
        case 7:
            SubjectHandle.RemoveTrait<FTeam7>();
            break;
        case 8:
            SubjectHandle.RemoveTrait<FTeam8>();
            break;
        case 9:
            SubjectHandle.RemoveTrait<FTeam9>();
            break;
        }
    }

    FORCEINLINE static void SetSubjectTeamTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
    {
        switch (Index)
        {
        default:
            SubjectHandle.SetTrait(FTeam0());
            break;
        case 0:
            SubjectHandle.SetTrait(FTeam0());
            break;
        case 1:
            SubjectHandle.SetTrait(FTeam1());
            break;
        case 2:
            SubjectHandle.SetTrait(FTeam2());
            break;
        case 3:
            SubjectHandle.SetTrait(FTeam3());
            break;
        case 4:
            SubjectHandle.SetTrait(FTeam4());
            break;
        case 5:
            SubjectHandle.SetTrait(FTeam5());
            break;
        case 6:
            SubjectHandle.SetTrait(FTeam6());
            break;
        case 7:
            SubjectHandle.SetTrait(FTeam7());
            break;
        case 8:
            SubjectHandle.SetTrait(FTeam8());
            break;
        case 9:
            SubjectHandle.SetTrait(FTeam9());
            break;
        }
    };

    FORCEINLINE static void SetRecordAvoGroupTraitByIndex(int32 Index, FSubjectRecord& SubjectRecord)
    {
        switch (Index)
        {
        default:
            SubjectRecord.SetTrait(FAvoGroup0());
            break;
        case 0:
            SubjectRecord.SetTrait(FAvoGroup0());
            break;
        case 1:
            SubjectRecord.SetTrait(FAvoGroup1());
            break;
        case 2:
            SubjectRecord.SetTrait(FAvoGroup2());
            break;
        case 3:
            SubjectRecord.SetTrait(FAvoGroup3());
            break;
        case 4:
            SubjectRecord.SetTrait(FAvoGroup4());
            break;
        case 5:
            SubjectRecord.SetTrait(FAvoGroup5());
            break;
        case 6:
            SubjectRecord.SetTrait(FAvoGroup6());
            break;
        case 7:
            SubjectRecord.SetTrait(FAvoGroup7());
            break;
        case 8:
            SubjectRecord.SetTrait(FAvoGroup8());
            break;
        case 9:
            SubjectRecord.SetTrait(FAvoGroup9());
            break;
        }
    };

    FORCEINLINE static void SetRecordTeamTraitByIndex(int32 Index, FSubjectRecord& SubjectRecord)
    {
        switch (Index)
        {
        default:
            SubjectRecord.SetTrait(FTeam0());
            break;
        case 0:
            SubjectRecord.SetTrait(FTeam0());
            break;
        case 1:
            SubjectRecord.SetTrait(FTeam1());
            break;
        case 2:
            SubjectRecord.SetTrait(FTeam2());
            break;
        case 3:
            SubjectRecord.SetTrait(FTeam3());
            break;
        case 4:
            SubjectRecord.SetTrait(FTeam4());
            break;
        case 5:
            SubjectRecord.SetTrait(FTeam5());
            break;
        case 6:
            SubjectRecord.SetTrait(FTeam6());
            break;
        case 7:
            SubjectRecord.SetTrait(FTeam7());
            break;
        case 8:
            SubjectRecord.SetTrait(FTeam8());
            break;
        case 9:
            SubjectRecord.SetTrait(FTeam9());
            break;
        }
    };

    FORCEINLINE static void IncludeAvoGroupTraitByIndex(int32 Index, FFilter& Filter)
    {
        switch (Index)
        {
        case 0:
            Filter.Include<FAvoGroup0>();
            break;

        case 1:
            Filter.Include<FAvoGroup1>();
            break;

        case 2:
            Filter.Include<FAvoGroup2>();
            break;

        case 3:
            Filter.Include<FAvoGroup3>();
            break;

        case 4:
            Filter.Include<FAvoGroup4>();
            break;

        case 5:
            Filter.Include<FAvoGroup5>();
            break;

        case 6:
            Filter.Include<FAvoGroup6>();
            break;

        case 7:
            Filter.Include<FAvoGroup7>();
            break;

        case 8:
            Filter.Include<FAvoGroup8>();
            break;

        case 9:
            Filter.Include<FAvoGroup9>();
            break;
        }
    };

    FORCEINLINE static void ExcludeAvoGroupTraitByIndex(int32 Index, FFilter& Filter)
    {
        switch (Index)
        {
        case 0:
            Filter.Exclude<FAvoGroup0>();
            break;

        case 1:
            Filter.Exclude<FAvoGroup1>();
            break;

        case 2:
            Filter.Exclude<FAvoGroup2>();
            break;

        case 3:
            Filter.Exclude<FAvoGroup3>();
            break;

        case 4:
            Filter.Exclude<FAvoGroup4>();
            break;

        case 5:
            Filter.Exclude<FAvoGroup5>();
            break;

        case 6:
            Filter.Exclude<FAvoGroup6>();
            break;

        case 7:
            Filter.Exclude<FAvoGroup7>();
            break;

        case 8:
            Filter.Exclude<FAvoGroup8>();
            break;

        case 9:
            Filter.Exclude<FAvoGroup9>();
            break;
        }
    };

};

//-------------------------------Async Trace-------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncTraceOutput, bool, Hit, const TArray<FTraceResult>&, TraceResults);

UCLASS()
class BATTLEFRAME_API USphereSweepForSubjectsAsyncAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintAssignable)
    FAsyncTraceOutput Completed;

    TObjectPtr<UWorld> CurrentWorld;
    TWeakObjectPtr<UNeighborGridComponent> NeighborGrid;
    TArray<FNeighborGridCell> ValidCells;

    int32 KeepCount;
    FVector Start;
    FVector End;
    float Radius;
    bool bCheckObstacle;
    FVector CheckOrigin;
    float CheckRadius;
    ESortMode SortMode;
    FVector SortOrigin;
    FBFFilter Filter;
    FTraceDrawDebugConfig DrawDebugConfig;
    FSubjectArray IgnoreSubjects;
    bool Hit;
    TArray<FHitResult> VisibilityResults;
    TArray<FTraceResult> TempResults;
    TArray<FTraceResult> Results;

    FFilter SubjectFilter;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "TraceStart, TraceEnd, CheckOrigin, SortOrigin, CheckObjectTypes, IgnoreSubjects, Filter, DrawDebugConfig"))
    static USphereSweepForSubjectsAsyncAction* SphereSweepForSubjectsAsync
    (
        const UObject* WorldContextObject = nullptr,
        UNeighborGridComponent* NeighborGridComponent = nullptr,
        int32 KeepCount = -1,
        const FVector TraceStart = FVector(0, 0, 0),
        const FVector TraceEnd = FVector(0, 0, 0),
        float Radius = 0.f,
        bool bCheckObstacle = false,
        const FVector CheckOrigin = FVector(0, 0, 0),
        float CheckRadius = 0.f,
        ESortMode SortMode = ESortMode::None,
        const FVector SortOrigin = FVector(0, 0, 0),
        const FSubjectArray IgnoreSubjects = FSubjectArray(),
        const FBFFilter DmgFilter = FBFFilter(),
        const FTraceDrawDebugConfig DrawDebugConfig = FTraceDrawDebugConfig()
    );

    virtual void Activate() override;
};
