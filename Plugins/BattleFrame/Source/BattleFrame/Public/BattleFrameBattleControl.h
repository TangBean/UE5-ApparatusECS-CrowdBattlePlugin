/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

// C++
#include <utility>

// Unreal
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Math/UnrealMathUtility.h"

// Apparatus
#include "Machine.h"
#include "Mechanism.h"

// BattleFrame
#include "BattleFrameFunctionLibraryRT.h"
#include "BattleFrameStructs.h"
#include "BattleFrameEnums.h"

#include "Traits/Debuff.h"
#include "Traits/Animation.h"
#include "Traits/Trace.h"
#include "Traits/Damage.h"
#include "Traits/Avoidance.h"
#include "Traits/Collider.h"
#include "Traits/SubType.h"
#include "Traits/Move.h"
#include "Traits/Navigation.h"
#include "Traits/Death.h"
#include "Traits/Appear.h"
#include "Traits/Rendering.h"
#include "Traits/RenderBatchData.h"
#include "Traits/Attack.h"
#include "Traits/TemporalDamage.h"
#include "Traits/Hit.h"
#include "Traits/Health.h"
#include "Traits/HealthBar.h"
#include "Traits/TextPopUp.h"
#include "Traits/Slow.h"
#include "Traits/SpawningFx.h"
#include "Traits/Defence.h"
#include "Traits/SphereObstacle.h"
#include "Traits/Curves.h"
#include "Traits/Statistics.h"
#include "Traits/BindFlowField.h"
#include "Traits/Sleep.h"
#include "Traits/ActorSpawnConfig.h"
#include "Traits/SoundConfig.h"
#include "Traits/FxConfig.h"
#include "Traits/Activated.h"
#include "Traits/IsSubjective.h"
#include "Traits/OwnerSubject.h"
#include "Traits/Chase.h"
#include "Traits/Patrol.h"
#include "Traits/TextPopConfig.h"
#include "Traits/ProjectileConfig.h"
#include "Traits/MayDie.h"
#include "Traits/PrimaryType.h"
#include "Traits/Transform.h"

#include "BattleFrameBattleControl.generated.h"

// Forward Declearation
class UNeighborGridComponent;

UCLASS()
class BATTLEFRAME_API ABattleFrameBattleControl : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BattleFrame)
	int32 MaxThreadsAllowed = FMath::Clamp(FPlatformMisc::NumberOfWorkerThreadsToSpawn(), 1, FPlatformMisc::NumberOfCoresIncludingHyperthreads());

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BattleFrame)
	int32 MinBatchSizeAllowed = 100;

	int32 ThreadsCount = 1;
	int32 BatchSize = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BattleFrame)
	bool bGamePaused = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = BattleFrame)
	int32 AgentCount = 0;

	static ABattleFrameBattleControl* Instance;
	UWorld* CurrentWorld = nullptr;
	AMechanism* Mechanism = nullptr;
	TArray<UNeighborGridComponent*> NeighborGrids;

	TSet<int32> ExistingRenderers;
	FStreamableManager StreamableManager;

	// Agent Sub-Status Flags
	EFlagmarkBit AppearDissolveFlag = EFlagmarkBit::A;
	EFlagmarkBit DeathDissolveFlag = EFlagmarkBit::B;

	EFlagmarkBit HitGlowFlag = EFlagmarkBit::C;
	EFlagmarkBit HitJiggleFlag = EFlagmarkBit::D;
	EFlagmarkBit HitPoppingTextFlag = EFlagmarkBit::E;
	EFlagmarkBit HitDecideHealthFlag = EFlagmarkBit::F;
	EFlagmarkBit DeathDisableCollisionFlag = EFlagmarkBit::G;
	EFlagmarkBit RegisterMultipleFlag = EFlagmarkBit::H;

	EFlagmarkBit AppearAnimFlag = EFlagmarkBit::I;
	EFlagmarkBit AttackAnimFlag = EFlagmarkBit::J;
	EFlagmarkBit HitAnimFlag = EFlagmarkBit::K;
	EFlagmarkBit DeathAnimFlag = EFlagmarkBit::L;
	EFlagmarkBit FallAnimFlag = EFlagmarkBit::M;

	// Spawn Config Queue
	TQueue<FProjectileConfig, EQueueMode::Mpsc> ProjectileConfigQueue;
	TQueue<FActorSpawnConfig_Final, EQueueMode::Mpsc> ActorSpawnConfigQueue;
	TQueue<FFxConfig_Final, EQueueMode::Mpsc> FxConfigQueue;
	TQueue<FSoundConfig_Final, EQueueMode::Mpsc> SoundConfigQueue;

	// Event Interface Queue
	TQueue<FAppearData, EQueueMode::Mpsc> OnAppearQueue;
	TQueue<FTraceData, EQueueMode::Mpsc> OnTraceQueue;
	TQueue<FMoveData, EQueueMode::Mpsc> OnMoveQueue;
	TQueue<FAttackData, EQueueMode::Mpsc> OnAttackQueue;
	TQueue<FHitData, EQueueMode::Mpsc> OnHitQueue;
	TQueue<FDeathData, EQueueMode::Mpsc> OnDeathQueue;

	// Draw Debug Queue
	TQueue<FDebugPointConfig, EQueueMode::Mpsc> DebugPointQueue;
	TQueue<FDebugLineConfig, EQueueMode::Mpsc> DebugLineQueue;
	TQueue<FDebugSphereConfig, EQueueMode::Mpsc> DebugSphereQueue;
	TQueue<FDebugCapsuleConfig, EQueueMode::Mpsc> DebugCapsuleQueue;
	TQueue<FDebugSectorConfig, EQueueMode::Mpsc> DebugSectorQueue;
	TQueue<FDebugCircleConfig, EQueueMode::Mpsc> DebugCircleQueue;

private:

	// all filters we gonna use
	bool bIsFilterReady = false;
	FFilter AgentCountFilter;
	FFilter AgentStatFilter;
	FFilter AgentMayDieFilter;
	FFilter AgentAppeaFilter;
	FFilter AgentAppearAnimFilter;
	FFilter AgentAppearDissolveFilter;
	FFilter AgentTraceFilter;
	FFilter AgentAttackFilter;
	FFilter AgentAttackingFilter;
	FFilter SubjectBeingHitFilter;
	FFilter TemporalDamageFilter;
	FFilter SlowFilter;
	FFilter DecideHealthFilter;
	FFilter AgentHealthBarFilter;
	FFilter AgentDeathFilter;
	FFilter AgentDeathDissolveFilter;
	FFilter AgentDeathAnimFilter;
	FFilter SpeedLimitOverrideFilter;
	FFilter AgentSleepFilter;
	FFilter AgentPatrolFilter;
	FFilter AgentMoveFilter;
	FFilter IdleToMoveAnimFilter;
	FFilter AgentStateMachineFilter;
	FFilter RenderBatchFilter;
	FFilter AgentRenderFilter;
	FFilter TextRenderFilter;
	FFilter SpawnProjectileFilter;
	FFilter SpawnActorFilter;
	FFilter SpawnFxFilter;
	FFilter PlaySoundFilter;
	FFilter SubjectFilterBase;
	FFilter ProjectileFilter;


public:

	ABattleFrameBattleControl()
	{
		PrimaryActorTick.bCanEverTick = true;
	}

	void BeginPlay() override;

	void Tick(float DeltaTime) override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		if (Instance == this)
		{
			Instance = nullptr;
		}

		Super::EndPlay(EndPlayReason);
	}

	//---------------------------------------------Helpers------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static ABattleFrameBattleControl* GetInstance()
	{
		return Instance;
	}

	void DefineFilters();

	static FVector FindNewPatrolGoalLocation(const FPatrol Patrol, const FCollider Collider, const FTrace Trace, const FTracing Tracing, const FLocated Located, const FScaled Scaled, int32 MaxAttempts);

	static bool GetInterpedWorldLocation(AFlowField* flowField, const FVector& location, const float angleThreshold, FVector& outInterpolatedWorldLoc);

	static void DrawDebugSector(UWorld* World, const FVector& Center, const FVector& Direction, float Radius, float AngleDegrees, float Height, const FColor& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness);

	static void CopyPasteAnimData(FAnimating& Animating, int32 From, int32 To);

	FORCEINLINE void PlayAnimAsMontage(FAnimation& Animation, FAnimating& Animating, FMoving& Moving, bool bLooped, bool bUseAnimLength, float AnimLength, float PlayRate, int32 AnimIndex, float LerpSpeedMult, float SafeDeltaTime)
	{
		if (Animating.bUpdateAnimState)
		{
			if (Animating.PreviousAnimState == EAnimState::BS_IdleMove) // Use slot 2
			{
				Animating.CurrentMontageSlot = 2;

				if (Animating.AnimIndex2 != AnimIndex) // 不重置Lerp，这样可以保证动画过渡连续性
				{
					Animating.AnimLerp1 = 0;
				}

				Animating.AnimCurrentTime2 = GetGameTimeSinceCreation();
				Animating.AnimPauseFrame2 = bLooped ? 0 : Animating.AnimPauseFrameArray.Num() > AnimIndex ? Animating.AnimPauseFrameArray[AnimIndex] : 0;
			}
			else // Use slot 1
			{
				if (Animating.CurrentMontageSlot == 1)
				{
					CopyPasteAnimData(Animating, 1, 0);// copy anim from 1 to slot 0
				}
				else
				{
					CopyPasteAnimData(Animating, 2, 0);// copy anim from 2 to slot 0
					Animating.CurrentMontageSlot = 1;
				}

				// write Hit anim into slot 1
				Animating.AnimCurrentTime1 = GetGameTimeSinceCreation();
				Animating.AnimPauseFrame1 = bLooped ? 0 : Animating.AnimPauseFrameArray.Num() > AnimIndex ? Animating.AnimPauseFrameArray[AnimIndex] : 0;

				Animating.AnimLerp0 = 0;
				Animating.AnimLerp1 = 0;
			}

			Animating.PreviousAnimState = Animating.AnimState;
			Animating.bUpdateAnimState = false;
		}

		if (Animating.CurrentMontageSlot == 2)
		{
			Animating.AnimIndex2 = AnimIndex;
			Animating.AnimPlayRate2 = bUseAnimLength ? Animating.AnimPauseFrame2 / Animating.SampleRate / AnimLength : PlayRate;

			TRange<float> InputRange(Animation.BS_IdleMove[0], Animation.BS_IdleMove[1]);
			TRange<float> OutputRange(0, 1);
			float Input = Moving.CurrentVelocity.Size2D();
			float TargetLerp = FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Input);

			Animating.AnimLerp0 = FMath::FInterpConstantTo(Animating.AnimLerp0, TargetLerp, SafeDeltaTime, Animation.LerpSpeed);
			Animating.AnimLerp1 = FMath::Clamp(Animating.AnimLerp1 + SafeDeltaTime * Animation.LerpSpeed, 0, 1);
		}
		else
		{
			Animating.AnimIndex1 = AnimIndex;
			Animating.AnimPlayRate1 = bUseAnimLength ? Animating.AnimPauseFrame1 / Animating.SampleRate / AnimLength : PlayRate;

			// transit from slot 0 to slot 1 using AnimLerp0
			Animating.AnimLerp0 = FMath::Clamp(Animating.AnimLerp0 + SafeDeltaTime * Animation.LerpSpeed, 0, 1);
		}
	}

	FORCEINLINE std::pair<bool, float> ProcessCritDamage(float BaseDamage, float damageMult, float Probability)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ProcessCrit");
		float ActualDamage = BaseDamage;
		bool IsCritical = false;  // 是否暴击

		// 生成一个[0, 1]范围内的随机数
		float CritChance = FMath::FRand();

		// 判断是否触发暴击
		if (CritChance < Probability)
		{
			ActualDamage *= damageMult;  // 应用暴击倍数
			IsCritical = true;  // 设置暴击标志
		}

		return { IsCritical, ActualDamage };  // 返回pair
	}

	FORCEINLINE static FTransform LocalOffsetToWorld(FQuat WorldRotation, FVector WorldLocation, FTransform LocalTransform)
	{
		// 计算世界空间的位置偏移
		FVector WorldLocationOffset = WorldRotation.RotateVector(LocalTransform.GetLocation());
		FVector FinalLocation = WorldLocation + WorldLocationOffset;

		// 组合旋转（世界朝向 + 本地偏移旋转）
		FQuat FinalRotation = WorldRotation * LocalTransform.GetRotation();

		return FTransform(FinalRotation, FinalLocation, LocalTransform.GetScale3D());
	}

	FORCEINLINE void QueueText(FTextPopConfig Config) const
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("QueueText");

		if (Config.Owner.HasTrait<FPoppingText>())
		{
			auto& PoppingText = Config.Owner.GetTraitRef<FPoppingText, EParadigm::Unsafe>();

			PoppingText.Lock();
			PoppingText.TextLocationArray.Add(Config.Location);
			PoppingText.Text_Value_Style_Scale_Offset_Array.Add(FVector4(Config.Value, Config.Style, Config.Scale, Config.Radius));
			//UE_LOG(LogTemp, Warning, TEXT("OldTrait"));
			PoppingText.Unlock();

			Config.Owner.SetFlag(HitPoppingTextFlag, true);
		}
	}

	FORCEINLINE void ResetPatrol(FPatrol& Patrol, FPatrolling& Patrolling, const FLocated& Located)
	{
		// Reset timer values
		Patrolling.MoveTimeLeft = Patrol.MaxMovingTime;
		Patrolling.WaitTimeLeft = Patrol.CoolDown;

		// Reset origin based on mode
		switch (Patrol.OriginMode)
		{
			case EPatrolOriginMode::Previous:
				Patrol.Origin = Located.Location; // use current location
				break;

			case EPatrolOriginMode::Initial:
				Patrol.Origin = Located.InitialLocation; // use initial location
				break;
		}
	};


	//---------------------------------------------Damager------------------------------------------------------------------

	void ApplyPointDamageAndDebuff(const FSubjectArray& Subjects, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Point& Damage, const FDebuff_Point& Debuff, TArray<FDmgResult>& DamageResults);

	void ApplyPointDamageAndDebuffDeferred(const FSubjectArray& Subjects, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Point& Damage, const FDebuff_Point& Debuff, TArray<FDmgResult>& DamageResults);

	void ApplyRadialDamageAndDebuff(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& Origin, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Radial& Damage, const FDebuff_Radial& Debuff, TArray<FDmgResult>& DamageResults);

	void ApplyRadialDamageAndDebuffDeferred(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& Origin, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Radial& Damage, const FDebuff_Radial& Debuff, TArray<FDmgResult>& DamageResults);

	void ApplyBeamDamageAndDebuff(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& StartLocation, const FVector& EndLocation, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Beam& Damage, const FDebuff_Beam& Debuff, TArray<FDmgResult>& DamageResults);

	void ApplyBeamDamageAndDebuffDeferred(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& StartLocation, const FVector& EndLocation, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Beam& Damage, const FDebuff_Beam& Debuff, TArray<FDmgResult>& DamageResults);

	
	//-------------------------------------------Pack Data------------------------------------------------------------------
	
	// PackData ：三个AnimIndex,整数,各分配10位
	FORCEINLINE static float EncodeAnimationIndices(int AnimIndex0, int AnimIndex1, int AnimIndex2)
	{
		// 确保输入在有效范围内 (0-1023)
		AnimIndex0 = FMath::Clamp(AnimIndex0, 0, 1023);
		AnimIndex1 = FMath::Clamp(AnimIndex1, 0, 1023);
		AnimIndex2 = FMath::Clamp(AnimIndex2, 0, 1023);

		// 位组合：| 未使用 | AnimIndex2 | AnimIndex1 | AnimIndex0 |
		uint32 packed = (static_cast<uint32>(AnimIndex2) << 20) | (static_cast<uint32>(AnimIndex1) << 10) | static_cast<uint32>(AnimIndex0);

		return *reinterpret_cast<float*>(&packed);
	}

	// PackData ：三个AnimPauseFrame,整数,各分配10位
	FORCEINLINE static float EncodePauseFrames(int Frame0, int Frame1, int Frame2)
	{
		Frame0 = FMath::Clamp(Frame0, 0, 1023);
		Frame1 = FMath::Clamp(Frame1, 0, 1023);
		Frame2 = FMath::Clamp(Frame2, 0, 1023);

		uint32 packed = (static_cast<uint32>(Frame2) << 20) |
			(static_cast<uint32>(Frame1) << 10) |
			static_cast<uint32>(Frame0);

		return *reinterpret_cast<float*>(&packed);
	}

	// PackData ：三个AnimPlayrate,浮点,各分配10位,数值范围(0-10 → 0-1023)
	FORCEINLINE static float EncodePlayRates(float Rate0, float Rate1, float Rate2)
	{
		// 线性映射到整数范围
		const float Scale = 1023.0f / 10.0f;
		uint32 iRate0 = static_cast<uint32>(FMath::Clamp(Rate0, 0.0f, 10.0f) * Scale);
		uint32 iRate1 = static_cast<uint32>(FMath::Clamp(Rate1, 0.0f, 10.0f) * Scale);
		uint32 iRate2 = static_cast<uint32>(FMath::Clamp(Rate2, 0.0f, 10.0f) * Scale);

		uint32 packed = (iRate2 << 20) | (iRate1 << 10) | iRate0;
		return *reinterpret_cast<float*>(&packed);
	}

	// PackData ：四个MaterialFx,浮点,各分配8位,数值范围(0-1 → 0-255)
	FORCEINLINE static float EncodeStatusEffects(float HitGlow, float Frozen, float Burning, float Poisoned)
	{
		const float Scale = 255.0f;
		uint32 iHit = static_cast<uint32>(FMath::Clamp(HitGlow, 0.0f, 1.0f) * Scale);
		uint32 iFrozen = static_cast<uint32>(FMath::Clamp(Frozen, 0.0f, 1.0f) * Scale);
		uint32 iBurning = static_cast<uint32>(FMath::Clamp(Burning, 0.0f, 1.0f) * Scale);
		uint32 iPoison = static_cast<uint32>(FMath::Clamp(Poisoned, 0.0f, 1.0f) * Scale);

		// 位组合：| Poisoned | Burning | Frozen | HitGlow |
		uint32 packed = (iPoison << 24) | (iBurning << 16) | (iFrozen << 8) | iHit;
		return *reinterpret_cast<float*>(&packed);
	}


	//--------------------------------------------A Star------------------------------------------------------------------

	bool FindPathAStar(AFlowField* FlowField, const FVector& StartLocation, const FVector& GoalLocation, TArray<FVector>& OutPath);

	void GetSteeringDirection(const FVector& CurrentLocation, const FVector& GoalLocation, const TArray<FVector>& PathPoints, float MoveSpeed, float LookAheadDistance, float PathRadius, float AcceptanceRadius, FVector& SteeringDirection, bool& bHasPath, bool& bIsCurrentNearPath, bool& bIsGoalNearEnd);

	FVector FindClosestPointOnSegment(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd);


	//---------------------------------------------RVO2------------------------------------------------------------------

	static void ComputeAvoidingVelocity(FAvoidance& Avoidance, FAvoiding& Avoiding, const TArray<FGridData>& SubjectNeighbors, const TArray<FGridData>& ObstacleNeighbors, float TimeStep);

	static bool LinearProgram1(const std::vector<RVO::Line>& lines, size_t lineNo, float radius, const RVO::Vector2& optVelocity, bool directionOpt, RVO::Vector2& result);

	static size_t LinearProgram2(const std::vector<RVO::Line>& lines, float radius, const RVO::Vector2& optVelocity, bool directionOpt, RVO::Vector2& result);

	static void LinearProgram3(const std::vector<RVO::Line>& lines, size_t numObstLines, size_t beginLine, float radius, RVO::Vector2& result);

};
