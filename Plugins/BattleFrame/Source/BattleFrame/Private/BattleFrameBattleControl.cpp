/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#include "BattleFrameBattleControl.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include <queue>

// Niagara 插件
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

// Apparatus 插件
#include "SubjectiveActorComponent.h"

// FlowFieldCanvas 插件
#include "FlowField.h"

// BattleFrame 插件
#include "NeighborGridActor.h"
#include "NeighborGridComponent.h"

#include "BattleFrameInterface.h"



ABattleFrameBattleControl* ABattleFrameBattleControl::Instance = nullptr;

void ABattleFrameBattleControl::BeginPlay()
{
	Super::BeginPlay();

	Instance = this;

	DefineFilters();
}

void ABattleFrameBattleControl::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("BattleControlTick");

	Super::Tick(DeltaTime);

	CurrentWorld = GetWorld();

	if (UNLIKELY(CurrentWorld))
	{
		Mechanism = UMachine::ObtainMechanism(CurrentWorld);

		NeighborGrids.Reset(); // to do : add in multiple grid support

		for (TActorIterator<ANeighborGridActor> It(CurrentWorld); It; ++It)
		{
			NeighborGrids.Add(It->GetComponent());
		}

		if (UNLIKELY(!bIsFilterReady))
		{
			DefineFilters();
		}
	}

	if (UNLIKELY(bGamePaused || !CurrentWorld || !Mechanism || NeighborGrids.IsEmpty())) return;

	float SafeDeltaTime = FMath::Clamp(DeltaTime, 0, 0.0333f);

	//UE_LOG(LogTemp, Warning, TEXT("bIsHitAnim: %d"), bIsHitAnim);

	//------------------- 数据统计 | Statistics ---------------------

	// 数据统计统计 | Statistics
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("Agent Statistics");

		auto Chain = Mechanism->EnchainSolid(AgentStatFilter);
		AgentCount = Chain->IterableNum();
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently([&](FSolidSubjectHandle Subject, FStatistics& Stats)
		{
			const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

			if (Stats.bEnable)
			{
				Stats.TotalTime += DeltaTime;
			}

			// 最大寿命机制
			const bool bHasDeath = Subject.HasTrait<FDeath>();
			const bool bHasDying = Subject.HasTrait<FDying>();

			if (bHasDeath && !bHasDying)
			{
				const auto& Death = Subject.GetTraitRef<FDeath>();

				if (Death.LifeSpan >= 0 && Stats.TotalTime > Death.LifeSpan)
				{
					Subject.SetTraitDeferred(FDying());

					// Death Event OutOfLifeSpan
					if (bHasIsSubjective)
					{
						FDeathData DeathData;
						DeathData.SelfSubject = FSubjectHandle(Subject);
						DeathData.State = EDeathEventState::OutOfLifeSpan;
						OnDeathQueue.Enqueue(DeathData);
					}
				}
			}

		}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	//----------------------- 出生 | Appear -------------------------

	// 出生 | Appear
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentAppearMain");

		auto Chain = Mechanism->EnchainSolid(AgentAppeaFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FDirected& Directed,
				FAppear& Appear,
				FAppearing& Appearing,
				FAnimating& Animating,
				FCurves& Curves)
			{
				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				// Initial execute
				if (Appearing.Time == 0 && !Appearing.bInitialized)
				{
					Appearing.bInitialized = true;

					// Actor
					for (const FActorSpawnConfig& Config : Appear.SpawnActor)
					{
						FActorSpawnConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Directed.Direction.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));

						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Fx
					for (const FFxConfig& Config : Appear.SpawnFx)
					{
						FFxConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Directed.Direction.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));

						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Sound
					for (const FSoundConfig& Config : Appear.PlaySound)
					{
						FSoundConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Directed.Direction.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));

						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Appear Event
					if (bHasIsSubjective)
					{
						FAppearData AppearData;
						AppearData.SelfSubject = FSubjectHandle(Subject);
						OnAppearQueue.Enqueue(AppearData);
					}
				}
				
				// Sub Status
				if (Appearing.Time >= Appear.Delay && !Appearing.bStarted)
				{
					Appearing.bStarted = true;

					// Animation
					if (Appear.bCanPlayAnim)
					{
						Subject.SetFlag(AppearAnimFlag);
					}

					// Dissolve In
					if (Appear.bCanDissolveIn)
					{
						Subject.SetFlag(AppearDissolveFlag);
					}
					else
					{
						Animating.Dissolve = 0;// unhide
					}
				}

				// 出生动画 | Birth Anim
				if (Subject.HasFlag(AppearAnimFlag))
				{
					if (Appearing.AnimTime >= Appear.Duration)
					{
						Subject.SetFlag(AppearAnimFlag,false);
					}

					Appearing.AnimTime += SafeDeltaTime;
				}

				// 出生淡入 | Dissolve In
				if (Subject.HasFlag(AppearDissolveFlag))
				{
					auto Curve = Curves.DissolveIn.GetRichCurve();

					if (!Curve || Curve->GetNumKeys() == 0) return;

					const auto EndTime = Curve->GetLastKey().Time;
					Animating.Dissolve = 1 - Curve->Eval(FMath::Clamp(Appearing.DissolveTime, 0, EndTime));

					if (Appearing.DissolveTime > EndTime)
					{
						Subject.SetFlag(AppearDissolveFlag, false);
					}

					Appearing.DissolveTime += SafeDeltaTime;
				}

				if (Appearing.Time < (Appear.Duration + Appear.Delay))
				{
					Appearing.Time += SafeDeltaTime;
				}
				else
				{
					Subject.RemoveTraitDeferred<FAppearing>();
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	//----------------------- 移动 | Move ---------------------------

	// 休眠 | Sleep
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentSleep");

		auto Chain = Mechanism->EnchainSolid(AgentSleepFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FTracing& Tracing,
				FSleep& Sleep,
				FSleeping& Sleeping)
			{
				if (!Sleep.bEnable)
				{
					Subject.RemoveTraitDeferred<FSleeping>();
					return;
				}

				if (Tracing.TraceResult.IsValid())
				{
					Subject.RemoveTraitDeferred<FSleeping>();
					return;
				}

				// WIP more logic

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 巡逻 | Patrol
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentPatrol");

		auto Chain = Mechanism->EnchainSolid(AgentPatrolFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FScaled& Scaled,
				FCollider& Collider,
				FTrace& Trace,
				FTracing& Tracing,
				FPatrol& Patrol,
				FPatrolling& Patrolling,
				FNavigation& Navigation,
				FNavigating& Navigating,
				FMove& Move,
				FMoving& Moving)
			{
				if (!Patrol.bEnable)
				{
					Subject.RemoveTraitDeferred<FPatrolling>();
					return;
				}

				if (Tracing.TraceResult.IsValid())
				{
					Subject.RemoveTraitDeferred<FPatrolling>();
					return;
				}

				// 检查是否到达目标点
				const bool bHasArrived = FVector::Dist2D(Located.Location, Moving.Goal) < Patrol.AcceptanceRadius;

				if (!bHasArrived)
				{
					// 移动超时逻辑
					if (Patrolling.MoveTimeLeft <= 0.f)
					{
						ResetPatrol(Patrol, Patrolling, Located);
						Moving.Goal = FindNewPatrolGoalLocation(Patrol, Collider, Trace, Tracing, Located, Scaled, 3);
						Moving.bHasGoal = true;
						Navigating.TimeLeft = 0;
					}
					else
					{
						Patrolling.MoveTimeLeft -= SafeDeltaTime;
					}
				}
				else
				{
					// 到达目标点后的等待逻辑
					if (Patrolling.WaitTimeLeft <= 0.f)
					{
						ResetPatrol(Patrol, Patrolling, Located);
						Moving.Goal = FindNewPatrolGoalLocation(Patrol, Collider, Trace, Tracing, Located, Scaled, 3);
						Moving.bHasGoal = true;
						Navigating.TimeLeft = 0;
					}
					else
					{
						Patrolling.WaitTimeLeft -= SafeDeltaTime;
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 推动 | Pushed Back
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SpeedLimitOverride");

		auto Chain = Mechanism->EnchainSolid(SpeedLimitOverrideFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FCollider Collider,
				FLocated Located,
				FSphereObstacle& SphereObstacle)
			{
				if (UNLIKELY(!IsValid(SphereObstacle.NeighborGrid))) return;

				if (!SphereObstacle.bOverrideSpeedLimit) return;

				bool Hit;
				TArray<FTraceResult> Results;
				FTraceDrawDebugConfig DebugConfig;

				FBFFilter Filter;
				Filter.IncludeTraits.Add(TBaseStructure<FAgent>::Get());
				Filter.IncludeTraits.Add(TBaseStructure<FLocated>::Get());
				Filter.IncludeTraits.Add(TBaseStructure<FScaled>::Get());
				Filter.IncludeTraits.Add(TBaseStructure<FCollider>::Get());
				Filter.IncludeTraits.Add(TBaseStructure<FMoving>::Get());
				Filter.IncludeTraits.Add(TBaseStructure<FActivated>::Get());

				SphereObstacle.NeighborGrid->SphereTraceForSubjects
				(
					-1,
					Located.Location,
					Collider.Radius,
					false,
					FVector::ZeroVector,
					0,
					ESortMode::None,
					FVector::ZeroVector,
					FSubjectArray(SphereObstacle.OverridingAgents.Array()),
					Filter,
					DebugConfig,
					Hit,
					Results
				);

				for (const FTraceResult& Result : Results)
				{
					SphereObstacle.OverridingAgents.Add(Result.Subject);
				}

				TSet<FSubjectHandle> Agents = SphereObstacle.OverridingAgents;

				for (const auto& Agent : Agents)
				{
					if (!Agent.IsValid()) continue;

					float AgentRadius = Agent.GetTrait<FGridData>().Radius;
					float SphereObstacleRadius = Collider.Radius;
					float CombinedRadius = AgentRadius + SphereObstacleRadius;
					FVector AgentLocation = Agent.GetTrait<FLocated>().Location;
					float Distance = FVector::Distance(Located.Location, AgentLocation);

					FMoving& AgentMoving = Agent.GetTraitRef<FMoving, EParadigm::Unsafe>();

					if (Distance < CombinedRadius)
					{
						AgentMoving.Lock();
						AgentMoving.bPushedBack = true;
						AgentMoving.PushBackSpeedOverride = SphereObstacle.NewSpeedLimit;
						AgentMoving.Unlock();
					}
					else if (Distance >= CombinedRadius/* * 1.25f*/)
					{
						AgentMoving.Lock();
						AgentMoving.bPushedBack = false;
						AgentMoving.Unlock();
						SphereObstacle.OverridingAgents.Remove(Agent);
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 移动 | Move
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentMove");
		auto Chain = Mechanism->EnchainSolid(AgentMoveFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FDirected& Directed,
				FScaled& Scaled,
				FCollider& Collider,
				FDefence& Defence,
				FSlowing& Slowing,
				FPatrol& Patrol,
				FChase& Chase,
				FMove& Move,
				FFall& Fall,
				FMoving& Moving,
				FNavigation& Navigation,
				FNavigating& Navigating,
				FTrace& Trace,
				FTracing& Tracing,
				FAvoidance& Avoidance,
				FAvoiding& Avoiding,
				FAnimating& Animating,
				FGridData& GridData)
			{
				const FVector SelfLocation = Located.Location;
				const float SelfRadius = Collider.Radius * Scaled.Scale;

				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				// 死亡区域检测			
				if (SelfLocation.Z - SelfRadius < Fall.KillZ)
				{
					Subject.DespawnDeferred();

					// Death Event KillZ
					if (bHasIsSubjective)
					{
						FDeathData DeathData;
						DeathData.SelfSubject = FSubjectHandle(Subject);
						DeathData.State = EDeathEventState::KillZ;
						OnDeathQueue.Enqueue(DeathData);
					}

					return;
				}

				if (!Move.bEnable) return;

				//--------------------------- Flow Field -------------------------------

				if (Navigation.bReloadFlowField)
				{
					Navigating.FlowField = Navigation.FlowFieldToUse.LoadSynchronous();
					Navigation.bReloadFlowField = false;
				}

				if (UNLIKELY(!IsValid(Navigating.FlowField)))
				{
					UE_LOG(LogTemp, Warning, TEXT("Navigation.FlowField is invalid | Navigation.FlowField无效"));
					return;
				}

				bool bInside_BaseFF;
				FCellStruct& Cell_BaseFF = Navigating.FlowField->GetCellAtLocation(SelfLocation, bInside_BaseFF);

				//------------------------- Move State Machine --------------------------

				// 状态
				const bool bHasValidTraceResult = Tracing.TraceResult.IsValid();
				const bool bIsTraceResultHasLocated = bHasValidTraceResult ? Tracing.TraceResult.HasTrait<FLocated>() : false;
				const bool bIsTraceResultHasBindFlowField = bHasValidTraceResult ? Tracing.TraceResult.HasTrait<FBindFlowField>() : false;

				const bool bIsAppearing = Subject.HasTrait<FAppearing>();
				const bool bIsAttacking = Subject.HasTrait<FAttacking>();
				const bool bIsDying = Subject.HasTrait<FDying>();
				const bool bIsSleeping = Subject.HasTrait<FSleeping>();
				const bool bIsPatrolling = Subject.HasTrait<FPatrolling>();
				const bool bIsBeingHit = Subject.HasTrait<FBeingHit>();
				const bool bIsChasing = Chase.bEnable && bHasValidTraceResult;

				float DistanceToGoal = 0;
				float FinalAcceptenceRadius = 0;
				bool bIsInAcceptanceRadius = false;

				// Sleep
				if (bIsSleeping)
				{
					if (Moving.MoveState != EMoveState::Sleep_Sleeping)
					{
						// Can trace again next frame
						Tracing.TimeLeft = 0;

						// Move Event Sleeping
						if (bHasIsSubjective)
						{
							FMoveData MoveData;
							MoveData.SelfSubject = FSubjectHandle(Subject);
							MoveData.State = EMoveEventState::Sleep_Sleeping;
							OnMoveQueue.Enqueue(MoveData);
						}

						Moving.MoveState = EMoveState::Sleep_Sleeping;
					}
				}
				// Patrol
				else if (bIsPatrolling)
				{
					DistanceToGoal = FVector::Dist2D(SelfLocation, Moving.Goal);
					bIsInAcceptanceRadius = DistanceToGoal <= Patrol.AcceptanceRadius;
					FinalAcceptenceRadius = Patrol.AcceptanceRadius;

					EMoveState NewMoveState = bIsInAcceptanceRadius ? EMoveState::Patrol_Waiting : EMoveState::Patrol_Patrolling;
					const bool bIsPreviouslyPatrolling = Moving.MoveState == EMoveState::Patrol_Waiting || Moving.MoveState == EMoveState::Patrol_Patrolling;

					if (Moving.MoveState != NewMoveState)
					{
						// Can trace again next frame
						if (!bIsPreviouslyPatrolling) Tracing.TimeLeft = 0;

						// Move Event Patrol
						if (bHasIsSubjective)
						{
							FMoveData MoveData;
							MoveData.SelfSubject = FSubjectHandle(Subject);
							MoveData.State = bIsInAcceptanceRadius ? EMoveEventState::Patrol_Waiting : EMoveEventState::Patrol_Patrolling;
							OnMoveQueue.Enqueue(MoveData);
						}

						Moving.MoveState = NewMoveState;
					}
				}
				// Chase
				else if (bIsChasing)
				{
					float OtherRadius = Tracing.TraceResult.HasTrait<FGridData>() ? Tracing.TraceResult.GetTraitRef<FGridData, EParadigm::Unsafe>().Radius : 0;
					DistanceToGoal = FMath::Clamp(FVector::Dist2D(SelfLocation, Moving.Goal) - SelfRadius - OtherRadius, 0, FLT_MAX);
					bIsInAcceptanceRadius = DistanceToGoal <= Chase.AcceptanceRadius;
					FinalAcceptenceRadius = Chase.AcceptanceRadius + OtherRadius;

					EMoveState NewMoveState = bIsInAcceptanceRadius ? EMoveState::Chase_Reached : EMoveState::Chase_Chasing;
					const bool bIsPreviouslyChasing = Moving.MoveState == EMoveState::Chase_Reached || Moving.MoveState == EMoveState::Chase_Chasing;

					if (Moving.MoveState != NewMoveState)
					{
						// Can trace again next frame
						if (!bIsPreviouslyChasing) Tracing.TimeLeft = 0;

						// Move Event Chasing
						if (bHasIsSubjective)
						{
							FMoveData MoveData;
							MoveData.SelfSubject = FSubjectHandle(Subject);
							MoveData.State = bIsInAcceptanceRadius ? EMoveEventState::Chase_Reached : EMoveEventState::Chase_Chasing;
							OnMoveQueue.Enqueue(MoveData);
						}

						Moving.MoveState = NewMoveState;
					}
				}
				// Approach
				else
				{
					DistanceToGoal = FVector::Dist2D(SelfLocation, Moving.Goal);
					bIsInAcceptanceRadius = DistanceToGoal <= Move.XY.AcceptanceRadius;
					FinalAcceptenceRadius = Move.XY.AcceptanceRadius;

					EMoveState NewMoveState = bIsInAcceptanceRadius ? EMoveState::Approach_Arrived : EMoveState::Approach_Approaching;
					const bool bIsPreviouslyApproaching = Moving.MoveState == EMoveState::Approach_Arrived || Moving.MoveState == EMoveState::Approach_Approaching;

					if (Moving.MoveState != NewMoveState)
					{
						// Can trace again next frame
						if (!bIsPreviouslyApproaching) Tracing.TimeLeft = 0;

						// Move Event
						if (bHasIsSubjective)
						{
							FMoveData MoveData;
							MoveData.SelfSubject = FSubjectHandle(Subject);
							MoveData.State = bIsInAcceptanceRadius ? EMoveEventState::Approach_Arrived : EMoveEventState::Approach_Approaching;
							OnMoveQueue.Enqueue(MoveData);
						}

						Moving.MoveState = NewMoveState;
					}
				}

				//--------------------- Desired Direction XY (Nav) -----------------------

				FVector DesiredMoveDirection = Directed.Direction;

				// 不需要寻路的情况
				const bool bShouldStopPathfinding = !Move.bEnable/* || !Moving.bHasGoal*/ || bIsAppearing || bIsSleeping || bIsAttacking || bIsDying;

				if (!bShouldStopPathfinding)
				{
					auto PathfindToGoal = [&]()
					{
						if (Navigation.bUseAStar) // find path with a*
						{
							// is valid path ?
							bool bHasPath;
							bool bIsGoalNearEnd;
							bool bIsCurrentNearPath;

							GetSteeringDirection(SelfLocation, Moving.Goal, Navigating.PathPoints, Moving.CurrentVelocity.Size2D(), SelfRadius * 2, SelfRadius * 2, FinalAcceptenceRadius, DesiredMoveDirection, bHasPath, bIsCurrentNearPath, bIsGoalNearEnd);

							// calculate path
							const bool bShouldCalculate = !bHasPath || !bIsGoalNearEnd || !bIsCurrentNearPath && Navigating.TimeLeft <= 0;

							if (bShouldCalculate)
							{
								FindPathAStar(Navigating.FlowField, SelfLocation, Moving.Goal, Navigating.PathPoints);
								Navigating.TimeLeft = Navigation.AStarCoolDown;
							}
							else
							{
								Navigating.TimeLeft = FMath::Clamp(Navigating.TimeLeft - SafeDeltaTime, 0, FLT_MAX);
							}

							// Draw Path
							if (Navigation.bDrawDebugShape && Navigating.PathPoints.Num() >= 2)
							{
								FVector PreviousPoint = Navigating.PathPoints[0];

								for (const auto& Point : Navigating.PathPoints)
								{
									FDebugSphereConfig SphereConfig;
									SphereConfig.Location = Point;
									SphereConfig.Radius = 10;
									SphereConfig.Color = FColor::Red;
									SphereConfig.LineThickness = 0.f;
									DebugSphereQueue.Enqueue(SphereConfig);

									FDebugLineConfig LineConfig;
									LineConfig.StartLocation = PreviousPoint;
									LineConfig.EndLocation = Point;
									LineConfig.Color = FColor::Red;
									LineConfig.LineThickness = 0.f;
									DebugLineQueue.Enqueue(LineConfig);

									PreviousPoint = Point;
								}
							}
						}
						else // approach directly
						{
							DesiredMoveDirection = (Moving.Goal - SelfLocation).GetSafeNormal2D();
						}
					};

					if (bHasValidTraceResult) // 有攻击目标
					{
						if (bIsTraceResultHasLocated) // 可以获取到目标位置
						{
							Moving.Goal = Tracing.TraceResult.GetTrait<FLocated>().Location;
							Moving.bHasGoal = true;
						}

						if (bIsTraceResultHasBindFlowField)
						{
							FBindFlowField BindFlowField = Tracing.TraceResult.GetTrait<FBindFlowField>();

							if (BindFlowField.bReloadFlowField) // 加载目标流场
							{
								BindFlowField.FlowField = BindFlowField.FlowFieldToBind.LoadSynchronous();
								BindFlowField.bReloadFlowField = false;
							}

							if (IsValid(BindFlowField.FlowField)) // 流场加载成功
							{
								bool bInside_TargetFF;
								FCellStruct& Cell_TargetFF = BindFlowField.FlowField->GetCellAtLocation(SelfLocation, bInside_TargetFF);

								if (bInside_TargetFF) // 自身在流场内，使用流场导航
								{
									Moving.Goal = Navigating.FlowField->GetCellAtCoord(Cell_TargetFF.goalCoord, Moving.bHasGoal).worldLoc;
									DesiredMoveDirection = Cell_TargetFF.dir.GetSafeNormal2D();
								}
								else
								{
									PathfindToGoal(); // 自身不在流场内无法使用流场
								}
							}
							else 
							{
								PathfindToGoal(); // 流场加载失败
							}
						}
						else
						{
							PathfindToGoal(); // 目标没有绑定流场
						}
					}
					else // 没有攻击目标
					{
						if (bInside_BaseFF) // 使用基础流场导航
						{
							Moving.Goal = Navigating.FlowField->GetCellAtCoord(Cell_BaseFF.goalCoord, Moving.bHasGoal).worldLoc;
							DesiredMoveDirection = Cell_BaseFF.dir.GetSafeNormal2D();
						}
						else // 不在基础流场内
						{
							PathfindToGoal(); // 目标没有绑定流场
						}
					}
				}

				//------------------------- Desired Speed XY ----------------------------

				// Cases that should stop moving
				const bool bIsAttackingNotColling = bIsAttacking ? Subject.GetTrait<FAttacking>().State != EAttackState::Cooling : false; // Stop when attacking and not cooling
				const bool bIsTimeToBrake = DistanceToGoal < (FMath::Square(Moving.CurrentVelocity.Size2D())) / (2.0f * Move.XY.MoveDeceleration); // 计算最小距离: S_min = V^2 / (2A)
				const bool bShouldStopMoving = Move.XY.bStopActiveMovement || Moving.bLaunching || Moving.bPushedBack || bIsTimeToBrake || bIsInAcceptanceRadius || bIsAppearing || bIsSleeping || bIsAttackingNotColling || bIsDying;

				float DesiredSpeedMultiplier = 0;

				// Calculate MoveSpeedMult
				if (!bShouldStopMoving)
				{
					// adjust speed during patrol
					DesiredSpeedMultiplier = Moving.MoveState == EMoveState::Patrol_Patrolling || Moving.MoveState == EMoveState::Patrol_Waiting ? Patrol.MoveSpeedMult : 1;

					// adjust speed during chase
					DesiredSpeedMultiplier = Moving.MoveState == EMoveState::Chase_Chasing || Moving.MoveState == EMoveState::Chase_Reached ? Chase.MoveSpeedMult : 1;

					// 减速效果累加
					Slowing.CombinedSlowMult = 1;
					for (const auto& Slow : Slowing.Slows) Slowing.CombinedSlowMult *= 1 - Slow.GetTrait<FSlow>().SlowStrength;
					Slowing.CombinedSlowMult = FMath::Lerp(Slowing.CombinedSlowMult, 1, Defence.SlowImmune);// 减速抗性

					DesiredSpeedMultiplier *= Slowing.CombinedSlowMult;

					// 朝向-移动方向夹角 插值
					float DotProduct = FVector::DotProduct(Directed.Direction, Moving.CurrentVelocity.GetSafeNormal2D());
					float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

					const TRange<float> TurnInputRange(Move.XY.MoveSpeedRangeMapByAngle.X, Move.XY.MoveSpeedRangeMapByAngle.Z);
					const TRange<float> TurnOutputRange(Move.XY.MoveSpeedRangeMapByAngle.Y, Move.XY.MoveSpeedRangeMapByAngle.W);

					DesiredSpeedMultiplier *= FMath::GetMappedRangeValueClamped(TurnInputRange, TurnOutputRange, AngleDegrees);

					// 速度-与目标距离二次方插值，离目标越近变化率越大
					const float MinDist = Move.XY.MoveSpeedRangeMapByDist.X;
					const float MaxDist = Move.XY.MoveSpeedRangeMapByDist.Z;
					const float OutputAtMin = Move.XY.MoveSpeedRangeMapByDist.Y;
					const float OutputAtMax = Move.XY.MoveSpeedRangeMapByDist.W;

					// 计算归一化因子并钳制到[0,1]范围
					float NormalizedFactor = (MaxDist - DistanceToGoal) / (MaxDist - MinDist);
					NormalizedFactor = FMath::Clamp(NormalizedFactor, 0.0f, 1.0f);

					// 使用二次方函数计算插值
					float FactorSquared = FMath::Square(NormalizedFactor);
					float MappedValue = OutputAtMax + (OutputAtMin - OutputAtMax) * FactorSquared;

					DesiredSpeedMultiplier *= MappedValue;
				}

				//----------------------- Desired Velocity XY ----------------------------

				float DesiredSpeed = Move.XY.MoveSpeed * DesiredSpeedMultiplier;
				FVector DesiredVelocity = DesiredSpeed * DesiredMoveDirection;
				Moving.DesiredVelocity = DesiredVelocity * FVector(1, 1, 0);

				// Draw Debug
				if (Move.bDrawDebugShape && !bShouldStopMoving)
				{
					FDebugCircleConfig AcceptanceRadiusCircleConfig;
					AcceptanceRadiusCircleConfig.Location = Moving.Goal;
					AcceptanceRadiusCircleConfig.Radius = FinalAcceptenceRadius;
					AcceptanceRadiusCircleConfig.Color = FColor::Purple;
					AcceptanceRadiusCircleConfig.LineThickness = 0.f;
					DebugCircleQueue.Enqueue(AcceptanceRadiusCircleConfig);

					FDebugLineConfig LineToGoalConfig;
					LineToGoalConfig.StartLocation = Located.Location - FVector(0, 0, SelfRadius);
					LineToGoalConfig.EndLocation = Moving.Goal;
					LineToGoalConfig.Color = FColor::Purple;
					LineToGoalConfig.LineThickness = 0.f;
					DebugLineQueue.Enqueue(LineToGoalConfig);

					if (bIsPatrolling)
					{
						FDebugCircleConfig PatrolCircleMinConfig;
						PatrolCircleMinConfig.Location = Patrol.Origin;
						PatrolCircleMinConfig.Radius = Patrol.PatrolRadiusMin;
						PatrolCircleMinConfig.Color = FColor::Purple;
						PatrolCircleMinConfig.LineThickness = 0;
						DebugCircleQueue.Enqueue(PatrolCircleMinConfig);

						FDebugCircleConfig PatrolCircleMaxConfig;
						PatrolCircleMaxConfig.Location = Patrol.Origin;
						PatrolCircleMaxConfig.Radius = Patrol.PatrolRadiusMax;
						PatrolCircleMaxConfig.Color = FColor::Purple;
						PatrolCircleMaxConfig.LineThickness = 0;
						DebugCircleQueue.Enqueue(PatrolCircleMaxConfig);

						FDebugLineConfig LineToPatrolOriginConfig;
						LineToPatrolOriginConfig.StartLocation = Patrol.Origin;
						LineToPatrolOriginConfig.EndLocation = Located.Location - FVector(0,0,SelfRadius);
						LineToPatrolOriginConfig.Color = FColor::Purple;
						LineToPatrolOriginConfig.LineThickness = 0.f;
						DebugLineQueue.Enqueue(LineToPatrolOriginConfig);
					}

					//if (bIsChasing)
					//{
					//	FDebugCircleConfig ChaseMaxDistCircleConfig;
					//	ChaseMaxDistCircleConfig.Location = Located.Location;
					//	ChaseMaxDistCircleConfig.Radius = Chase.MaxDistance;
					//	ChaseMaxDistCircleConfig.Color = FColor::Purple;
					//	ChaseMaxDistCircleConfig.LineThickness = 0.f;
					//	DebugCircleQueue.Enqueue(ChaseMaxDistCircleConfig);
					//}
				}

				//---------------------------- Launched -----------------------------------

				if (Moving.LaunchVelSum != FVector::ZeroVector)// add pending deltaV into current V
				{
					Moving.CurrentVelocity += Moving.LaunchVelSum * (1 - (bIsDying ? Defence.LaunchImmuneDead : Defence.LaunchImmuneAlive));

					FVector XYDir = Moving.CurrentVelocity.GetSafeNormal2D();
					float XYSpeed = Moving.CurrentVelocity.Size2D();
					XYSpeed = FMath::Clamp(XYSpeed, 0, Move.XY.MoveSpeed + Defence.LaunchMaxImpulse);
					FVector XYVelocity = XYSpeed * XYDir;

					float ZSpeed = Moving.CurrentVelocity.Z;
					ZSpeed = FMath::Clamp(ZSpeed, -Defence.LaunchMaxImpulse, Defence.LaunchMaxImpulse);

					Moving.CurrentVelocity = FVector(XYVelocity.X, XYVelocity.Y, ZSpeed);
					Moving.LaunchVelSum = FVector::ZeroVector;
					Moving.bLaunching = true;
				}

				if (Moving.bLaunching)// switch launching state by vV
				{
					if (Moving.CurrentVelocity.Size2D() < 100.f)
					{
						Moving.bLaunching = false;
					}
				}

				//------------------- Final Velocity XY (Avoidance) --------------------------------

				// 计算避障速度
				const auto NeighborGrid = Tracing.NeighborGrid;

				if (LIKELY(Avoidance.bEnable) && LIKELY(IsValid(NeighborGrid)))
				{
					const float AvoidSpeedMultiplier = bIsAttacking ? 0.25 : 1;
					const auto AvoidingRadius = Avoiding.Radius;
					const auto TraceDist = Avoidance.TraceDist;
					const float CombinedRadiusSqr = FMath::Square(AvoidingRadius + TraceDist);
					const int32 MaxNeighbors = Avoidance.MaxNeighbors;
					uint32 SelfHash = GridData.SubjectHash;

					// Avoid Subject Neighbors
					const FVector SubjectRange3D(TraceDist + AvoidingRadius, TraceDist + AvoidingRadius, AvoidingRadius);
					TArray<FIntVector> NeighbourCellCoords = NeighborGrid->GetNeighborCells(SelfLocation, SubjectRange3D);

					// 使用最大堆收集最近的SubjectNeighbors
					auto SubjectCompare = [&](const FGridData& A, const FGridData& B)
						{
							return A.DistSqr > B.DistSqr;
						};

					TArray<FGridData> SubjectNeighbors;
					SubjectNeighbors.Reserve(MaxNeighbors);

					TArray<uint32> SeenHashes;
					SeenHashes.Reserve(MaxNeighbors);

					FFilter SubjectFilter = SubjectFilterBase;

					// 碰撞组
					if (!Avoidance.IgnoreGroups.IsEmpty())
					{
						const int32 ClampedGroups = FMath::Clamp(Avoidance.IgnoreGroups.Num(), 0, 9);

						for (int32 i = 0; i < ClampedGroups; ++i)
						{
							UBattleFrameFunctionLibraryRT::ExcludeAvoGroupTraitByIndex(Avoidance.IgnoreGroups[i], SubjectFilter);
						}
					}

					// this for loop is the most expensive code of all
					for (const auto& Coord : NeighbourCellCoords)
					{
						//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachCell");
						auto& Subjects = NeighborGrid->GetCellAt(NeighborGrid->SubjectCells, Coord).Subjects;

						for (auto& Data : Subjects)
						{
							// we put faster cache friendly checks before slower checks
							// 排除自身
							if (UNLIKELY(Data.SubjectHash == SelfHash)) continue;

							if (UNLIKELY(!Data.SubjectHandle.IsValid())) continue;

							// 距离检查
							const float DistSqr = FVector::DistSquared(SelfLocation, FVector(Data.Location));
							if (DistSqr > CombinedRadiusSqr) continue;

							// 去重
							if (UNLIKELY(SeenHashes.Contains(Data.SubjectHash))) continue;
							SeenHashes.Add(Data.SubjectHash);

							if (UNLIKELY(Data.SubjectHandle.HasTrait<FDying>())) continue;

							// Filter By Traits
							if (UNLIKELY(!Data.SubjectHandle.Matches(SubjectFilter))) continue;

							// we limit the amount of subjects. we keep the nearest MaxNeighbors amount of neighbors
							// 动态维护堆
							if (LIKELY(SubjectNeighbors.Num() < MaxNeighbors))
							{
								Data.DistSqr = DistSqr;
								SubjectNeighbors.HeapPush(Data, SubjectCompare);
							}
							else
							{
								const auto& HeapTop = SubjectNeighbors.HeapTop();

								if (UNLIKELY(DistSqr < HeapTop.DistSqr))
								{
									Data.DistSqr = DistSqr;
									SubjectNeighbors.HeapPopDiscard(SubjectCompare);
									SubjectNeighbors.HeapPush(Data, SubjectCompare);
								}
							}
						}
					}

					//TRACE_CPUPROFILER_EVENT_SCOPE_STR("CalVelAgents");
					Avoidance.MaxSpeed = bIsAttacking ? Move.XY.MoveSpeed * 0.25 : Moving.DesiredVelocity.Size2D();
					Avoidance.DesiredVelocity = RVO::Vector2(Moving.DesiredVelocity.X, Moving.DesiredVelocity.Y);
					Avoiding.CurrentVelocity = RVO::Vector2(Moving.CurrentVelocity.X, Moving.CurrentVelocity.Y);

					// suggest the velocity to avoid collision
					TArray<FGridData> EmptyArray;
					ComputeAvoidingVelocity(Avoidance, Avoiding, SubjectNeighbors, EmptyArray, SafeDeltaTime);

					FVector AvoidingVelocity(Avoidance.AvoidingVelocity.x(), Avoidance.AvoidingVelocity.y(), 0);
					FVector CurrentVelocity = Moving.CurrentVelocity * FVector(1, 1, 0);
					FVector InterpedVelocity = FVector::ZeroVector;

					// apply velocity
					if (LIKELY(!Moving.bFalling && !Moving.bLaunching && !Moving.bPushedBack))
					{
						const bool bIsAccelerating = AvoidingVelocity.SizeSquared2D() > CurrentVelocity.SizeSquared2D();
						InterpedVelocity = FMath::VInterpConstantTo(CurrentVelocity, AvoidingVelocity, DeltaTime, bIsAccelerating ? Move.XY.MoveAcceleration : Move.XY.MoveDeceleration);
					}
					else if (Moving.bFalling)
					{
						InterpedVelocity = FMath::VInterpConstantTo(CurrentVelocity, AvoidingVelocity, DeltaTime, 100);
					}
					else if (Moving.bLaunching || Moving.bPushedBack)
					{
						InterpedVelocity = FMath::VInterpConstantTo(CurrentVelocity, AvoidingVelocity, DeltaTime, Move.XY.MoveDeceleration);
					}

					Moving.CurrentVelocity = FVector(InterpedVelocity.X, InterpedVelocity.Y, Moving.CurrentVelocity.Z);

					// Avoid Obstacle Neighbors
					Avoidance.MaxSpeed = Moving.bPushedBack ? FMath::Max(Moving.CurrentVelocity.Size2D(), Moving.PushBackSpeedOverride) : Moving.CurrentVelocity.Size2D();
					Avoidance.DesiredVelocity = RVO::Vector2(Moving.CurrentVelocity.X, Moving.CurrentVelocity.Y);
					Avoiding.CurrentVelocity = RVO::Vector2(Moving.CurrentVelocity.X, Moving.CurrentVelocity.Y);

					const float ObstacleRange = Avoidance.RVO_TimeHorizon_Obstacle * Avoidance.MaxSpeed + Avoiding.Radius;
					const FVector ObstacleRange3D(ObstacleRange, ObstacleRange, Avoiding.Radius);
					TArray<FIntVector> ObstacleCellCoords = NeighborGrid->GetNeighborCells(SelfLocation, ObstacleRange3D);

					TSet<FGridData> ValidSphereObstacleNeighbors;
					TSet<FGridData> ValidBoxObstacleNeighbors;

					ValidSphereObstacleNeighbors.Reserve(MaxNeighbors);
					ValidBoxObstacleNeighbors.Reserve(MaxNeighbors);

					// lambda to gather obstacles
					auto ProcessSphereObstacles = [&](const FGridData& Obstacle)
						{
							ValidSphereObstacleNeighbors.Add(Obstacle);
						};

					auto ProcessBoxObstacles = [&](const FGridData& Obstacle)
						{
							if (LIKELY(ValidBoxObstacleNeighbors.Contains(Obstacle))) return;
							const FSubjectHandle ObstacleHandle = Obstacle.SubjectHandle;
							if (UNLIKELY(!ObstacleHandle.IsValid()))return;
							const auto ObstacleData = ObstacleHandle.GetTraitPtr<FBoxObstacle, EParadigm::Unsafe>();
							if (UNLIKELY(!ObstacleData)) return;

							const RVO::Vector2& ObstaclePoint = ObstacleData->point_;
							const float ObstaclePointZ = ObstacleData->pointZ_;
							const float ObstacleHalfHeight = ObstacleData->height_ * 0.5f;
							const RVO::Vector2& NextObstaclePoint = ObstacleData->nextPoint_;

							// Z 轴范围检查
							const float ObstacleZMin = ObstaclePointZ - ObstacleHalfHeight;
							const float ObstacleZMax = ObstaclePointZ + ObstacleHalfHeight;
							const float SubjectZMin = SelfLocation.Z - AvoidingRadius;
							const float SubjectZMax = SelfLocation.Z + AvoidingRadius;

							if (SubjectZMax < ObstacleZMin || SubjectZMin > ObstacleZMax) return;

							// 2D 碰撞检测（RVO）
							RVO::Vector2 currentPos(Located.Location.X, Located.Location.Y);

							float leftOfValue = RVO::leftOf(ObstaclePoint, NextObstaclePoint, currentPos);

							if (leftOfValue < 0.0f)
							{
								ValidBoxObstacleNeighbors.Add(Obstacle);
							}
						};

					auto ProcessObstacles = [&](const TArray<FGridData, TInlineAllocator<16>>& Obstacles)
						{
							for (const auto& Obstacle : Obstacles)
							{
								if (Obstacle.SubjectHandle.HasTrait<FSphereObstacle>())
								{
									ProcessSphereObstacles(Obstacle);
								}
								else
								{
									ProcessBoxObstacles(Obstacle);
								}
							}
						};

					for (const FIntVector& Coord : ObstacleCellCoords)
					{
						const auto& ObstacleCell = NeighborGrid->GetCellAt(NeighborGrid->ObstacleCells, Coord);
						ProcessObstacles(ObstacleCell.Subjects);
					}

					for (const FIntVector& Coord : ObstacleCellCoords)
					{
						const auto& StaticObstacleCell = NeighborGrid->GetCellAt(NeighborGrid->StaticObstacleCells, Coord);
						ProcessObstacles(StaticObstacleCell.Subjects);
					}

					TArray<FGridData> SphereObstacleNeighbors = ValidSphereObstacleNeighbors.Array();
					TArray<FGridData> BoxObstacleNeighbors = ValidBoxObstacleNeighbors.Array();

					ComputeAvoidingVelocity(Avoidance, Avoiding, SphereObstacleNeighbors, BoxObstacleNeighbors, SafeDeltaTime);

					Moving.CurrentVelocity = FVector(Avoidance.AvoidingVelocity.x(), Avoidance.AvoidingVelocity.y(), Moving.CurrentVelocity.Z);
				}

				// 记录平均速度
				if (UNLIKELY(Moving.bShouldInit))
				{
					Moving.Initialize();
				}

				if (UNLIKELY(Moving.TimeLeft <= 0.f))
				{
					Moving.UpdateVelocityHistory(Moving.CurrentVelocity);
				}

				Moving.TimeLeft -= SafeDeltaTime;

				//--------------------------- Velocity Z -----------------------------

				// 定义球体追踪lambda函数
				auto PerformSphereTrace = [&](FVector& OutLocation) -> bool
					{
						TRACE_CPUPROFILER_EVENT_SCOPE_STR("SphereTraceForGround");
						const float TraceDistance = FMath::Abs(SelfLocation.Z - Fall.KillZ);
						const FVector TraceStart = SelfLocation + FVector(0, 0, SelfRadius);
						const FVector TraceEnd = FVector(SelfLocation.X, SelfLocation.Y, Fall.KillZ);

						FHitResult HitResult;

						bool bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
							GetWorld(),
							TraceStart,
							TraceEnd,
							SelfRadius,
							Fall.GroundObjectType,
							true,
							TArray<TObjectPtr<AActor>>(),
							EDrawDebugTrace::None,
							HitResult,
							true,
							FLinearColor::Gray,
							FLinearColor::Red,
							1);

						if (bHit)
						{
							OutLocation = HitResult.Location;
							OutLocation.Z -= SelfRadius;
						}

						return bHit;
					};

				// 根据选择的模式进行地面采样
				bool bIsSet = false;
				FVector GroundLocation = FVector::ZeroVector;

				switch (Fall.GroundTraceMode)
				{
				case EGroundTraceMode::FlowFieldAndSphereTrace:
					// 模式1：优先使用流场，失败时回退到球体追踪
					bIsSet = GetInterpedWorldLocation(Navigating.FlowField, SelfLocation, Fall.SphereTraceAngleThreshold, GroundLocation);
					if (!bIsSet) bIsSet = PerformSphereTrace(GroundLocation);
					break;

				case EGroundTraceMode::FlowField:
					// 模式2：仅使用流场采样
					bIsSet = GetInterpedWorldLocation(Navigating.FlowField, SelfLocation, Fall.SphereTraceAngleThreshold, GroundLocation);
					break;

				case EGroundTraceMode::SphereTrace:
					// 模式3：直接使用球体追踪
					bIsSet = PerformSphereTrace(GroundLocation);
					break;
				}

				if (LIKELY(bIsSet))
				{
					// 计算投影高度
					const float GroundHeight = GroundLocation.Z;

					if (UNLIKELY(Fall.bCanFly))
					{
						if (Fall.bEnable)
						{
							Moving.CurrentVelocity.Z += FMath::Clamp(Moving.FlyingHeight + GroundHeight - SelfLocation.Z, -100, 100);//fly at a certain height above ground
							Moving.CurrentVelocity.Z *= 0.9f;
						}
						else
						{
							Moving.CurrentVelocity.Z = 0;
						}
					}
					else
					{
						const float CollisionThreshold = GroundHeight + SelfRadius;

						// 高度状态判断
						if (UNLIKELY(SelfLocation.Z - CollisionThreshold > SelfRadius * 0.1f))// need a bit of tolerance or it will be hard to decide is it is on ground or in the air
						{
							// 应用重力
							if (Fall.bEnable)
							{
								Moving.CurrentVelocity.Z += Fall.Gravity * SafeDeltaTime;
							}
							else
							{
								Moving.CurrentVelocity.Z = 0;
							}

							if (!Moving.bFalling)
							{
								// 进入坠落状态
								Moving.bFalling = true;

								// 使用坠落动画
								if (Fall.bFallAnim)
								{
									Subject.SetFlag(FallAnimFlag);
								}
							}
						}
						else
						{
							// 地面接触处理
							const float GroundContactThreshold = GroundHeight - SelfRadius;

							// 着陆状态切换
							if (Moving.bFalling)
							{
								// 移除坠落状态
								Moving.bFalling = false;

								// 中止坠落动画
								Subject.SetFlag(FallAnimFlag, false);

								if (Fall.bEnable)
								{
									FVector BounceDecay = FVector(Move.XY.MoveBounceVelocityDecay.X, Move.XY.MoveBounceVelocityDecay.X, Move.XY.MoveBounceVelocityDecay.Y);
									Moving.CurrentVelocity = Moving.CurrentVelocity * BounceDecay * FVector(1, 1, (FMath::Abs(Moving.CurrentVelocity.Z) > 100.f) ? -1 : 0);// zero out small number
								}
								else
								{
									Moving.CurrentVelocity.Z = 0;
								}
							}

							// 平滑移动到地面

							if (Fall.bEnable)
							{
								Located.Location.Z = FMath::FInterpTo(SelfLocation.Z, CollisionThreshold, SafeDeltaTime, SelfRadius * 0.5);
							}
						}
					}
				}
				else
				{
					if (UNLIKELY(Fall.bCanFly))
					{
						if (Fall.bEnable)
						{
							Moving.CurrentVelocity.Z *= 0.9f;
						}
						else
						{
							Moving.CurrentVelocity.Z = 0;
						}
					}
					else
					{
						// 应用重力
						if (Fall.bEnable)
						{
							Moving.CurrentVelocity.Z += Fall.Gravity * SafeDeltaTime;
						}
						else
						{
							Moving.CurrentVelocity.Z = 0;
						}

						if (!Moving.bFalling)
						{
							// 进入坠落状态
							Moving.bFalling = true;

							// 使用坠落动画
							if (Fall.bFallAnim)
							{
								Subject.SetFlag(FallAnimFlag);
							}
						}
					}
				}

				//-------------------------- New Location -----------------------------
				
				// 执行最终位移
				Located.PreLocation = Located.Location;
				Located.Location += Moving.CurrentVelocity * SafeDeltaTime;

				//--------------------------- Orientation --------------------------------------

				Moving.TurnSpeedMult = 0;

				if (Move.Yaw.bEnable)
				{
					bool bIsAiming = false;
					bool bIsAttckingStatePrePost = false;

					if (bIsAttacking)
					{
						const auto State = Subject.GetTrait<FAttacking>().State;

						bIsAiming = State == EAttackState::Aim; // 瞄准阶段强制朝向攻击目标
						bIsAttckingStatePrePost = State == EAttackState::PreCast_FirstExec || State == EAttackState::PreCast || State == EAttackState::PostCast; // 播放攻击动画的时间段不转向
					}

					// 不转向的情况
					const bool bShouldStopTurning = !Move.bEnable || Moving.bFalling || Moving.bLaunching || Moving.bPushedBack || bIsAppearing || bIsSleeping || bIsAttckingStatePrePost || bIsDying;

					if (!bShouldStopTurning)
					{
						// 转向减速乘数
						Moving.TurnSpeedMult = Slowing.CombinedSlowMult;

						// 计算希望朝向的方向
						float VelocitySize = 0;

						if (UNLIKELY(bIsAiming)) // 如果是攻击状态瞄准阶段，就朝向攻击目标
						{
							if (bHasValidTraceResult)
							{
								FVector TargetLocation = Tracing.TraceResult.GetTrait<FLocated>().Location;
								Directed.DesiredDirection = (TargetLocation - SelfLocation).GetSafeNormal2D();
							}
						}
						else
						{
							// 计算速度比例和混合因子
							float SpeedRatio = FMath::Clamp(Moving.CurrentVelocity.Size2D() / Move.XY.MoveSpeed, 0.0f, 1.0f);
							float BlendFactor = FMath::Pow(SpeedRatio, 2.0f); // 使用平方使低速时更倾向于平均速度

							// 混合当前速度和平均速度
							FVector LerpedVelocity = FMath::Lerp(Moving.AverageVelocity, Moving.CurrentVelocity, BlendFactor);
							FVector VelocityDirection = LerpedVelocity.GetSafeNormal2D();
							VelocitySize = LerpedVelocity.Size2D();

							switch (Move.Yaw.TurnMode)
							{
								case EOrientMode::ToPath:
									Directed.DesiredDirection = DesiredMoveDirection.SizeSquared2D() == 0 ? Directed.Direction : DesiredMoveDirection;
									break;

								case EOrientMode::ToMovement:
									Directed.DesiredDirection = VelocityDirection;
									break;

								case EOrientMode::ToMovementForwardAndBackward:
									// 朝向-移动方向夹角 插值
									float DotProduct = FVector::DotProduct(Moving.DesiredVelocity.GetSafeNormal2D(), VelocityDirection);
									float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
									float bInvertSign = AngleDegrees > 90.f ? -1.f : 1.f;
									Directed.DesiredDirection = VelocityDirection * bInvertSign;
									break;
							}
						}

						// 执行转向插值
						FRotator CurrentRot = Directed.Direction.GetSafeNormal2D().ToOrientationRotator();
						FRotator TargetRot = Directed.DesiredDirection.GetSafeNormal2D().ToOrientationRotator();

						// 计算当前与目标的Yaw差
						float CurrentYaw = CurrentRot.Yaw;
						float TargetYaw = TargetRot.Yaw;
						float DeltaYaw = FRotator::NormalizeAxis(TargetYaw - CurrentYaw);

						// 小角度容差
						const float ANGLE_TOLERANCE = 0.1f;

						if (FMath::Abs(DeltaYaw) < ANGLE_TOLERANCE)
						{
							// 已经对准目标，停止旋转
							Moving.CurrentAngularVelocity = 0.0f;
							CurrentRot.Yaw = TargetYaw;
						}
						else
						{
							// 旋转方向
							const float Dir = FMath::Sign(DeltaYaw);
							float Acceleration = 0.0f;

							// 速度方向判断
							if (FMath::Sign(Moving.CurrentAngularVelocity) == Dir)
							{
								// 方向正确时的减速判断
								const float CurrentSpeed = FMath::Abs(Moving.CurrentAngularVelocity);
								const float StopDistance = (CurrentSpeed * CurrentSpeed) / (2 * Move.Yaw.TurnAcceleration);

								if (StopDistance >= FMath::Abs(DeltaYaw))
								{
									// 需要减速停止
									Acceleration = -Dir * Move.Yaw.TurnAcceleration;
								}
								else if (CurrentSpeed < Move.Yaw.TurnSpeed)
								{
									// 可以继续加速
									Acceleration = Dir * Move.Yaw.TurnAcceleration;
								}
							}
							else
							{
								// 方向错误时先减速到0
								if (!FMath::IsNearlyZero(Moving.CurrentAngularVelocity, 0.1f))
								{
									Acceleration = -FMath::Sign(Moving.CurrentAngularVelocity) * Move.Yaw.TurnAcceleration;
								}
								else
								{
									// 静止状态直接开始加速
									Acceleration = Dir * Move.Yaw.TurnAcceleration;
								}
							}

							// 计算新角速度
							float NewAngularVelocity = Moving.CurrentAngularVelocity + Acceleration * DeltaTime;
							NewAngularVelocity = FMath::Clamp(NewAngularVelocity, -Move.Yaw.TurnSpeed, Move.Yaw.TurnSpeed);

							// 使用平均速度计算实际转动角度
							const float AvgAngularVelocity = 0.5f * (Moving.CurrentAngularVelocity + NewAngularVelocity);
							float AppliedDeltaYaw = AvgAngularVelocity * DeltaTime;

							// 防止角度过冲
							if (FMath::Abs(AppliedDeltaYaw) > FMath::Abs(DeltaYaw))
							{
								AppliedDeltaYaw = DeltaYaw;
								NewAngularVelocity = 0.0f; // 到达目标后停止
							}

							// 应用旋转
							CurrentRot.Yaw = FRotator::NormalizeAxis(CurrentRot.Yaw + AppliedDeltaYaw);
							Moving.CurrentAngularVelocity = NewAngularVelocity * Moving.TurnSpeedMult;
						}

						// 应用朝向
						if (bIsAiming || VelocitySize > KINDA_SMALL_NUMBER)
						{
							Directed.Direction = CurrentRot.Vector();
						}
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 更新邻居网格 | Update NeighborGrid
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("Update NeighborGrid");

		for (UNeighborGridComponent* Grid : NeighborGrids)
		{
			if (Grid)
			{
				Grid->Update();
			}
		}
	}
	#pragma endregion

	//----------------------- 攻击 | Attack -------------------------

	// 索敌 | Trace
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentTrace");

		// Trace Player 0
		bool bPlayerIsValid = false;
		FVector PlayerLocation;
		FSubjectHandle PlayerHandle;
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(CurrentWorld, 0);

		if (IsValid(PlayerPawn))
		{
			USubjectiveActorComponent* SubjectiveComponent = PlayerPawn->FindComponentByClass<USubjectiveActorComponent>();

			if (IsValid(SubjectiveComponent))
			{
				PlayerHandle = SubjectiveComponent->GetHandle();

				if (PlayerHandle.IsValid())
				{
					if (!PlayerHandle.HasTrait<FDying>() && PlayerHandle.HasTrait<FLocated>() && PlayerHandle.HasTrait<FHealth>() && PlayerHandle.GetTrait<FHealth>().Current > 0)
					{
						PlayerLocation = PlayerPawn->GetActorLocation();
						bPlayerIsValid = true;
					}
				}
			}
		}

		// Trace By Filter
		auto Chain = Mechanism->EnchainSolid(AgentTraceFilter);
		Chain->Retain();
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, 200, ThreadsCount, BatchSize);

		TArray<FValidSubjects> ValidSubjectsArray;
		ValidSubjectsArray.SetNum(ThreadsCount);

		// Gather all agent that need to do tracing
		Chain->OperateConcurrently([&](FSolidSubjectHandle Subject, FLocated& Located, FTrace& Trace, FTracing& Tracing, FMoving& Moving)
			{
				bool bShouldTrace = false;
				const bool bHasAttacking = Subject.HasTrait<FAttacking>();// we don't trace in the middle of an attack
				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				if (Trace.bEnable && !bHasAttacking)
				{
					if (Tracing.TimeLeft <= 0)
					{
						switch (Moving.MoveState)
						{
							// 休眠时索敌
							case EMoveState::Sleep_Sleeping:
								bShouldTrace = Subject.GetTrait<FSleep>().bCanTrace;
								Tracing.TimeLeft = Trace.SectorTrace.Sleep.bEnable ? Trace.SectorTrace.Sleep.CoolDown : Trace.SectorTrace.Common.CoolDown;
								break;

							// 巡逻时索敌
							case EMoveState::Patrol_Patrolling:
								bShouldTrace = Subject.GetTrait<FPatrol>().bCanTrace;
								Tracing.TimeLeft = Trace.SectorTrace.Patrol.bEnable ? Trace.SectorTrace.Patrol.CoolDown : Trace.SectorTrace.Common.CoolDown;
								break;
								
							case EMoveState::Patrol_Waiting:
								bShouldTrace = Subject.GetTrait<FPatrol>().bCanTrace;
								Tracing.TimeLeft = Trace.SectorTrace.Patrol.bEnable ? Trace.SectorTrace.Patrol.CoolDown : Trace.SectorTrace.Common.CoolDown;
								break;

							// 追逐时索敌
							case EMoveState::Chase_Chasing:
								bShouldTrace = Subject.GetTrait<FChase>().bCanTrace;
								Tracing.TimeLeft = Trace.SectorTrace.Chase.bEnable ? Trace.SectorTrace.Chase.CoolDown : Trace.SectorTrace.Common.CoolDown;
								break;

							case EMoveState::Chase_Reached:
								bShouldTrace = Subject.GetTrait<FChase>().bCanTrace;
								Tracing.TimeLeft = Trace.SectorTrace.Chase.bEnable ? Trace.SectorTrace.Chase.CoolDown : Trace.SectorTrace.Common.CoolDown;
								break;

							// 一般移动
							case EMoveState::Approach_Approaching: 
								bShouldTrace = true;
								Tracing.TimeLeft = Trace.SectorTrace.Common.CoolDown;
								break;

							case EMoveState::Approach_Arrived:
								bShouldTrace = true;
								Tracing.TimeLeft = Trace.SectorTrace.Common.CoolDown;
								break;
						}
					}
					else
					{
						Tracing.TimeLeft = FMath::Clamp(Tracing.TimeLeft - SafeDeltaTime, 0, FLT_MAX);
					}

					if (bShouldTrace)// we add iterables into separate arrays and then append them.
					{
						uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
						uint32 Index = ThreadId % ThreadsCount;// this may not evenly distribute, but well enough

						if (LIKELY(ValidSubjectsArray.IsValidIndex(Index)))
						{
							ValidSubjectsArray[Index].Lock();// we lock child arrays individually
							ValidSubjectsArray[Index].Subjects.Add(Subject);
							ValidSubjectsArray[Index].Unlock();
						}

						// Trace Event Begin
						if (bHasIsSubjective)
						{
							FTraceData TraceData;
							TraceData.SelfSubject = FSubjectHandle(Subject);
							TraceData.State = ETraceEventState::Begin;
							OnTraceQueue.Enqueue(TraceData);
						}
					}
				}

				// Draw debug line and sphere at trace result
				const bool bHasValidTraceResult = Tracing.TraceResult.IsValid() && Tracing.TraceResult.HasTrait<FLocated>();

				if (Trace.bEnable && Trace.bDrawDebugShape && bHasValidTraceResult)
				{
					FVector OtherLocation = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;
					float OtherScale = Tracing.TraceResult.HasTrait<FScaled>() ? Tracing.TraceResult.GetTraitRef<FScaled, EParadigm::Unsafe>().Scale : 1;
					float OtherRadius = Tracing.TraceResult.HasTrait<FCollider>() ? Tracing.TraceResult.GetTraitRef<FCollider, EParadigm::Unsafe>().Radius : 0;
					OtherRadius *= OtherScale;

					FDebugLineConfig LineConfig;
					LineConfig.StartLocation = Located.Location;
					LineConfig.EndLocation = OtherLocation;
					LineConfig.Color = FColor::Orange;
					LineConfig.LineThickness = 0.f;
					DebugLineQueue.Enqueue(LineConfig);

					FDebugSphereConfig SphereConfig;
					SphereConfig.Location = OtherLocation;
					SphereConfig.Radius = OtherRadius;
					SphereConfig.Color = FColor::Orange;
					SphereConfig.LineThickness = 0.f;
					DebugSphereQueue.Enqueue(SphereConfig);
				}

			}, ThreadsCount, BatchSize);

		TArray<FSolidSubjectHandle> ValidSubjects;

		for (auto& CurrentArray : ValidSubjectsArray)
		{
			ValidSubjects.Append(CurrentArray.Subjects);
		}

		// Do Trace
		ParallelFor(ValidSubjects.Num(), [&](int32 Index)
		{
			FSolidSubjectHandle Subject = ValidSubjects[Index];

			FLocated& Located = Subject.GetTraitRef<FLocated>();
			FDirected& Directed = Subject.GetTraitRef<FDirected>();
			FScaled& Scaled = Subject.GetTraitRef<FScaled>();
			FCollider& Collider = Subject.GetTraitRef<FCollider>();

			FTrace& Trace = Subject.GetTraitRef<FTrace>();
			FTracing& Tracing = Subject.GetTraitRef<FTracing>();
			FSleep& Sleep = Subject.GetTraitRef<FSleep>();
			FPatrol& Patrol = Subject.GetTraitRef<FPatrol>();
			FChase& Chase = Subject.GetTraitRef<FChase>();
			FMoving& Moving = Subject.GetTraitRef<FMoving>();
			FNavigating& Navigating = Subject.GetTraitRef<FNavigating>();

			const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();
			bool bHasValidTraceResult = false;
			bool bFinalCheckVisibility = false;
			bool bFinalDrawDebugShape = Trace.bDrawDebugShape;
			bool bIsParamsSet = false;
			bool bCanTrace = false;

			float FinalRange = Collider.Radius * Scaled.Scale;
			float FinalAngle = 0;
			float FinalHeight = 0;

			EMoveState MoveState = Moving.MoveState;
			FSectorTraceParamsSpecific Params;
			FSectorTraceParams Params_Common;

			// 确定用哪一套索敌参数
			switch (MoveState)
			{
				// 休眠时索敌
				case EMoveState::Sleep_Sleeping: 

					bCanTrace = Sleep.bCanTrace;
					Params = Trace.SectorTrace.Sleep;

					if (Params.bEnable && Sleep.bCanTrace)
					{
						FinalRange += Params.TraceRadius;
						FinalAngle = Params.TraceAngle;
						FinalHeight = Params.TraceHeight;
						bFinalCheckVisibility = Params.bCheckObstacle;
						bIsParamsSet = true;
					}

					break;

				// 巡逻时索敌
				case EMoveState::Patrol_Patrolling: 

					bCanTrace = Patrol.bCanTrace;
					Params = Trace.SectorTrace.Patrol;

					if (Params.bEnable && Patrol.bCanTrace)
					{
						FinalRange += Params.TraceRadius;
						FinalAngle = Params.TraceAngle;
						FinalHeight = Params.TraceHeight;
						bFinalCheckVisibility = Params.bCheckObstacle;
						bIsParamsSet = true;
					}

					break;

				case EMoveState::Patrol_Waiting: 

					bCanTrace = Patrol.bCanTrace;
					Params = Trace.SectorTrace.Patrol;

					if (Params.bEnable && Patrol.bCanTrace)
					{
						FinalRange += Params.TraceRadius;
						FinalAngle = Params.TraceAngle;
						FinalHeight = Params.TraceHeight;
						bFinalCheckVisibility = Params.bCheckObstacle;
						bIsParamsSet = true;
					}

					break;

				// 追逐时索敌
				case EMoveState::Chase_Chasing: 

					bCanTrace = Chase.bCanTrace;
					Params = Trace.SectorTrace.Chase;

					if (Params.bEnable && Chase.bCanTrace)
					{
						FinalRange += Params.TraceRadius;
						FinalAngle = Params.TraceAngle;
						FinalHeight = Params.TraceHeight;
						bFinalCheckVisibility = Params.bCheckObstacle;
						bIsParamsSet = true;
					}

					break;

				case EMoveState::Chase_Reached:

					bCanTrace = Chase.bCanTrace;
					Params = Trace.SectorTrace.Chase;

					if (Params.bEnable && Chase.bCanTrace)
					{
						FinalRange += Params.TraceRadius;
						FinalAngle = Params.TraceAngle;
						FinalHeight = Params.TraceHeight;
						bFinalCheckVisibility = Params.bCheckObstacle;
						bIsParamsSet = true;
					}

					break;

				// 一般情况
				case EMoveState::Approach_Approaching: 

					bCanTrace = Trace.bEnable;
					Params_Common = Trace.SectorTrace.Common;

					FinalRange += Params_Common.TraceRadius;
					FinalAngle = Params_Common.TraceAngle;
					FinalHeight = Params_Common.TraceHeight;
					bFinalCheckVisibility = Params_Common.bCheckObstacle;
					bIsParamsSet = true;

					break;

				case EMoveState::Approach_Arrived:

					bCanTrace = Trace.bEnable;
					Params_Common = Trace.SectorTrace.Common;

					FinalRange += Params_Common.TraceRadius;
					FinalAngle = Params_Common.TraceAngle;
					FinalHeight = Params_Common.TraceHeight;
					bFinalCheckVisibility = Params_Common.bCheckObstacle;
					bIsParamsSet = true;

					break;
			}

			// 保底参数
			if (bCanTrace && !bIsParamsSet)
			{
				Params_Common = Trace.SectorTrace.Common;

				FinalRange += Params_Common.TraceRadius;
				FinalAngle = Params_Common.TraceAngle;
				FinalHeight = Params_Common.TraceHeight;
				bFinalCheckVisibility = Params_Common.bCheckObstacle;
			}

			if (bCanTrace)
			{
				// Draw Debug Config
				FTraceDrawDebugConfig DebugConfig;
				DebugConfig.bDrawDebugShape = bFinalDrawDebugShape;
				DebugConfig.Color = FColor::Orange;
				DebugConfig.Duration = Tracing.TimeLeft;
				DebugConfig.LineThickness = 5;
				DebugConfig.HitPointSize = 3;

				Tracing.TraceResult = FSubjectHandle();

				// Do trace
				switch (Trace.Mode)
				{
					case ETraceMode::TargetIsPlayer_0:
					{
						if (bPlayerIsValid)
						{
							TArray<FHitResult> VisibilityResults;

							// 高度检查
							float HeightDifference = PlayerLocation.Z - Located.Location.Z;

							if (HeightDifference <= FinalHeight)
							{
								// 计算目标半径和实际距离平方
								float PlayerRadius = PlayerHandle.HasTrait<FCollider>() ? PlayerHandle.GetTrait<FCollider>().Radius : 0;
								float CombinedRadiusSquared = FMath::Square(FinalRange);
								float DistanceSquared = FVector::DistSquared(Located.Location, PlayerLocation);

								// 距离检查 - 使用距离平方
								if (DistanceSquared <= CombinedRadiusSquared)
								{
									// 角度检查
									const FVector ToPlayerDir = (PlayerLocation - Located.Location).GetSafeNormal();
									const float DotValue = FVector::DotProduct(Directed.Direction, ToPlayerDir);
									const float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(DotValue));

									if (AngleDiff <= FinalAngle * 0.5f)
									{
										if (bFinalCheckVisibility && IsValid(Tracing.NeighborGrid))
										{
											const FVector SubjectSurfacePoint = PlayerLocation - (ToPlayerDir * PlayerRadius);
												
											FHitResult VisibilityResult;

											bool Hit = UKismetSystemLibrary::SphereTraceSingleForObjects
											(
												GetWorld(),
												Located.Location,
												SubjectSurfacePoint,
												0,
												Trace.Filter.ObstacleObjectType,
												true,
												TArray<TObjectPtr<AActor>>(),
												EDrawDebugTrace::None,
												VisibilityResult,
												true,
												FLinearColor::Gray,
												FLinearColor::Red,
												1
											);

											if (Trace.bDrawDebugShape)
											{
												VisibilityResults.Add(VisibilityResult);
											}

											if (!Hit)
											{
												Tracing.TraceResult = PlayerHandle;
											}
										}
										else
										{
											Tracing.TraceResult = PlayerHandle;
										}
									}
								}
							}

							// draw visibility check results
							if (Trace.bDrawDebugShape)
							{
								for (const auto& VisibilityResult : VisibilityResults)
								{
									// 计算起点到终点的向量
									FVector Direction = VisibilityResult.TraceEnd - VisibilityResult.TraceStart;
									float TotalDistance = Direction.Size();

									// 处理零距离情况（使用默认旋转）
									FRotator ShapeRot = FRotator::ZeroRotator;

									if (TotalDistance > 0)
									{
										Direction /= TotalDistance;
										ShapeRot = FRotationMatrix::MakeFromZ(Direction).Rotator();
									}

									// 计算圆柱部分高度（总高度减去两端的半球）
									float CylinderHeight = 0;

									// 计算胶囊体中心位置（两点中点）
									FVector ShapeLoc = (VisibilityResult.TraceStart + VisibilityResult.TraceEnd) * 0.5f;

									// 配置调试胶囊体参数
									FDebugCapsuleConfig CapsuleConfig;
									CapsuleConfig.Color = DebugConfig.Color;
									CapsuleConfig.Location = ShapeLoc;
									CapsuleConfig.Rotation = ShapeRot;
									CapsuleConfig.Radius = 0;
									CapsuleConfig.Height = CylinderHeight;
									CapsuleConfig.LineThickness = DebugConfig.LineThickness;
									CapsuleConfig.Duration = DebugConfig.Duration;

									// 加入调试队列
									ABattleFrameBattleControl::GetInstance()->DebugCapsuleQueue.Enqueue(CapsuleConfig);

									// 绘制碰撞点
									FDebugPointConfig PointConfig;
									PointConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
									PointConfig.Duration = DebugConfig.Duration;
									PointConfig.Location = VisibilityResult.bBlockingHit ? VisibilityResult.ImpactPoint : VisibilityResult.TraceEnd;
									PointConfig.Size = DebugConfig.HitPointSize;

									ABattleFrameBattleControl::GetInstance()->DebugPointQueue.Enqueue(PointConfig);
								}
							}
						}

						break;
					}

					case ETraceMode::SectorTraceByTraits:
					{
						if (LIKELY(IsValid(Tracing.NeighborGrid)))
						{
							bool Hit;
							TArray<FTraceResult> Results;

							const FVector TraceDirection = Directed.Direction.GetSafeNormal2D();

							// ignore self
							FSubjectArray IgnoreList;
							IgnoreList.Subjects.Add(FSubjectHandle(Subject));

							Tracing.NeighborGrid->SectorTraceForSubjects
							(
								1,
								Located.Location,   // 检测原点
								FinalRange,         // 检测半径
								FinalHeight,        // 检测高度
								TraceDirection,     // 扇形方向
								FinalAngle,         // 扇形角度
								bFinalCheckVisibility,
								Located.Location,
								0,
								ESortMode::NearToFar,
								Located.Location,
								IgnoreList,
								Trace.Filter,       // 过滤条件
								DebugConfig,
								Hit,
								Results             // 输出结果
							);

							// 直接使用结果（扇形检测已包含角度验证）
							if (Hit && Results[0].Subject.IsValid())
							{
								Tracing.TraceResult = Results[0].Subject;
							}
						}
						break;
					}
				}

				bHasValidTraceResult = Tracing.TraceResult.IsValid();

				// Draw Trace Sector
				if (bFinalDrawDebugShape)
				{
					// Trace Shape
					FDebugSectorConfig SectorConfig1;
					SectorConfig1.Location = Located.Location;
					SectorConfig1.Radius = FinalRange;
					SectorConfig1.Height = FinalHeight;
					SectorConfig1.Direction = Directed.Direction.GetSafeNormal2D();
					SectorConfig1.Angle = FinalAngle;
					SectorConfig1.Duration = DebugConfig.Duration;
					SectorConfig1.Color = DebugConfig.Color;
					SectorConfig1.LineThickness = 0;
					SectorConfig1.DepthPriority = 0;

					DebugSectorQueue.Enqueue(SectorConfig1);
				}

				// Trace Event, Succeed or Fail
				if (bHasIsSubjective)
				{
					FTraceData TraceData;
					TraceData.SelfSubject = FSubjectHandle(Subject);
					TraceData.State = bHasValidTraceResult ? ETraceEventState::End_Reason_Succeed : ETraceEventState::End_Reason_Fail;
					TraceData.TraceResult = bHasValidTraceResult ? Tracing.TraceResult : FSubjectHandle();
					OnTraceQueue.Enqueue(TraceData);
				}
			}

			// Go back to patrol state when no target
			const bool bShouldPatrol = !bHasValidTraceResult && !Subject.HasTrait<FPatrolling>() && Patrol.OnLostTarget == EPatrolRecoverMode::Patrol;

			if (bShouldPatrol)
			{
				FPatrolling NewPatrolling;
				ResetPatrol(Patrol, NewPatrolling, Located);
				Subject.SetTraitDeferred(NewPatrolling);
			}
		});

		Chain->Release();

		Mechanism->ApplyDeferreds();
	}
	#pragma endregion

	// 攻击触发 | Trigger Attack
	#pragma region 
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentAttackTrigger");

		auto Chain = Mechanism->EnchainSolid(AgentAttackFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FDirected Directed,
				FScaled& Scaled,
				FCollider& Collider,
				FAttack& Attack,
				FTrace& Trace,
				FTracing& Tracing)
			{
				if (!Attack.bEnable) return;

				if (Subject.HasFlag(HitAnimFlag)) return;
				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				// Debug Draw Attack Range
				if (Attack.bDrawDebugShape)
				{
					FDebugCircleConfig CircleConfig;
					CircleConfig.Location = Located.Location;
					CircleConfig.Radius = Attack.Range + Collider.Radius * Scaled.Scale;
					CircleConfig.Color = FColor::Red;

					DebugCircleQueue.Enqueue(CircleConfig);
				}

				const bool bHasAttacking = Subject.HasTrait<FAttacking>();

				if (!bHasAttacking) // 能否发起新一轮攻击
				{
					const bool bIsTargetValid = Tracing.TraceResult.IsValid() && !Tracing.TraceResult.HasTrait<FDying>() && Tracing.TraceResult.HasTrait<FLocated>() && Tracing.TraceResult.HasTrait<FHealth>() && Tracing.TraceResult.GetTrait<FHealth>().Current > 0;

					if (bIsTargetValid)
					{
						const float SelfRadius = Collider.Radius * Scaled.Scale;
						const float OtherRadius = Tracing.TraceResult.HasTrait<FGridData>() ? Tracing.TraceResult.GetTrait<FGridData>().Radius : 0;

						FVector TargetLocation = Tracing.TraceResult.GetTrait<FLocated>().Location;
						float DistSquared = FVector::DistSquared(Located.Location, TargetLocation);
						float CombinedRadius = Attack.Range + SelfRadius + OtherRadius;
						float CombinedRadiusSquared = FMath::Square(CombinedRadius);

						// 触发攻击
						if (DistSquared <= CombinedRadiusSquared)
						{
							Subject.SetTraitDeferred(FAttacking());

							// Attack Aim(Begin) Event
							if (bHasIsSubjective)
							{
								FAttackData AttackData;
								AttackData.SelfSubject = FSubjectHandle(Subject);
								AttackData.State = EAttackEventState::Aiming;
								AttackData.AttackTarget = Tracing.TraceResult;
								OnAttackQueue.Enqueue(AttackData);
							}
						}
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 攻击过程 | Do Attack
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentAttacking");

		auto Chain = Mechanism->EnchainSolid(AgentAttackingFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FDirected& Directed,
				FScaled& Scaled,
				FCollider& Collider,
				FRendering& Rendering,
				FAnimating& Animating,
				FAttack& Attack,
				FAttacking& Attacking,
				FMoving& Moving,
				FTrace& Trace,
				FTracing& Tracing,
				FDebuff& Debuff,
				FDamage& Damage,
				FDefence& Defence,
				FSlowing& Slowing)
			{
				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				// 瞄准
				if (Attacking.State == EAttackState::Aim)
				{
					// 检查被瞄准对象是否有效
					bool bIsTargetValid = Tracing.TraceResult.IsValid() && !Tracing.TraceResult.HasTrait<FDying>() && Tracing.TraceResult.HasTrait<FLocated>() && Tracing.TraceResult.HasTrait<FHealth>() && Tracing.TraceResult.GetTrait<FHealth>().Current > 0;

					// 瞄准目标无效，提前中止
					if (!bIsTargetValid)
					{
						Subject.RemoveTraitDeferred<FAttacking>(); // 不再攻击
						Tracing.TimeLeft = 0; // 可立即重新索敌

						// Attack End Event
						if (bHasIsSubjective)
						{
							FAttackData AttackData;
							AttackData.SelfSubject = FSubjectHandle(Subject);
							AttackData.State = EAttackEventState::End_Reason_InvalidTarget;
							AttackData.AttackTarget = Tracing.TraceResult;
							OnAttackQueue.Enqueue(AttackData);
						}

						return;
					}

					// 检查距离和夹角
					bool bIsDistValid = false;
					bool bIsAngleValid = false;

					if (bIsTargetValid)
					{
						// 检查距离
						const float SelfRadius = Collider.Radius * Scaled.Scale;
						const float OtherRadius = Tracing.TraceResult.HasTrait<FGridData>() ? Tracing.TraceResult.GetTrait<FGridData>().Radius : 0;
						FVector TargetLocation = Tracing.TraceResult.GetTrait<FLocated>().Location;

						float CombinedRadiusSqr = FMath::Square(Attack.Range + SelfRadius + OtherRadius);
						float DistSqr = FVector::DistSquared(Located.Location, TargetLocation);

						if (DistSqr <= CombinedRadiusSqr)
						{
							bIsDistValid = true;
						}

						// 检查夹角
						FVector ToTargetDir = (TargetLocation - Located.Location).GetSafeNormal2D();
						float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Directed.Direction, ToTargetDir)));

						if (Angle <= Attack.AngleToleranceATK)
						{
							bIsAngleValid = true;
						}
					}

					// 瞄准目标太远，提前中止
					if (!bIsDistValid)
					{
						Subject.RemoveTraitDeferred<FAttacking>(); // 不再攻击

						// Attack End Event
						if (bHasIsSubjective)
						{
							FAttackData AttackData;
							AttackData.SelfSubject = FSubjectHandle(Subject);
							AttackData.State = EAttackEventState::End_Reason_NotInATKRange;
							AttackData.AttackTarget = Tracing.TraceResult;
							OnAttackQueue.Enqueue(AttackData);
						}

						return;
					}

					// 瞄准完成，开始攻击
					if (bIsAngleValid && Attacking.AimTime == Attack.MinAimTime)
					{
						Attacking.State = EAttackState::PreCast_FirstExec;
					}
				}

				// 开始攻击
				if (Attacking.State == EAttackState::PreCast_FirstExec)
				{
					Attacking.State = EAttackState::PreCast; // Do Once

					// Start Animation
					if (Attack.bCanPlayAnim)
					{
						Subject.SetFlag(AttackAnimFlag);
					}

					FVector SpawnLocation = Located.Location;
					FQuat SpawnRotation = Directed.Direction.ToOrientationQuat();

					// Projectile
					for (const auto& Config : Attack.SpawnProjectile)
					{
						FProjectileConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.TargetSubject = Tracing.TraceResult;
						NewConfig.FromPoint = Located.Location + Directed.Direction * Collider.Radius * Scaled.Scale;
						NewConfig.ToPoint = Tracing.TraceResult.GetTrait<FLocated>().Location;
						NewConfig.TargetVelocity = Tracing.TraceResult.HasTrait<FMoving>() ? Tracing.TraceResult.GetTrait<FMoving>().CurrentVelocity : FVector::ZeroVector;

						switch (Config.SpawnOrigin)
						{
						case ESpawnOrigin::AtSelf:

							NewConfig.AttachToSubject = FSubjectHandle(Subject);
							NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
							NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(SpawnRotation, SpawnLocation));

							Mechanism->SpawnSubjectDeferred(NewConfig);
							break;

						case ESpawnOrigin::AtTarget:

							if (Tracing.TraceResult.IsValid())
							{
								NewConfig.AttachToSubject = Tracing.TraceResult;
								SpawnLocation = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;
								FQuat TargetRotation = Tracing.TraceResult.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction.ToOrientationQuat();
								NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
								NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(TargetRotation, SpawnLocation));

								Mechanism->SpawnSubjectDeferred(NewConfig);
							}
							break;
						}
					}

					// Actor
					for (const auto& Config : Attack.SpawnActor)
					{
						FActorSpawnConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);

						switch (Config.SpawnOrigin)
						{
							case ESpawnOrigin::AtSelf:
							{
								NewConfig.AttachToSubject = FSubjectHandle(Subject);
								NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
								NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(SpawnRotation, SpawnLocation));

								Mechanism->SpawnSubjectDeferred(NewConfig);
								break;
							}

							case ESpawnOrigin::AtTarget:
							{
								if (Tracing.TraceResult.IsValid())
								{
									NewConfig.AttachToSubject = Tracing.TraceResult;
									SpawnLocation = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;
									FQuat TargetRotation = Tracing.TraceResult.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction.ToOrientationQuat();
									NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
									NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(TargetRotation, SpawnLocation));

									Mechanism->SpawnSubjectDeferred(NewConfig);
								}
								break;
							}
						}
					}

					// Fx
					for (const auto& Config : Attack.SpawnFx)
					{
						FFxConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);

						switch (Config.SpawnOrigin)
						{
						case ESpawnOrigin::AtSelf:

							NewConfig.AttachToSubject = FSubjectHandle(Subject);
							NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
							NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(SpawnRotation, SpawnLocation));

							Mechanism->SpawnSubjectDeferred(NewConfig);
							break;

						case ESpawnOrigin::AtTarget:

							if (Tracing.TraceResult.IsValid())
							{
								NewConfig.AttachToSubject = Tracing.TraceResult;
								SpawnLocation = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;
								FQuat TargetRotation = Tracing.TraceResult.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction.ToOrientationQuat();
								NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
								NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(TargetRotation, SpawnLocation));

								Mechanism->SpawnSubjectDeferred(NewConfig);
							}
							break;
						}
					}

					// Sound
					for (const auto& Config : Attack.PlaySound)
					{
						FSoundConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);

						switch (Config.SpawnOrigin)
						{
						case EPlaySoundOrigin_Attack::PlaySound3D_AtSelf:

							NewConfig.AttachToSubject = FSubjectHandle(Subject);
							NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
							NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(SpawnRotation, SpawnLocation));

							Mechanism->SpawnSubjectDeferred(NewConfig);
							break;

						case EPlaySoundOrigin_Attack::PlaySound3D_AtTarget:

							if (Tracing.TraceResult.IsValid())
							{
								NewConfig.AttachToSubject = Tracing.TraceResult;
								SpawnLocation = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;
								FQuat TargetRotation = Tracing.TraceResult.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction.ToOrientationQuat();
								NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(SpawnRotation, SpawnLocation, NewConfig.Transform);
								NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(TargetRotation, SpawnLocation));

								Mechanism->SpawnSubjectDeferred(NewConfig);
							}
							break;
						}
					}

					// Attack Begin Event
					if (bHasIsSubjective)
					{
						FAttackData AttackData;
						AttackData.SelfSubject = FSubjectHandle(Subject);
						AttackData.State = EAttackEventState::Begin;
						AttackData.AttackTarget = Tracing.TraceResult;
						OnAttackQueue.Enqueue(AttackData);
					}
				}

				// 判定击中
				if (Attacking.State == EAttackState::PreCast && Attacking.ATKTime >= Attack.TimeOfHit)
				{
					Attacking.State = EAttackState::PostCast; // Do Once

					TArray<FDmgResult> DmgResults;

					bool bIsTargetValid = Tracing.TraceResult.IsValid() && !Tracing.TraceResult.HasTrait<FDying>() && Tracing.TraceResult.HasTrait<FLocated>() && Tracing.TraceResult.HasTrait<FHealth>() && Tracing.TraceResult.GetTrait<FHealth>().Current > 0;
					bool bCanApplyDmg = Attack.TimeOfHitAction == EAttackMode::ApplyDMG || Attack.TimeOfHitAction == EAttackMode::SuicideATK;

					if (bIsTargetValid && bCanApplyDmg)
					{
						if (Attack.TimeOfHitAction == EAttackMode::ApplyDMG || Attack.TimeOfHitAction == EAttackMode::SuicideATK)
						{
							// 获取双方位置信息
							FVector AttackerPos = Located.Location;
							FVector TargetPos = Tracing.TraceResult.GetTraitRef<FLocated, EParadigm::Unsafe>().Location;

							// 计算距离
							const float SelfRadius = Collider.Radius * Scaled.Scale;
							float OtherRadius = Tracing.TraceResult.HasTrait<FGridData>() ? Tracing.TraceResult.GetTraitRef<FGridData, EParadigm::Unsafe>().Radius : 0;
							float Distance = FMath::Clamp(FVector::Distance(AttackerPos, TargetPos) - SelfRadius - OtherRadius, 0, FLT_MAX);

							// 计算夹角
							FVector AttackerForward = Subject.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction.GetSafeNormal2D();
							FVector ToTargetDir = (TargetPos - AttackerPos).GetSafeNormal2D();
							float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(AttackerForward, ToTargetDir)));

							// Deal Dmg
							if (Distance <= Attack.RangeToleranceHit && Angle <= Attack.AngleToleranceHit)
							{
								if (Damage.DmgRadius == 0)
								{
									// 范围伤害
									ApplyPointDamageAndDebuffDeferred
									(
										FSubjectArray(TArray<FSubjectHandle>{Tracing.TraceResult}),
										FSubjectArray(),
										FSubjectHandle{ Subject },
										FSubjectHandle{ Subject },
										Located.Location,
										FDamage_Point(Damage),
										FDebuff_Point(Debuff),
										DmgResults
									);
								}
								else
								{
									ApplyRadialDamageAndDebuffDeferred
									(
										Tracing.NeighborGrid,
										-1,
										TargetPos,
										FSubjectArray(),
										FSubjectHandle{ Subject },
										FSubjectHandle{ Subject },
										Located.Location,
										FDamage_Radial(Damage),
										FDebuff_Radial(Debuff),
										DmgResults
									);
								}
							}
						}
					}

					// Attack Hit Event
					if (bHasIsSubjective)
					{
						FAttackData AttackData;
						AttackData.SelfSubject = FSubjectHandle(Subject);
						AttackData.State = EAttackEventState::Hit;
						AttackData.AttackTarget = Tracing.TraceResult;
						AttackData.DmgResults.Append(DmgResults);
						OnAttackQueue.Enqueue(AttackData);
					}

					// Suicide
					if (Attack.TimeOfHitAction == EAttackMode::SuicideATK || Attack.TimeOfHitAction == EAttackMode::Despawn)
					{
						Subject.DespawnDeferred();

						if (bHasIsSubjective)
						{
							// Death Event SuicideAttack
							FDeathData DeathData;
							DeathData.SelfSubject = FSubjectHandle(Subject);
							DeathData.State = EDeathEventState::SuicideAttack;
							OnDeathQueue.Enqueue(DeathData);

							// Attack Event Complete
							FAttackData AttackData;
							AttackData.SelfSubject = FSubjectHandle(Subject);
							AttackData.State = EAttackEventState::End_Reason_Complete;
							AttackData.AttackTarget = Tracing.TraceResult;
							OnAttackQueue.Enqueue(AttackData);
						}
					}
				}

				// 进入冷却
				if (Attacking.State == EAttackState::PostCast && Attacking.ATKTime == Attack.DurationPerRound)
				{
					Attacking.State = EAttackState::Cooling; // Do Once

					// Stop Animation
					Subject.SetFlag(AttackAnimFlag,false);

					// Attack Event Cooling 
					if (bHasIsSubjective)
					{
						FAttackData AttackData;
						AttackData.SelfSubject = FSubjectHandle(Subject);
						AttackData.State = EAttackEventState::Cooling;
						AttackData.AttackTarget = Tracing.TraceResult;
						OnAttackQueue.Enqueue(AttackData);
					}
				}

				// 冷却完成，本轮攻击结束
				if (Attacking.State == EAttackState::Cooling && Attacking.CoolTime == Attack.CoolDown)
				{
					Subject.RemoveTraitDeferred<FAttacking>(); // 不再攻击

					// Can trace again next frame ?
					bool bIsTargetValid = Tracing.TraceResult.IsValid() && !Tracing.TraceResult.HasTrait<FDying>() && Tracing.TraceResult.HasTrait<FLocated>() && Tracing.TraceResult.HasTrait<FHealth>() && Tracing.TraceResult.GetTrait<FHealth>().Current > 0;

					if (!bIsTargetValid) Tracing.TimeLeft = 0;

					// Attack Event Complete
					if (bHasIsSubjective)
					{
						FAttackData AttackData;
						AttackData.SelfSubject = FSubjectHandle(Subject);
						AttackData.State = EAttackEventState::End_Reason_Complete;
						AttackData.AttackTarget = Tracing.TraceResult;
						OnAttackQueue.Enqueue(AttackData);
					}
				}

				// 更新计时器
				if (Attacking.State == EAttackState::Aim)
				{
					Attacking.AimTime = FMath::Clamp(Attacking.AimTime + SafeDeltaTime, 0, Attack.MinAimTime);
				}
				else if (Attacking.State == EAttackState::PreCast_FirstExec || Attacking.State == EAttackState::PreCast || Attacking.State == EAttackState::PostCast)
				{
					Attacking.ATKTime = FMath::Clamp(Attacking.ATKTime + SafeDeltaTime, 0, Attack.DurationPerRound);
				}
				else if (Attacking.State == EAttackState::Cooling)
				{
					Attacking.CoolTime = FMath::Clamp(Attacking.CoolTime + SafeDeltaTime, 0, Attack.CoolDown);
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	//-------------------- 投射物 | Projectile ----------------------

	// 生成投射物 | Spawn Projectile
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SpawnProjectile");

		Mechanism->Operate<FUnsafeChain>(SpawnProjectileFilter,
			[&](FSubjectHandle Subject,
				FProjectileConfig_Final& Config)
			{
				// delay to spawn projectile
				if (!Config.bSpawned && Config.Delay == 0)
				{
					// 获取Data Asset
					auto DataAsset = Config.ProjectileConfigDataAsset.LoadSynchronous();

					if (!IsValid(DataAsset))
					{
						Subject.Despawn();
						return;
					}

					// 存储生成时的世界变换（用于后续相对位置计算）
					const FTransform SpawnWorldTransform = Config.SpawnTransform;

					// 生成子弹
					for (int32 i = 0; i < Config.Quantity; ++i)
					{
						bool Succeed;
						FSubjectHandle ProjectileHandle;
						FVector LaunchVelocity = FVector::ZeroVector;

						switch (DataAsset->MovementMode)
						{
							case EProjectileMoveMode::Static:
								UBattleFrameFunctionLibraryRT::SpawnProjectile_Static(Succeed, ProjectileHandle, Config.NeighborGrid, Config.ProjectileConfigDataAsset.LoadSynchronous(), SpawnWorldTransform.GetScale3D() * Config.Multipliers.Static.ScaleMult, Config.FromPoint, Config.OwnerSubject, FSubjectArray());
								break;

							case EProjectileMoveMode::Interped:
								UBattleFrameFunctionLibraryRT::SpawnProjectile_Interped(Succeed, ProjectileHandle, Config.NeighborGrid, Config.ProjectileConfigDataAsset.LoadSynchronous(), SpawnWorldTransform.GetScale3D() * Config.Multipliers.Interped.ScaleMult, Config.FromPoint, Config.ToPoint, Config.TargetSubject, Config.Multipliers.Interped.XOffsetMult, Config.Multipliers.Interped.YOffsetMult, Config.Multipliers.Interped.ZOffsetMult, Config.OwnerSubject, FSubjectArray());
								break;

							case EProjectileMoveMode::Ballistic:
								UBattleFrameFunctionLibraryRT::SpawnProjectile_Ballistic(Succeed, ProjectileHandle, Config.NeighborGrid, Config.ProjectileConfigDataAsset.LoadSynchronous(), SpawnWorldTransform.GetScale3D()* Config.Multipliers.Ballistic.ScaleMult, Config.FromPoint, Config.ToPoint, Config.TargetSubject, Config.TargetVelocity, FSubjectHandle(Subject), FSubjectArray());
								break;

							case EProjectileMoveMode::Tracking:
								UBattleFrameFunctionLibraryRT::SpawnProjectile_Tracking(Succeed, ProjectileHandle, Config.NeighborGrid, Config.ProjectileConfigDataAsset.LoadSynchronous(), SpawnWorldTransform.GetScale3D() * Config.Multipliers.Tracking.ScaleMult, Config.FromPoint, Config.ToPoint, Config.TargetSubject, (Config.ToPoint - Config.FromPoint).GetSafeNormal(), Config.OwnerSubject, FSubjectArray());
								break;
						}

						Config.SpawnedProjectiles.Add(ProjectileHandle);
					}

					Config.bSpawned = true;
				}

				// 更新附着对象位置
				bool bShouldUpdateAttachment = !Config.bSpawned || Config.bAttached;

				if (bShouldUpdateAttachment)
				{
					bool bCanUpdateAttachment = Config.AttachToSubject.IsValid() && Config.AttachToSubject.HasTrait<FDirected>() && Config.AttachToSubject.HasTrait<FLocated>();

					if (bCanUpdateAttachment)
					{
						// 获取宿主当前世界变换
						const FTransform CurrentAttachTransform(Config.AttachToSubject.GetTrait<FDirected>().Direction.Rotation(), Config.AttachToSubject.GetTrait<FLocated>().Location);

						// 计算新的世界变换 = 初始相对变换 * 宿主当前变换
						Config.SpawnTransform = Config.InitialRelativeTransform * CurrentAttachTransform;

						// 更新所有生成的粒子系统
						if (Config.bSpawned)
						{
							for (auto ProjectileHandle : Config.SpawnedProjectiles)
							{
								if (ProjectileHandle.IsValid())
								{
									ProjectileHandle.GetTraitRef<FLocated, EParadigm::Unsafe>().Location = Config.SpawnTransform.GetLocation();
									ProjectileHandle.GetTraitRef<FDirected, EParadigm::Unsafe>().Direction = Config.SpawnTransform.GetRotation().Vector();
								}
							}
						}
					}
				}

				// 检查生命周期
				if (Config.bSpawned)
				{
					bool bHasValidChild = false;

					for (auto ProjectileHandle : Config.SpawnedProjectiles)
					{
						if (ProjectileHandle.IsValid())
						{
							bHasValidChild = true;
							break;
						}
					}

					if (!bHasValidChild)
					{
						Subject.Despawn();
					}
				}

				// 更新计时器
				if (Config.Delay > 0)
				{
					Config.Delay = FMath::Max(0.f, Config.Delay - SafeDeltaTime);
				}
			});
	}
	#pragma endregion

	// 投射物运动与伤害 | Projectile Move and Dmg 
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("Projectile Move and Dmg");

		auto Chain = Mechanism->EnchainSolid(ProjectileFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FSubType& SubType,
				FProjectileParams& ProjectileParams,
				FProjectileParamsRT& ProjectileParamsRT,
				FLocated& Located,
				FDirected& Directed,
				FScaled& Scaled)
			{
				Located.PreLocation = Located.Location;

				const auto NeighborGrid = IsValid(ProjectileParamsRT.NeighborGridComponent) ? ProjectileParamsRT.NeighborGridComponent : NeighborGrids[0];

				const bool bIsInterped = Subject.HasTrait<FProjectileMove_Interped>(); // 插值
				const bool bIsBallistic = Subject.HasTrait<FProjectileMove_Ballistic>(); // 抛物线
				const bool bIsTracking = Subject.HasTrait<FProjectileMove_Tracking>(); // 追踪

				const bool bIsPoint = Subject.HasTrait<FDamage_Point>() && Subject.HasTrait<FDebuff_Point>(); // 点伤害
				const bool bIsRadial = Subject.HasTrait<FDamage_Radial>() && Subject.HasTrait<FDebuff_Radial>(); // 球形伤害
				const bool bIsBeam = Subject.HasTrait<FDamage_Beam>() && Subject.HasTrait<FDebuff_Beam>(); // 球扫伤害

				// Movement
				bool bArrived = false;
				FVector NewLocation = FVector::ZeroVector;

				if (bIsInterped)
				{
					auto& ProjectileMove_Interped = Subject.GetTraitRef<FProjectileMove_Interped>();
					auto& ProjectileMoving_Interped = Subject.GetTraitRef<FProjectileMoving_Interped>();

					UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Interped
					(
						bArrived,
						NewLocation,
						ProjectileMoving_Interped.FromPoint,
						ProjectileMoving_Interped.ToPoint,
						ProjectileMoving_Interped.Target,
						ProjectileMove_Interped.XOffset,
						ProjectileMoving_Interped.XOffsetMult,
						ProjectileMove_Interped.YOffset,
						ProjectileMoving_Interped.YOffsetMult,
						ProjectileMove_Interped.ZOffset,
						ProjectileMoving_Interped.ZOffsetMult,
						ProjectileMoving_Interped.BirthTime,
						CurrentWorld->GetTimeSeconds(),
						ProjectileMove_Interped.Speed
					);
				}
				else if (bIsBallistic)
				{
					auto& ProjectileMove_Ballistic = Subject.GetTraitRef<FProjectileMove_Ballistic>();
					auto& ProjectileMoving_Ballistic = Subject.GetTraitRef<FProjectileMoving_Ballistic>();

					UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Ballistic
					(
						bArrived,
						NewLocation,
						ProjectileMoving_Ballistic.FromPoint,
						ProjectileMoving_Ballistic.ToPoint,
						ProjectileMoving_Ballistic.BirthTime,
						CurrentWorld->GetTimeSeconds(),
						ProjectileMove_Ballistic.Gravity,
						ProjectileMoving_Ballistic.InitialVelocity
					);
				}
				else if (bIsTracking)
				{
					auto& ProjectileMove_Tracking = Subject.GetTraitRef<FProjectileMove_Tracking>();
					auto& ProjectileMoving_Tracking = Subject.GetTraitRef<FProjectileMoving_Tracking>();

					UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Tracking
					(
						bArrived,
						NewLocation,
						ProjectileMoving_Tracking.CurrentVelocity,
						Located.Location,
						ProjectileMoving_Tracking.ToPoint,
						ProjectileMoving_Tracking.Target,
						ProjectileMove_Tracking.Acceleration,
						ProjectileMove_Tracking.MaxSpeed,
						SafeDeltaTime,
						ProjectileParams.Radius
					);
				}

				// Trace for Subject
				float SubjectDistSqr = -1;
				bool bHitSubject = false;
				TArray<FTraceResult> HitSubjectResult;

				if (IsValid(NeighborGrid))
				{
					if (!ProjectileParams.bTraceOnlyOnArrival || bArrived)
					{
						UBattleFrameFunctionLibraryRT::SphereSweepForSubjects
						(
							bHitSubject,
							HitSubjectResult,
							NeighborGrid,
							1, // return the nearest one
							Located.Location, // sweep start
							NewLocation, // sweep end
							ProjectileParams.Radius, // collider radius
							false,// check obstacle
							FVector::ZeroVector, // check from
							0, // check sphere sweep radius
							ESortMode::NearToFar,
							Located.Location, // sort from
							ProjectileParamsRT.IgnoreSubjects,
							ProjectileParams.Filter,
							FTraceDrawDebugConfig()
						);

						if (bHitSubject)
						{
							SubjectDistSqr = FVector::DistSquared(Located.Location, HitSubjectResult[0].HitLocation); // 与碰撞点的距离
						}
					}
				}

				// Trace for obstacle
				float ObstacleDistSqr = -1;
				bool bHitObstacle = false;
				FHitResult HitObstacleResult;

				if (ProjectileParams.bCheckObstacle)
				{
					if (!ProjectileParams.bTraceOnlyOnArrival || bArrived)
					{
						bHitObstacle = UKismetSystemLibrary::SphereTraceSingleForObjects
						(
							CurrentWorld,
							Located.Location,
							NewLocation,
							ProjectileParams.Radius,
							ProjectileParams.Filter.ObstacleObjectType,
							true,
							TArray<TObjectPtr<AActor>>(),
							EDrawDebugTrace::None,
							HitObstacleResult,
							true,
							FLinearColor::Gray,
							FLinearColor::Red,
							1
						);

						if (bHitObstacle)
						{
							ObstacleDistSqr = FVector::DistSquared(Located.Location, HitObstacleResult.ImpactPoint); // 与碰撞点的距离
						}
					}
				}

				// Get the nearest result
				bool bCollided = true;

				if (SubjectDistSqr == -1 && ObstacleDistSqr == -1)
				{
					// no collision
					Located.Location = NewLocation;
					bCollided = false;
				}
				else if (SubjectDistSqr == -1)
				{
					// only collided with an obstacle
					Located.Location = HitObstacleResult.Location;
					ProjectileParams.Health--;
				}
				else if (ObstacleDistSqr == -1)
				{
					// only collided with a subject
					Located.Location = HitSubjectResult[0].ShapeLocation;
					ProjectileParams.Health--;
				}
				else
				{
					// collided with both a subject and an obstacle, so we choose the nearest result
					if (SubjectDistSqr <= ObstacleDistSqr)
					{
						Located.Location = HitObstacleResult.Location;
						ProjectileParams.Health--;
						bHitObstacle = false;
					}
					else
					{
						Located.Location = HitSubjectResult[0].ShapeLocation;
						ProjectileParams.Health--;
						bHitSubject = false;
					}
				}

				// 插值朝向
				Directed.DesiredDirection = (Located.Location - Located.PreLocation).GetSafeNormal2D();

				if (ProjectileParams.bRotationFollowVelocity)
				{
					Directed.Direction = Directed.DesiredDirection.Size() == 0 ? Directed.Direction : FMath::VInterpTo(Directed.Direction, Directed.DesiredDirection, SafeDeltaTime, 10);
				}

				bool bShouldDespawn = false;

				// RemoveOnNoHealth
				if (ProjectileParams.bRemoveOnNoHealth && ProjectileParams.Health <= 0)
				{
					bShouldDespawn = true;
				}

				// RemoveOnNoLifeSpan
				if (!ProjectileParams.bRemoveOnNoLifeSpan || ProjectileParams.LifeSpan > 0)
				{
					ProjectileParams.LifeSpan -= SafeDeltaTime;
				}
				else
				{
					bShouldDespawn = true;
				}

				// RemoveOnHitObstacle
				if (ProjectileParams.bRemoveOnHitObstacle && bHitObstacle)
				{
					bShouldDespawn = true;
				}

				// RemoveOnArrival
				if (ProjectileParams.bRemoveOnArrival && bArrived)
				{
					bShouldDespawn = true;
				}

				// Apply damage and debuff
				float DmgRadius = 0;

				if (bCollided || bArrived || bShouldDespawn)
				{
					// Clean ignore list
					if (!ProjectileParams.bHurtTargetOnlyOnce)
					{
						for (const auto& IgnoreSubject : ProjectileParamsRT.IgnoreSubjects.Subjects)
						{
							// remove from ignore list on end overlap, so on next begin overlap the subject can get damaged again
							const bool bShouldRemove = !IgnoreSubject.IsValid() || !IgnoreSubject.HasTrait<FLocated>() || FVector::DistSquared(IgnoreSubject.GetTrait<FLocated>().Location, Located.Location) > FMath::Square(ProjectileParams.Radius);

							if (bShouldRemove)
							{
								ProjectileParamsRT.IgnoreSubjects.Subjects.Remove(IgnoreSubject);
							}
						}
					}

					// Get DmgRadius
					if (bIsRadial)
					{
						DmgRadius = Subject.GetTraitRef<FDamage_Radial>().DmgRadius;
					}
					else if (bIsBeam)
					{
						DmgRadius = Subject.GetTraitRef<FDamage_Beam>().DmgRadius;
					}

					// Do apply dmg and debuff
					TArray<FDmgResult> DamageResults;

					if (bIsPoint)
					{
						const auto& Damage_Point = Subject.GetTraitRef<FDamage_Point>();
						const auto& Debuff_Point = Subject.GetTraitRef<FDebuff_Point>();

						FSubjectArray SubjectArray;
						SubjectArray.Subjects.Add(HitSubjectResult[0].Subject);

						ApplyPointDamageAndDebuffDeferred
						(
							SubjectArray,
							ProjectileParamsRT.IgnoreSubjects,
							ProjectileParamsRT.Instigator,
							FSubjectHandle(Subject),
							Located.Location,
							Damage_Point,
							Debuff_Point,
							DamageResults
						);
					}
					else if (bIsRadial)
					{
						const auto& Damage_Radial = Subject.GetTraitRef<FDamage_Radial>();
						const auto& Debuff_Radial = Subject.GetTraitRef<FDebuff_Radial>();

						ApplyRadialDamageAndDebuffDeferred
						(
							NeighborGrid,
							-1,
							Located.Location,
							ProjectileParamsRT.IgnoreSubjects,
							ProjectileParamsRT.Instigator,
							FSubjectHandle(Subject),
							Located.Location,
							Damage_Radial,
							Debuff_Radial,
							DamageResults
						);
					}
					else if (bIsBeam)
					{
						const auto& Damage_Beam = Subject.GetTraitRef<FDamage_Beam>();
						const auto& Debuff_Beam = Subject.GetTraitRef<FDebuff_Beam>();

						ApplyBeamDamageAndDebuffDeferred
						(
							NeighborGrid,
							-1,
							Located.Location,
							Located.Location + Damage_Beam.DmgDirectionAndDistance,
							ProjectileParamsRT.IgnoreSubjects,
							ProjectileParamsRT.Instigator,
							FSubjectHandle(Subject),
							Located.Location,
							Damage_Beam,
							Debuff_Beam,
							DamageResults
						);
					}

					// Add to ignore list
					for (const auto& DamageResult : DamageResults)
					{
						ProjectileParamsRT.IgnoreSubjects.Subjects.AddUnique(DamageResult.DamagedSubject);
					}

					// Hit particle burst
					FSubjectRecord BurstFxRecord;
					BurstFxRecord.SetTrait(FProjectile());
					BurstFxRecord.SetTrait(FIsBurstFx());
					BurstFxRecord.SetTrait(Located);
					BurstFxRecord.SetTrait(Directed);
					BurstFxRecord.SetTrait(Scaled);
					UBattleFrameFunctionLibraryRT::SetRecordSubTypeTraitByIndex(SubType.Index, BurstFxRecord);

					Mechanism->SpawnSubjectDeferred(BurstFxRecord);
				}

				if (bShouldDespawn)
				{
					Subject.DespawnDeferred();
				}

				// Draw Debug
				if (ProjectileParams.bDrawDebugShape)
				{
					FDebugSphereConfig ColliderRadius;
					ColliderRadius.Radius = ProjectileParams.Radius;
					ColliderRadius.Location = Located.Location;
					ColliderRadius.Color = FColor::Red;
					ColliderRadius.LineThickness = 0.f;
					DebugSphereQueue.Enqueue(ColliderRadius);

					FDebugSphereConfig DamageRadius;
					DamageRadius.Radius = DmgRadius;
					DamageRadius.Location = Located.Location;
					DamageRadius.Color = FColor::Red;
					DamageRadius.LineThickness = 0.f;
					DebugSphereQueue.Enqueue(DamageRadius);
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	//------------------------ 受击 | Hit ---------------------------

	// 受击反馈 | Hit Reaction
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SubjectBeingHit");
		 
		auto Chain = Mechanism->EnchainSolid(SubjectBeingHitFilter);// it processes hero and prop type too
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FScaled& Scaled,
				FHealth& Health,
				FBeingHit& BeingHit)
			{
				bool bCanRemoveBeingHit = true;

				// 结算伤害
				while (!Health.DamageToTake.IsEmpty() && !Health.DamageInstigator.IsEmpty())
				{
					// 如果怪物死了，跳出循环
					if (Health.Current <= 0) break;

					FSubjectHandle Instigator = FSubjectHandle();
					float DamageToTake = 0.f;
					FVector HitDirection = FVector::ZeroVector;

					Health.DamageInstigator.Dequeue(Instigator);
					Health.DamageToTake.Dequeue(DamageToTake);
					Health.HitDirection.Dequeue(HitDirection);

					if (!Health.bLockHealth)
					{
						bool bIsValidStats = false;
						FStatistics* Stats = nullptr;

						if (Instigator.IsValid())
						{
							if (Instigator.HasTrait<FStatistics>())
							{
								// 伤害与积分最后结算
								Stats = Instigator.GetTraitPtr<FStatistics, EParadigm::Unsafe>();

								if (Stats->bEnable)
								{
									bIsValidStats = true;
								}
							}
						}

						if (Health.Current - DamageToTake > 0) // 不是致命伤害
						{
							// 统计数据
							if (bIsValidStats)
							{
								Stats->Lock();
								Stats->TotalDamage += DamageToTake;
								Stats->Unlock();
							}
						}
						else // 是致命伤害
						{
							Subject.SetTraitDeferred(FDying{ false,0,0,0,0,Instigator,HitDirection });	// 标记为死亡

							if (Subject.HasTrait<FFall>())
							{
								Subject.GetTraitRef<FFall, EParadigm::Unsafe>().bCanFly = false; // 如果在飞行会掉下来
							}

							// 统计数据
							if (bIsValidStats)
							{
								int32 Score = Subject.HasTrait<FAgent>() ? Subject.GetTraitRef<FAgent, EParadigm::Unsafe>().Score : 0;

								Stats->Lock();
								Stats->TotalDamage += FMath::Min(DamageToTake, Health.Current);
								Stats->TotalKills += 1;
								Stats->TotalScore += Score;
								Stats->Unlock();
							}
						}

						// 扣除血量
						Health.Current -= FMath::Min(DamageToTake, Health.Current);
					}
				}

				// 血条
				const bool bHasHealthBar = Subject.HasTrait<FHealthBar>();

				if(bHasHealthBar)
				{
					auto& HealthBar = Subject.GetTraitRef<FHealthBar>();

					if (HealthBar.bShowHealthBar)
					{
						HealthBar.TargetRatio = FMath::Clamp(Health.Current / Health.Maximum, 0, 1);
						HealthBar.CurrentRatio = FMath::FInterpConstantTo(HealthBar.CurrentRatio, HealthBar.TargetRatio, SafeDeltaTime, HealthBar.InterpSpeed * 0.1);

						if (HealthBar.TargetRatio - HealthBar.CurrentRatio != 0)
						{
							bCanRemoveBeingHit = false;
						}

						if (HealthBar.HideOnFullHealth)
						{
							if (Health.Current == Health.Maximum)
							{
								HealthBar.Opacity = 0;
							}
							else
							{
								HealthBar.Opacity = 1;
							}
						}
						else
						{
							HealthBar.Opacity = 1;
						}

						if (HealthBar.HideOnEmptyHealth && Health.Current <= 0)
						{
							HealthBar.Opacity = 0;
						}
					}
					else
					{
						HealthBar.Opacity = 0;
					}
				}

				// 发光,形变,动画
				const bool bHasHit = Subject.HasTrait<FHit>();
				const bool bHasCurves = Subject.HasTrait<FCurves>();
				const bool bHasAnimating = Subject.HasTrait<FAnimating>();

				if(bHasHit && bHasCurves && bHasAnimating)
				{
					auto& Hit = Subject.GetTraitRef<FHit>();
					auto& Curves = Subject.GetTraitRef<FCurves>();
					auto& Animating = Subject.GetTraitRef<FAnimating>();

					//------------------------ 受击发光 | Hit Glow -------------------------

					const bool bIsGlowing = Subject.HasFlag(HitGlowFlag);

					if (bIsGlowing)
					{
						// 获取曲线
						auto GlowCurve = Curves.HitEmission.GetRichCurve();

						// 检查曲线是否有关键帧
						if (!GlowCurve || GlowCurve->GetNumKeys() == 0) return;

						// 获取曲线的最后一个关键帧的时间
						const auto GlowEndTime = GlowCurve->GetLastKey().Time;

						// 受击发光
						Animating.HitGlow = GlowCurve->Eval(BeingHit.GlowTime);

						// 更新发光时间
						if (BeingHit.GlowTime < GlowEndTime)
						{
							BeingHit.GlowTime += SafeDeltaTime;
						}

						// 计时器完成后删除 Trait
						if (BeingHit.GlowTime >= GlowEndTime)
						{
							Animating.HitGlow = 0; // 重置发光值
							Subject.SetFlag(HitGlowFlag, false);
						}
						else
						{
							bCanRemoveBeingHit = false;
						}
					}

					//------------------------ 受击形变 | Hit Jiggle -------------------------

					const bool bIsJiggling = Subject.HasFlag(HitJiggleFlag);

					if (bIsJiggling)
					{
						// 获取曲线
						auto JiggleCurve = Curves.HitJiggle.GetRichCurve();

						// 检查曲线是否有关键帧
						if (!JiggleCurve || JiggleCurve->GetNumKeys() == 0) return;

						// 获取曲线的最后一个关键帧的时间
						const auto JiggleEndTime = JiggleCurve->GetLastKey().Time;

						// 受击变形
						Scaled.JiggleMultiplier.X = (JiggleCurve->Eval(BeingHit.JiggleTime) - 1) * -Hit.JiggleStr * 0.5 + 1;
						Scaled.JiggleMultiplier.Y = (JiggleCurve->Eval(BeingHit.JiggleTime) - 1) * -Hit.JiggleStr * 0.5 + 1;
						Scaled.JiggleMultiplier.Z = (JiggleCurve->Eval(BeingHit.JiggleTime) - 1) * -Hit.JiggleStr *  -1 + 1;

						// 更新形变时间
						if (BeingHit.JiggleTime < JiggleEndTime)
						{
							BeingHit.JiggleTime += SafeDeltaTime;
						}

						// 计时器完成后删除 Trait
						if (BeingHit.JiggleTime >= JiggleEndTime)
						{
							Subject.SetFlag(HitJiggleFlag, false);
						}
						else
						{
							bCanRemoveBeingHit = false;
						}
					}

					//------------------------ 受击动画 | Hit Anim -------------------------

					const bool bIsAnimating = Subject.HasFlag(HitAnimFlag);

					if (bIsAnimating)
					{
						if (BeingHit.AnimTime >= Hit.AnimLength)
						{
							Subject.SetFlag(HitAnimFlag, false);
						}
						else
						{
							bCanRemoveBeingHit = false;
						}

						BeingHit.AnimTime += SafeDeltaTime;
					}
				}

				// 全部执行完成后移除BeingHit状态
				if (bCanRemoveBeingHit)
				{
					Subject.RemoveTraitDeferred<FBeingHit>();
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 减速马甲 | Slow Ghost Subject
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentSlowed");

		auto Chain = Mechanism->EnchainSolid(SlowFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject, 
				FSlow& Slow)
			{
				// 减速对象不存在时终止
				if (!Slow.SlowTarget.IsValid())
				{
					Subject.DespawnDeferred();
					return;
				}

				// 第一次运行时，登记到agent的减速马甲列表
				if (Slow.bJustSpawned)
				{
					auto& TargetSlowing = Slow.SlowTarget.GetTraitRef<FSlowing, EParadigm::Unsafe>();

					TargetSlowing.Lock();
					TargetSlowing.Slows.Add(FSubjectHandle(Subject));
					TargetSlowing.Unlock();

					const bool bHasAnimating = Slow.SlowTarget.HasTrait<FAnimating>();

					// 开启材质特效
					if (bHasAnimating)
					{
						auto& TargetAnimating = Slow.SlowTarget.GetTraitRef<FAnimating, EParadigm::Unsafe>();

						TargetAnimating.Lock();
						switch (Slow.DmgType)
						{
							case EDmgType::Fire:
								TargetAnimating.FireFx = 1;
								break;
							case EDmgType::Ice:
								TargetAnimating.IceFx = 1;
								break;
							case EDmgType::Poison:
								TargetAnimating.PoisonFx = 1;
								break;
						}
						TargetAnimating.Unlock();
					}

					Slow.bJustSpawned = false;
				}

				// 持续时间结束，解除减速
				if (Slow.SlowTimeout <= 0)
				{
					auto& TargetSlowing = Slow.SlowTarget.GetTraitRef<FSlowing, EParadigm::Unsafe>();

					TargetSlowing.Lock();
					TargetSlowing.Slows.Remove(FSubjectHandle(Subject));
					TargetSlowing.Unlock();

					const bool bHasAnimating = Slow.SlowTarget.HasTrait<FAnimating>();

					// 重置材质特效
					if (bHasAnimating)
					{
						bool bHasSameDmgType = false;

						// 是否还存在同伤害类型的减速马甲
						TargetSlowing.Lock();
						for (const auto& OtherSlow : TargetSlowing.Slows)
						{
							if (OtherSlow.GetTrait<FSlow>().DmgType == Slow.DmgType)
							{
								bHasSameDmgType = true;
								break;
							}
						}
						TargetSlowing.Unlock();

						// 是否还存在同伤害类型的延时伤害马甲
						auto& TargetTemporalDamaging = Slow.SlowTarget.GetTraitRef<FTemporalDamaging, EParadigm::Unsafe>();

						TargetTemporalDamaging.Lock();
						for (const auto& OtherTemporalDamage : TargetTemporalDamaging.TemporalDamages)
						{
							if (OtherTemporalDamage.GetTrait<FTemporalDamage>().DmgType == Slow.DmgType)
							{
								bHasSameDmgType = true;
								break;
							}
						}
						TargetTemporalDamaging.Unlock();

						// 如果没有同伤害类型的马甲，可以重置材质特效了
						if (!bHasSameDmgType)
						{
							auto& TargetAnimating = Slow.SlowTarget.GetTraitRef<FAnimating, EParadigm::Unsafe>();

							TargetAnimating.Lock();
							switch (Slow.DmgType)
							{
								case EDmgType::Fire:
									TargetAnimating.FireFx = 0;
									break;
								case EDmgType::Ice:
									TargetAnimating.IceFx = 0;
									break;
								case EDmgType::Poison:
									TargetAnimating.PoisonFx = 0;
									break;
							}
							TargetAnimating.Unlock();
						}
					}

					Subject.DespawnDeferred();
					return;
				}

				// 更新计时器
				Slow.SlowTimeout -= SafeDeltaTime;

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 延时伤害马甲 | Temporal Damager Ghost Subject
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentTemporalDamaging");

		auto Chain = Mechanism->EnchainSolid(TemporalDamageFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject, 
				FTemporalDamage& TemporalDamage)
			{
				// 伤害对象不存在时终止
				if (!TemporalDamage.TemporalDamageTarget.IsValid())
				{
					Subject.DespawnDeferred();
					return;
				}

				// 第一次运行时，登记到agent的持续伤害马甲列表
				if (TemporalDamage.bJustSpawned)
				{
					auto& TargetTemporalDamaging = TemporalDamage.TemporalDamageTarget.GetTraitRef<FTemporalDamaging, EParadigm::Unsafe>();

					TargetTemporalDamaging.Lock();
					TargetTemporalDamaging.TemporalDamages.Add(FSubjectHandle(Subject));
					TargetTemporalDamaging.Unlock();

					const bool bHasAnimating = TemporalDamage.TemporalDamageTarget.HasTrait<FAnimating>();

					if (bHasAnimating)
					{
						auto& TargetAnimating = TemporalDamage.TemporalDamageTarget.GetTraitRef<FAnimating, EParadigm::Unsafe>();

						TargetAnimating.Lock();
						switch (TemporalDamage.DmgType)
						{
							case EDmgType::Fire:
								TargetAnimating.FireFx = 1;
								break;
							case EDmgType::Ice:
								TargetAnimating.IceFx = 1;
								break;
							case EDmgType::Poison:
								TargetAnimating.PoisonFx = 1;
								break;
						}
						TargetAnimating.Unlock();
					}

					TemporalDamage.bJustSpawned = false;
				}

				// 持续伤害结束时终止
				if (TemporalDamage.RemainingTemporalDamage <= 0 || TemporalDamage.CurrentSegment >= TemporalDamage.TemporalDmgSegment)
				{
					auto& TargetTemporalDamaging = TemporalDamage.TemporalDamageTarget.GetTraitRef<FTemporalDamaging, EParadigm::Unsafe>();

					// 从马甲列表移除
					TargetTemporalDamaging.Lock();
					TargetTemporalDamaging.TemporalDamages.Remove(FSubjectHandle(Subject));
					TargetTemporalDamaging.Unlock();

					const bool bHasAnimating = TemporalDamage.TemporalDamageTarget.HasTrait<FAnimating>();

					// 重置材质特效
					if (bHasAnimating)
					{
						bool bHasSameDmgType = false;

						// 是否还存在同伤害类型的延时伤害马甲
						TargetTemporalDamaging.Lock();
						for (const auto& OtherTemporalDamage : TargetTemporalDamaging.TemporalDamages)
						{
							if (OtherTemporalDamage.GetTrait<FTemporalDamage>().DmgType == TemporalDamage.DmgType)
							{
								bHasSameDmgType = true;
								break;
							}
						}
						TargetTemporalDamaging.Unlock();

						// 是否还存在同伤害类型的减速马甲
						auto& TargetSlowing = TemporalDamage.TemporalDamageTarget.GetTraitRef<FSlowing, EParadigm::Unsafe>();

						TargetSlowing.Lock();
						for (const auto& OtherSlow : TargetSlowing.Slows)
						{
							if (OtherSlow.GetTrait<FSlow>().DmgType == TemporalDamage.DmgType)
							{
								bHasSameDmgType = true;
								break;
							}
						}
						TargetSlowing.Unlock();

						// 如果没有同伤害类型的马甲，可以重置材质特效了
						if (!bHasSameDmgType)
						{
							auto& TargetAnimating = TemporalDamage.TemporalDamageTarget.GetTraitRef<FAnimating, EParadigm::Unsafe>();

							TargetAnimating.Lock();
							switch (TemporalDamage.DmgType)
							{
								case EDmgType::Fire:
									TargetAnimating.FireFx = 0;
									break;
								case EDmgType::Ice:
									TargetAnimating.IceFx = 0;
									break;
								case EDmgType::Poison:
									TargetAnimating.PoisonFx = 0;
									break;
							}
							TargetAnimating.Unlock();
						}
					}

					Subject.DespawnDeferred();
					return;
				}

				TemporalDamage.TemporalDamageTimeout -= SafeDeltaTime;

				// 倒计时结束，造成一次伤害
				if (TemporalDamage.TemporalDamageTimeout <= 0)
				{
					// 计算本次伤害值
					float ThisSegmentDamage = 0.0f;

					// 扣除目标生命值
					if (TemporalDamage.TemporalDamageTarget.HasTrait<FHealth>())
					{
						auto& TargetHealth = TemporalDamage.TemporalDamageTarget.GetTraitRef<FHealth, EParadigm::Unsafe>();

						if (TargetHealth.Current > 0)
						{
							// 计算本次伤害值
							float DamagePerSegment = TemporalDamage.TotalTemporalDamage / TemporalDamage.TemporalDmgSegment;

							// 确保最后一段使用剩余伤害值
							if (TemporalDamage.CurrentSegment == TemporalDamage.TemporalDmgSegment - 1)
							{
								ThisSegmentDamage = TemporalDamage.RemainingTemporalDamage;
							}
							else
							{
								ThisSegmentDamage = FMath::Min(DamagePerSegment, TemporalDamage.RemainingTemporalDamage);
							}

							float ClampedDamage = FMath::Min(ThisSegmentDamage, TargetHealth.Current);

							// 应用伤害
							TargetHealth.DamageToTake.Enqueue(ClampedDamage);

							// 记录伤害施加者
							TargetHealth.DamageInstigator.Enqueue(TemporalDamage.TemporalDamageInstigator);

							TargetHealth.HitDirection.Enqueue(FVector(0,0,0.0001f));

							//Temporal.TemporalDamageTarget.SetFlag(NeedSettleDmgFlag, true);

							// 生成伤害数字
							if (TemporalDamage.TemporalDamageTarget.HasTrait<FTextPopUp>())
							{
								const auto& TextPopUp = TemporalDamage.TemporalDamageTarget.GetTraitRef<FTextPopUp, EParadigm::Unsafe>();

								if (TextPopUp.Enable)
								{
									float Style;

									if (ClampedDamage < TextPopUp.WhiteTextBelowPercent)
									{
										Style = 0;
									}
									else if (ClampedDamage < TextPopUp.OrangeTextAbovePercent)
									{
										Style = 1;
									}
									else
									{
										Style = 2;
									}

									float Radius = TemporalDamage.TemporalDamageTarget.HasTrait<FGridData>() ? TemporalDamage.TemporalDamageTarget.GetTraitRef<FGridData, EParadigm::Unsafe>().Radius : 0;
									FVector Location = TemporalDamage.TemporalDamageTarget.HasTrait<FLocated>() ? TemporalDamage.TemporalDamageTarget.GetTraitRef<FLocated, EParadigm::Unsafe>().Location : FVector::ZeroVector;

									QueueText(FTextPopConfig(TemporalDamage.TemporalDamageTarget, ClampedDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
								}
							}
						}
					}

					// 更新伤害状态
					TemporalDamage.RemainingTemporalDamage -= ThisSegmentDamage;
					TemporalDamage.CurrentSegment++;

					// 重置倒计时（仅当还有剩余伤害段数时）
					if (TemporalDamage.CurrentSegment < TemporalDamage.TemporalDmgSegment && TemporalDamage.RemainingTemporalDamage > 0)
					{
						TemporalDamage.TemporalDamageTimeout = TemporalDamage.TemporalDmgInterval;
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 死亡 | Death
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentDeath");

		auto Chain = Mechanism->EnchainSolid(AgentDeathFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FLocated& Located,
				FDirected& Directed,
				FDeath& Death,
				FDying& Dying,
				FMoving& Moving,
				FAnimating& Animating,
				FCurves& Curves)
			{
				if (!Death.bEnable) return;
				const bool bHasIsSubjective = Subject.HasTrait<FIsSubjective>();

				// Init, do once
				if (Dying.Time == 0 && !Dying.bInitialized)
				{
					Dying.bInitialized = true;

					// Actor
					for (const FActorSpawnConfig& Config : Death.SpawnActor)
					{
						FActorSpawnConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Dying.HitDirection.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));
						
						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Fx
					for (const FFxConfig& Config : Death.SpawnFx)
					{
						FFxConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Dying.HitDirection.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));
						NewConfig.LaunchSpeed = Dying.HitDirection.Size();
						
						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Sound
					for (const FSoundConfig& Config : Death.PlaySound)
					{
						FSoundConfig_Final NewConfig(Config);
						NewConfig.OwnerSubject = FSubjectHandle(Subject);
						NewConfig.AttachToSubject = FSubjectHandle(Subject);
						NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Dying.HitDirection.ToOrientationQuat(), Located.Location, NewConfig.Transform);
						NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(FTransform(Directed.Direction.ToOrientationQuat(), Located.Location));
						
						Mechanism->SpawnSubjectDeferred(NewConfig);
					}

					// Death Begin Event
					if (bHasIsSubjective)
					{
						FDeathData DeathData;
						DeathData.SelfSubject = FSubjectHandle(Subject);
						OnDeathQueue.Enqueue(DeathData);
					}

					// Sub-Status
					if (Death.DespawnDelay > 0)
					{
						Dying.Duration = Death.DespawnDelay;

						// Anim
						if (Death.bCanPlayAnim)
						{
							Subject.SetFlag(DeathAnimFlag);
						}

						// Fade out
						if (Death.bCanFadeout)
						{
							Subject.SetFlag(DeathDissolveFlag);
						}
					}
				}

				if (Dying.Time < Dying.Duration)
				{
					Dying.Time += SafeDeltaTime; // 计时

					// 关闭碰撞
					//if (Death.bDisableCollision && !Subject.HasFlag(DeathDisableCollisionFlag) && Moving.CurrentVelocity.Size2D() < 0)
					//{
					//	Subject.SetFlag(DeathDisableCollisionFlag);
					//}

					// 死亡消融					
					if (Subject.HasFlag(DeathDissolveFlag))
					{
						// 获取曲线
						auto Curve = Curves.DissolveOut.GetRichCurve();

						// 检查曲线是否有关键帧
						if (!Curve || Curve->GetNumKeys() == 0) return;

						// 获取曲线的最后一个关键帧的时间
						const auto EndTime = Curve->GetLastKey().Time;

						// 计算溶解效果
						if (Dying.DeathDissolveTime >= Death.FadeOutDelay && (Dying.DeathDissolveTime - Death.FadeOutDelay) < EndTime)
						{
							Animating.Dissolve = 1 - Curve->Eval(Dying.DeathDissolveTime - Death.FadeOutDelay);
						}

						// 更新溶解时间
						Dying.DeathDissolveTime += SafeDeltaTime;
					}
				}
				else
				{
					Subject.DespawnDeferred(); // 移除
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	//-------------- 游戏线程逻辑 | Game Thread Logic ---------------

	// 生成Actor | Spawn Actor
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SpawnActors");

		Mechanism->Operate<FUnsafeChain>(SpawnActorFilter,
			[&](FSubjectHandle Subject,
				FActorSpawnConfig_Final& Config)
			{
				// delay to spawn actors
				if (!Config.bSpawned && Config.Delay == 0)
				{
					// Spawn actors
					if (Config.bEnable && Config.Quantity > 0)
					{
						if (!IsValid(Config.ActorClass)) Config.ActorClass = Config.SoftActorClass.LoadSynchronous();

						if (IsValid(Config.ActorClass))
						{
							FActorSpawnParameters SpawnParams;
							SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

							// 存储生成时的世界变换（用于后续相对位置计算）
							const FTransform SpawnWorldTransform = Config.SpawnTransform;

							for (int32 i = 0; i < Config.Quantity; ++i)
							{
								TObjectPtr<AActor> Actor = CurrentWorld->SpawnActor<AActor>(Config.ActorClass, SpawnWorldTransform, SpawnParams);

								if (IsValid(Actor))
								{
									Config.SpawnedActors.Add(Actor);
									Actor->SetActorScale3D(SpawnWorldTransform.GetScale3D());

									// 直接设置Owner关系
									if (USubjectiveActorComponent* SubjectiveComponent = Actor->FindComponentByClass<USubjectiveActorComponent>())
									{
										FSubjectHandle Subjective = SubjectiveComponent->GetHandle();
										if (Subjective.HasTrait<FOwnerSubject>())
										{
											auto& OwnerTrait = Subjective.GetTraitRef<FOwnerSubject, EParadigm::Unsafe>();
											OwnerTrait.Owner = Config.OwnerSubject;
											OwnerTrait.Host = Subject;
										}
									}
								}
							}
						}
					}
					Config.bSpawned = true;
				}

				// 更新附着对象位置
				bool bShouldUpdateAttachment = !Config.bSpawned || Config.bAttached;

				if (bShouldUpdateAttachment)
				{
					bool bCanUpdateAttachment = Config.AttachToSubject.IsValid() && Config.AttachToSubject.HasTrait<FDirected>() && Config.AttachToSubject.HasTrait<FLocated>();

					if (bCanUpdateAttachment)
					{
						// 获取宿主当前世界变换
						const FTransform CurrentAttachTransform(Config.AttachToSubject.GetTrait<FDirected>().Direction.ToOrientationQuat(), Config.AttachToSubject.GetTrait<FLocated>().Location);

						// 计算新的世界变换 = 初始相对变换 * 宿主当前变换
						Config.SpawnTransform = Config.InitialRelativeTransform * CurrentAttachTransform;

						// 更新所有生成的Actor
						if (Config.bSpawned)
						{
							for (auto& Actor : Config.SpawnedActors)
							{
								if (IsValid(Actor))
								{
									Actor->SetActorTransform(Config.SpawnTransform);
								}
							}
						}
					}
				}

				// 检查生命周期
				if (Config.bSpawned)
				{
					bool bHasValidChild = false;
					for (const auto& Actor : Config.SpawnedActors)
					{
						if (IsValid(Actor))
						{
							bHasValidChild = true;
							break;
						}
					}

					const bool bLifeIsInfinite = Config.LifeSpan < 0;

					if (!bLifeIsInfinite)
					{
						const bool bLifeExpired = Config.LifeSpan == 0;
						const bool bInvalidAttachment = Config.bAttached && !Config.AttachToSubject.IsValid();

						if (!bHasValidChild || bLifeExpired || bInvalidAttachment)
						{
							for (auto& Actor : Config.SpawnedActors)
							{
								if (IsValid(Actor)) Actor->Destroy();
							}
							Subject.Despawn();
						}
					}
				}

				// 更新计时器
				if (Config.Delay > 0)
				{
					Config.Delay = FMath::Max(0.f, Config.Delay - SafeDeltaTime);
				}
				else if (Config.LifeSpan > 0)
				{
					Config.LifeSpan = FMath::Max(0.f, Config.LifeSpan - SafeDeltaTime);
				}

			});
	}
	#pragma endregion

	// 生成粒子 | Spawn Fx
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SpawnFx");

		Mechanism->Operate<FUnsafeChain>(SpawnFxFilter,
			[&](FSubjectHandle Subject,
				FFxConfig_Final& Config)
			{
				// delay to spawn Fx
				if (!Config.bSpawned && Config.Delay == 0)
				{
					// 存储生成时的世界变换（用于后续相对位置计算）
					const FTransform SpawnWorldTransform = Config.SpawnTransform;

					// 合批情况下的SubType
					if (Config.SubType != EESubType::None)
					{
						FLocated FxLocated = { SpawnWorldTransform.GetLocation() };
						FDirected FxDirected = { SpawnWorldTransform.GetRotation().GetForwardVector() * Config.LaunchSpeed };
						FScaled FxScaled = { 1, SpawnWorldTransform.GetScale3D() };

						FSubjectRecord FxRecord;
						FxRecord.SetTrait(FSpawningFx());
						FxRecord.SetTrait(FIsBurstFx());
						FxRecord.SetTrait(FxLocated);
						FxRecord.SetTrait(FxDirected);
						FxRecord.SetTrait(FxScaled);

						UBattleFrameFunctionLibraryRT::SetRecordSubTypeTraitByEnum(Config.SubType, FxRecord);

						for (int32 i = 0; i < Config.Quantity; ++i)
						{
							Mechanism->SpawnSubject(FxRecord);
						}
					}

					// 处理非合批情况
					if (!IsValid(Config.NiagaraAsset)) Config.NiagaraAsset = Config.SoftNiagaraAsset.LoadSynchronous();
					if (!IsValid(Config.CascadeAsset)) Config.CascadeAsset = Config.SoftCascadeAsset.LoadSynchronous();

					for (int32 i = 0; i < Config.Quantity; ++i)
					{
						if (IsValid(Config.NiagaraAsset))
						{
							auto NS = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
								CurrentWorld,
								Config.NiagaraAsset,
								SpawnWorldTransform.GetLocation(),
								SpawnWorldTransform.GetRotation().Rotator(),
								SpawnWorldTransform.GetScale3D(),
								true,  // bAutoDestroy
								true,  // bAutoActivate
								ENCPoolMethod::AutoRelease);

							Config.SpawnedNiagaraSystems.Add(NS);
						}

						if (IsValid(Config.CascadeAsset))
						{
							auto CS = UGameplayStatics::SpawnEmitterAtLocation(
								CurrentWorld,
								Config.CascadeAsset,
								SpawnWorldTransform.GetLocation(),
								SpawnWorldTransform.GetRotation().Rotator(),
								SpawnWorldTransform.GetScale3D(),
								true,  // bAutoDestroy
								EPSCPoolMethod::AutoRelease);

							Config.SpawnedCascadeSystems.Add(CS);
						}
					}

					Config.bSpawned = true;
				}

				// 更新附着对象位置
				bool bShouldUpdateAttachment = !Config.bSpawned || Config.bAttached;

				if (bShouldUpdateAttachment)
				{
					bool bCanUpdateAttachment = Config.AttachToSubject.IsValid() && Config.AttachToSubject.HasTrait<FDirected>() && Config.AttachToSubject.HasTrait<FLocated>();

					if (bCanUpdateAttachment)
					{
						// 获取宿主当前世界变换
						const FTransform CurrentAttachTransform(Config.AttachToSubject.GetTrait<FDirected>().Direction.Rotation(), Config.AttachToSubject.GetTrait<FLocated>().Location);

						// 计算新的世界变换 = 初始相对变换 * 宿主当前变换
						Config.SpawnTransform = Config.InitialRelativeTransform * CurrentAttachTransform;

						// 更新所有生成的粒子系统
						if (Config.bSpawned)
						{
							for (auto& Fx : Config.SpawnedNiagaraSystems)
							{
								if (IsValid(Fx))
								{
									Fx->SetWorldTransform(Config.SpawnTransform);
								}
							}

							for (auto& Fx : Config.SpawnedCascadeSystems)
							{
								if (IsValid(Fx))
								{
									Fx->SetWorldTransform(Config.SpawnTransform);
								}
							}
						}
					}
				}

				// 检查生命周期
				if (Config.bSpawned)
				{
					bool bHasValidChild = false;
					for (auto& Fx : Config.SpawnedNiagaraSystems)
					{
						if (IsValid(Fx))
						{
							bHasValidChild = true;
							break;
						}
					}

					if (!bHasValidChild)
					{
						for (auto& Fx : Config.SpawnedCascadeSystems)
						{
							if (IsValid(Fx))
							{
								bHasValidChild = true;
								break;
							}
						}
					}

					const bool bLifeIsInfinite = Config.LifeSpan < 0;

					if (!bLifeIsInfinite)
					{
						const bool bLifeExpired = Config.LifeSpan == 0;
						const bool bInvalidAttachment = Config.bAttached && !Config.AttachToSubject.IsValid();

						if (!bHasValidChild || bLifeExpired || bInvalidAttachment)
						{
							for (auto& Fx : Config.SpawnedNiagaraSystems)
							{
								if (IsValid(Fx)) Fx->DestroyComponent();
							}
							for (auto& Fx : Config.SpawnedCascadeSystems)
							{
								if (IsValid(Fx)) Fx->DestroyComponent();
							}
							Subject.Despawn();
						}
					}
				}

				// 更新计时器
				if (Config.Delay > 0)
				{
					Config.Delay = FMath::Max(0.f, Config.Delay - SafeDeltaTime);
				}
				else if (Config.LifeSpan > 0)
				{
					Config.LifeSpan = FMath::Max(0.f, Config.LifeSpan - SafeDeltaTime);
				}
			});
	}
	#pragma endregion

	// 播放音效 | Play Sound
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("PlaySound");

		Mechanism->Operate<FUnsafeChain>(PlaySoundFilter,
			[&](FSubjectHandle Subject,
				FSoundConfig_Final& Config)
			{
				// delay to play sound
				if (!Config.bSpawned && Config.Delay <= 0)
				{
					if (Config.Sound && Config.bEnable)
					{
						// 存储生成时的世界变换（用于后续相对位置计算）
						const FTransform SpawnWorldTransform = Config.SpawnTransform;

						// 播放加载完成的音效
						StreamableManager.RequestAsyncLoad(Config.Sound.ToSoftObjectPath(), FStreamableDelegate::CreateLambda([this, &Config, SpawnWorldTransform, Subject]()
							{
								if (Config.SpawnOrigin == EPlaySoundOrigin::PlaySound2D)
								{
									// 2D音效直接播放，不处理附着
									auto AudioComp = UGameplayStatics::CreateSound2D(
										GetWorld(),
										Config.Sound.Get(),
										Config.Volume);
									Config.SpawnedSounds.Add(AudioComp);
								}
								else
								{
									// 3D音效处理位置和附着
									FTransform PlayTransform = SpawnWorldTransform;

									if (Config.bAttached && Config.AttachToSubject.IsValid())
									{
										const FTransform CurrentAttachTransform(Config.AttachToSubject.GetTrait<FDirected>().Direction.Rotation(), Config.AttachToSubject.GetTrait<FLocated>().Location);
										PlayTransform = Config.InitialRelativeTransform * CurrentAttachTransform;
									}

									auto AudioComp = UGameplayStatics::SpawnSoundAtLocation(
										GetWorld(),
										Config.Sound.Get(),
										PlayTransform.GetLocation(),
										PlayTransform.Rotator(),
										Config.Volume);
									Config.SpawnedSounds.Add(AudioComp);
								}
							}));
					}
					Config.bSpawned = true;
				}

				// 更新附着对象位置（仅对3D音效有效）
				bool bShouldUpdateAttachment = !Config.bSpawned || Config.bAttached;

				if (bShouldUpdateAttachment)
				{
					bool bCanUpdateAttachment = Config.AttachToSubject.IsValid() && Config.AttachToSubject.HasTrait<FDirected>() && Config.AttachToSubject.HasTrait<FLocated>();

					if (bCanUpdateAttachment)
					{
						// 获取宿主当前世界变换
						const FTransform CurrentAttachTransform(Config.AttachToSubject.GetTrait<FDirected>().Direction.Rotation(), Config.AttachToSubject.GetTrait<FLocated>().Location);

						// 计算新的世界变换 = 初始相对变换 * 宿主当前变换
						Config.SpawnTransform = Config.InitialRelativeTransform * CurrentAttachTransform;

						// 更新所有生成的音效位置
						if (Config.bSpawned)
						{
							for (auto& AudioComp : Config.SpawnedSounds)
							{
								if (IsValid(AudioComp))
								{
									AudioComp->SetWorldLocationAndRotation(Config.SpawnTransform.GetLocation(), Config.SpawnTransform.Rotator());
								}
							}
						}
					}
				}

				// 检查生命周期
				if (Config.bSpawned)
				{
					bool bHasValidChild = false;

					for (auto& AudioComp : Config.SpawnedSounds)
					{
						if (IsValid(AudioComp) && AudioComp->IsPlaying())
						{
							bHasValidChild = true;
							break;
						}
					}

					const bool bLifeIsInfinite = Config.LifeSpan < 0;

					if (!bLifeIsInfinite)
					{
						const bool bLifeExpired = Config.LifeSpan == 0;
						const bool bInvalidAttachment = Config.bAttached && Config.bDespawnWhenNoParent && !Config.AttachToSubject.IsValid();

						if (!bHasValidChild || bLifeExpired || bInvalidAttachment)
						{
							for (auto& AudioComp : Config.SpawnedSounds)
							{
								if (IsValid(AudioComp))
								{
									AudioComp->Stop();
									AudioComp->DestroyComponent();
								}
							}
							Subject.Despawn();
						}
					}
				}

				// 更新计时器
				if (Config.Delay > 0)
				{
					Config.Delay = FMath::Max(0.f, Config.Delay - SafeDeltaTime);
				}
				else if (Config.LifeSpan > 0)
				{
					Config.LifeSpan = FMath::Max(0.f, Config.LifeSpan - SafeDeltaTime);
				}
			});
	}
	#pragma endregion

	// 事件接口 | Event Interface
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("EventInterface");

		while (!OnAppearQueue.IsEmpty())
		{
			FAppearData Data;
			OnAppearQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnAppear(DmgActor, Data);
				}
			}
		}

		while (!OnTraceQueue.IsEmpty())
		{
			FTraceData Data;
			OnTraceQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnTrace(DmgActor, Data);
				}
			}
		}

		while (!OnMoveQueue.IsEmpty())
		{
			FMoveData Data;
			OnMoveQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnMove(DmgActor, Data);
				}
			}
		}

		while (!OnAttackQueue.IsEmpty())
		{
			FAttackData Data;
			OnAttackQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnAttack(DmgActor, Data);
				}
			}
		}

		while (!OnHitQueue.IsEmpty())
		{
			FHitData Data;
			OnHitQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnHit(DmgActor, Data);
				}
			}
		}

		while (!OnDeathQueue.IsEmpty())
		{
			FDeathData Data;
			OnDeathQueue.Dequeue(Data);

			if (Data.SelfSubject.IsValid())
			{
				TObjectPtr<AActor> DmgActor = Data.SelfSubject.GetSubjective()->GetActor();

				if (DmgActor && DmgActor->GetClass()->ImplementsInterface(UBattleFrameInterface::StaticClass()))
				{
					IBattleFrameInterface::Execute_OnDeath(DmgActor, Data);
				}
			}
		}
	}
	#pragma endregion

	// 调试图形 | Draw Debug Shapes
	#pragma region
	{
		// 绘制点队列
		while (!DebugPointQueue.IsEmpty())
		{
			FDebugPointConfig Config;
			DebugPointQueue.Dequeue(Config);

			DrawDebugPoint(
				CurrentWorld,
				Config.Location,
				Config.Size,
				Config.Color,
				false,
				Config.Duration,
				3
			);
		}

		// 绘制胶囊体队列
		while (!DebugCapsuleQueue.IsEmpty())
		{
			FDebugCapsuleConfig Config;
			DebugCapsuleQueue.Dequeue(Config);

			// 计算胶囊体半高（从中心到顶部/底部的距离）
			const float HalfHeight = FMath::Max(0.0f, Config.Height * 0.5f);

			DrawDebugCapsule(
				CurrentWorld,
				Config.Location,          // 胶囊体中心位置
				HalfHeight,               // 半高（从中心到端点的距离）
				Config.Radius,            // 半径
				Config.Rotation.Quaternion(), // 转换为四元数
				Config.Color,             // 配置的颜色
				false,                    // 非持久
				Config.Duration,          // 生命周期0=只持续1帧
				0,
				Config.LineThickness      // 线宽（胶囊体通常需要较细的线）
			);
		}

		// 绘制线队列
		while (!DebugLineQueue.IsEmpty())
		{
			FDebugLineConfig Config;
			DebugLineQueue.Dequeue(Config);

			DrawDebugLine(
				CurrentWorld,
				Config.StartLocation,
				Config.EndLocation,
				Config.Color,
				false,
				Config.Duration,
				3,
				Config.LineThickness
			);
		}

		// 绘制球队列
		while (!DebugSphereQueue.IsEmpty())
		{
			FDebugSphereConfig Config;
			DebugSphereQueue.Dequeue(Config);

			DrawDebugSphere(
				CurrentWorld,
				Config.Location,
				Config.Radius,
				12,
				Config.Color,
				false,
				Config.Duration,
				0,
				Config.LineThickness
			);
		}

		// 绘制扇形队列
		while (!DebugSectorQueue.IsEmpty())
		{
			FDebugSectorConfig Config;
			DebugSectorQueue.Dequeue(Config);

			DrawDebugSector(
				CurrentWorld,
				Config.Location,
				Config.Direction,
				Config.Radius,
				Config.Angle, // 扇形角度
				Config.Height,
				Config.Color, // 橙色扇形
				false, // 非持久
				Config.Duration, // 显示0.1秒
				Config.DepthPriority, // 深度优先级
				Config.LineThickness // 线宽
			);

			//UE_LOG(LogTemp, Log, TEXT("Dequeue"));
		}

		// 绘制圆队列
		while (!DebugCircleQueue.IsEmpty())
		{
			FDebugCircleConfig Config;
			DebugCircleQueue.Dequeue(Config);

			// 绘制圆形（XY平面）
			DrawDebugCircle(
				CurrentWorld,
				Config.Location,  // 圆心位置
				Config.Radius,    // 圆半径
				36,               // 分段数（足够平滑）
				Config.Color,     // 配置的颜色
				false,            // 非持久
				Config.Duration,  // 生命周期0=只持续1帧
				3,                // 深度优先级
				Config.LineThickness,// 线宽
				FVector(1, 0, 0), // X轴
				FVector(0, 1, 0), // Y轴
				false             // 不绘制坐标轴
			);
		}
	}
	#pragma endregion

	//---------------------- 渲染 | Rendering -----------------------

	// 动画状态机 | Anim State Machine
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("Agent Anim State Machine");

		auto Chain = Mechanism->EnchainSolid(AgentStateMachineFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FAnimation& Animation,
				FAnimating& Animating,
				FAppear& Appear,
				FAttack& Attack,
				FDefence& Defence,
				FHit& Hit,
				FDeath& Death,
				FMove& Move,
				FFall& Fall,
				FMoving& Moving,
				FSlowing& Slowing)
			{
				bool bIsDyingAnim = Subject.HasFlag(DeathAnimFlag);
				bool bIsAppearAnim = Subject.HasFlag(AppearAnimFlag);
			    bool bIsHitAnim = Subject.HasFlag(HitAnimFlag);
				bool bIsAttackAnim = Subject.HasFlag(AttackAnimFlag);
				bool bIsFallAnim = Subject.HasFlag(FallAnimFlag);

				if (bIsDyingAnim && !Subject.HasTrait<FDying>())
				{
					Subject.SetFlag(DeathAnimFlag, false);
					bIsDyingAnim = false;
				}

				if (bIsAppearAnim && !Subject.HasTrait<FAppearing>())
				{
					Subject.SetFlag(AppearAnimFlag, false);
					bIsAppearAnim = false;
				}

				if (bIsAttackAnim && !Subject.HasTrait<FAttacking>())
				{
					Subject.SetFlag(AttackAnimFlag, false);
					bIsAttackAnim = false;
				}

				if (bIsHitAnim)
				{
					if (!Subject.HasTrait<FBeingHit>())
					{
						Subject.SetFlag(HitAnimFlag, false);
					}
					else
					{
						bIsHitAnim = Subject.HasTrait<FAttacking>() ? Subject.GetTrait<FAttacking>().State != EAttackState::PreCast : bIsHitAnim; // 前摇动画是不能打断的，但是后摇可以取消

						if (bIsHitAnim)
						{
							Subject.SetFlag(AttackAnimFlag, false); // hit anim will interrupt attack anim
						}
						else
						{
							Subject.SetFlag(HitAnimFlag, false);
						}
					}
					bIsHitAnim = false;
				}

				const bool bIsMoveAnim = !bIsAppearAnim && !bIsAttackAnim && !bIsHitAnim && !bIsDyingAnim && !bIsFallAnim;
				
				// Switch anim based on priority
				if (bIsDyingAnim )
				{
					if (Animating.AnimState != EAnimState::Dying)
					{
						Animating.AnimState = EAnimState::Dying;
						Animating.bUpdateAnimState = true;
					}
				}
				else if (bIsAppearAnim)
				{
					if (Animating.AnimState != EAnimState::Appearing)
					{
						Animating.AnimState = EAnimState::Appearing;
						Animating.bUpdateAnimState = true;
					}
				}
				else if (bIsHitAnim)
				{
					if (Animating.AnimState != EAnimState::BeingHit)
					{
						Animating.AnimState = EAnimState::BeingHit;
						Animating.bUpdateAnimState = true;
					}
				}
				else if (bIsAttackAnim)
				{
					if (Animating.AnimState != EAnimState::Attacking)
					{
						Animating.AnimState = EAnimState::Attacking;
						Animating.bUpdateAnimState = true;
					}
				}
				else if (bIsFallAnim)
				{
					if (Animating.AnimState != EAnimState::Falling)
					{
						Animating.AnimState = EAnimState::Falling;
						Animating.bUpdateAnimState = true;
					}
				}
				else if (bIsMoveAnim)
				{
					if (Animating.AnimState != EAnimState::BS_IdleMove)
					{
						Animating.AnimState = EAnimState::BS_IdleMove;
						Animating.bUpdateAnimState = true;
					}
				}

				// 动画状态机 | Anim State Machine
				switch (Animating.AnimState) // i sampled vat 3 times in shader. the following code use them to simulate an idle-move-montage state machine with anim blending
				{
					case EAnimState::BS_IdleMove:
					{
						if (Animating.bUpdateAnimState)
						{
							if (Animating.CurrentMontageSlot == 1)
							{
								CopyPasteAnimData(Animating, 1, 2);// copy anim from slot 1 to slot 2
								Animating.CurrentMontageSlot = 2;
							}

							// write idle anim to slot 0
							Animating.AnimCurrentTime0 = GetGameTimeSinceCreation();
							Animating.AnimPauseFrame0 = 0;
							Animating.AnimOffsetTime0 = FMath::RandRange(Animation.IdleRandomTimeOffset.X, Animation.IdleRandomTimeOffset.Y);

							// write move anim to slot 1
							Animating.AnimCurrentTime1 = GetGameTimeSinceCreation();
							Animating.AnimPauseFrame1 = 0;
							Animating.AnimOffsetTime1 = FMath::RandRange(Animation.MoveRandomTimeOffset.X, Animation.MoveRandomTimeOffset.Y);

							// reset AnimLerp1
							Animating.AnimLerp1 = 1;

							Animating.PreviousAnimState = Animating.AnimState;
							Animating.bUpdateAnimState = false;
						}

						// write blendspace ratio into AnimLerp0
						const TRange<float> InputRange(Animation.BS_IdleMove[0], Animation.BS_IdleMove[1]);
						const TRange<float> OutputRange(0, 1);
						float Input = Moving.CurrentVelocity.Size2D();
						float TargetLerp = FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Input);

						Animating.AnimLerp0 = FMath::FInterpConstantTo(Animating.AnimLerp0, TargetLerp, SafeDeltaTime, Animation.LerpSpeed);
						Animating.AnimLerp1 = FMath::Clamp(Animating.AnimLerp1 - SafeDeltaTime * Animation.LerpSpeed, 0, 1);

						Animating.AnimPlayRate0 = Animation.IdlePlayRate;
						Animating.AnimPlayRate1 = Animation.MovePlayRate;

						Animating.AnimIndex0 = Animation.IndexOfIdleAnim;
						Animating.AnimIndex1 = Animation.IndexOfMoveAnim;

						break;
					}

					case EAnimState::Appearing:
					{
						if (Animating.bUpdateAnimState)
						{
							Animating.CurrentMontageSlot = 2;

							// write appear anim into slot 2
							Animating.AnimCurrentTime2 = GetGameTimeSinceCreation();
							Animating.AnimIndex2 = Animation.IndexOfAppearAnim;
							Animating.AnimPauseFrame2 = Animating.AnimPauseFrameArray.Num() > Animation.IndexOfAppearAnim ? Animating.AnimPauseFrameArray[Animation.IndexOfAppearAnim] : 0;
							Animating.AnimPlayRate2 = Animating.AnimPauseFrame2 / Animating.SampleRate / Appear.Duration;
							Animating.AnimLerp1 = 1;

							Animating.PreviousAnimState = Animating.AnimState;
							Animating.bUpdateAnimState = false;
						}

						break;
					}

					case EAnimState::BeingHit:
					{
						PlayAnimAsMontage(Animation, Animating, Moving, false, true, Hit.AnimLength, 1, Animation.IndexOfHitAnim, 10, SafeDeltaTime);

						break;
					}

					case EAnimState::Attacking:
					{
						PlayAnimAsMontage(Animation, Animating, Moving, false, true, Attack.DurationPerRound, 1, Animation.IndexOfAttackAnim, 1, SafeDeltaTime);

						break;
					}

					case EAnimState::Falling:
					{
						PlayAnimAsMontage(Animation, Animating, Moving, true, false, 1, Animation.FallPlayRate, Animation.IndexOfFallAnim, 1, SafeDeltaTime);

						break;
					}

					case EAnimState::Dying:
					{
						PlayAnimAsMontage(Animation, Animating, Moving, false, true, Death.AnimLength, 1, Animation.IndexOfDeathAnim, 1, SafeDeltaTime);

						break;
					}
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 池初始化 | Init Pooling Info
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("ClearValidTransforms");

		auto Chain = Mechanism->EnchainSolid(RenderBatchFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FAgentRenderBatchData& Data)
			{
				Data.ValidTransforms.Reset();

				Data.Text_Location_Array.Reset();
				Data.Text_Value_Style_Scale_Offset_Array.Reset();

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 收集渲染数据 | Gather Render Data
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("AgentRender");

		auto Chain = Mechanism->EnchainSolid(AgentRenderFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FRendering& Rendering,
				FLocated& Located,
				FDirected& Directed,
				FScaled& Scaled,
				FCollider& Collider,
				FAnimation& Animation,
				FAnimating& Animating,
				FHealth& Health,
				FHealthBar& HealthBar,
				FPoppingText& PoppingText)
			{
				// Interp MatFx data
				Animating.IceFxInterped = FMath::FInterpTo(Animating.IceFxInterped, Animating.IceFx, SafeDeltaTime, 5);
				Animating.FireFxInterped = FMath::FInterpTo(Animating.FireFxInterped, Animating.FireFx, SafeDeltaTime, 5);
				Animating.PoisonFxInterped = FMath::FInterpTo(Animating.PoisonFxInterped, Animating.PoisonFx, SafeDeltaTime, 5);

				FAgentRenderBatchData& Data = Rendering.Renderer.GetTraitRef<FAgentRenderBatchData, EParadigm::Unsafe>();

				FQuat Rotation{ FQuat::Identity };
				Rotation = Directed.Direction.Rotation().Quaternion();

				FVector FinalScale(Data.Scale);
				FinalScale = FinalScale * Scaled.RenderScale * Scaled.JiggleMultiplier;

				float FinalRadius = Collider.Radius * Scaled.Scale;

				// 在计算转换时减去Radius
				FTransform SubjectTransform(Rotation * Data.OffsetRotation.Quaternion(), Located.Location + Data.OffsetLocation - FVector(0, 0, FinalRadius), FinalScale); // 减去Z轴上的Radius			

				int32 InstanceId = Rendering.InstanceId;

				Data.Lock();

				Data.ValidTransforms[InstanceId] = true;
				Data.Transforms[InstanceId] = SubjectTransform;

				// Transforms
				Data.LocationArray[InstanceId] = SubjectTransform.GetLocation();
				Data.OrientationArray[InstanceId] = SubjectTransform.GetRotation();
				Data.ScaleArray[InstanceId] = SubjectTransform.GetScale3D();

				// Dynamic params 0, encode multiple values into a single float
				float Elem0 = EncodeAnimationIndices(Animating.AnimIndex0, Animating.AnimIndex1, Animating.AnimIndex2);
				float Elem1 = EncodePauseFrames(Animating.AnimPauseFrame0, Animating.AnimPauseFrame1, Animating.AnimPauseFrame2);
				float Elem2 = EncodePlayRates(Animating.AnimPlayRate0, Animating.AnimPlayRate1, Animating.AnimPlayRate2);
				float Elem3 = EncodeStatusEffects(Animating.HitGlow, Animating.IceFxInterped, Animating.FireFxInterped, Animating.PoisonFxInterped);

				Data.AnimIndex_PauseFrame_Playrate_MatFx_Array[InstanceId] = FVector4(Elem0, Elem1, Elem2, Elem3);

				// Dynamic params 1
				Data.AnimTimeStamp_Array[InstanceId] = FVector4(Animating.AnimCurrentTime0 - Animating.AnimOffsetTime0, Animating.AnimCurrentTime1 - Animating.AnimOffsetTime1, Animating.AnimCurrentTime2 - Animating.AnimOffsetTime2, 0);

				// Pariticle color
				Data.AnimLerp0_AnimLerp1_Team_Dissolve_Array[InstanceId] = FVector4(Animating.AnimLerp0, Animating.AnimLerp1, Animating.Team, Animating.Dissolve);

				// HealthBar
				Data.HealthBar_Opacity_CurrentRatio_TargetRatio_Array[InstanceId] = FVector(HealthBar.Opacity, HealthBar.CurrentRatio, HealthBar.TargetRatio);

				// PopText
				Data.Text_Location_Array.Append(PoppingText.TextLocationArray);

				float MaxFinalScale = FMath::Max3(FinalScale.X, FinalScale.Y, FinalScale.Z);

				for (const auto& Text_Value_Style_Scale_Offset : PoppingText.Text_Value_Style_Scale_Offset_Array)
				{
					Data.Text_Value_Style_Scale_Offset_Array.Add(Text_Value_Style_Scale_Offset * FVector4(1,1,1, MaxFinalScale));
				}

				Data.Unlock();

				PoppingText.TextLocationArray.Empty();
				PoppingText.Text_Value_Style_Scale_Offset_Array.Empty();

				Subject.SetFlag(HitPoppingTextFlag, false);

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 池写入 | Write Pooling Info
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("WritePoolingInfo");

		auto Chain = Mechanism->EnchainSolid(RenderBatchFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently(
			[&](FSolidSubjectHandle Subject,
				FAgentRenderBatchData& Data)
			{
				// 重置和隐藏限制数组成员
				Data.FreeTransforms.Reset();

				for (int32 i = Data.ValidTransforms.IndexOf(false); i < Data.Transforms.Num(); i = Data.ValidTransforms.IndexOf(false, i + 1))
				{
					Data.FreeTransforms.Add(i);
					Data.InsidePool_Array[i] = true;
				}

			}, ThreadsCount, BatchSize);
	}
	#pragma endregion

	// 发送至Niagara | Send Data to Niagara
	#pragma region
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("SendDataToNiagara");

		Mechanism->Operate<FUnsafeChain>(RenderBatchFilter,
			[&](FSubjectHandle Subject,
				FAgentRenderBatchData& Data)
			{
				// ------------------Transform---------------------------------

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
					Data.SpawnedNiagaraSystem,
					FName("LocationArray"),
					Data.LocationArray
				);

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayQuat(
					Data.SpawnedNiagaraSystem,
					FName("OrientationArray"),
					Data.OrientationArray
				);

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
					Data.SpawnedNiagaraSystem,
					FName("ScaleArray"),
					Data.ScaleArray
				);

				// ---------------------VAT------------------------------

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(
					Data.SpawnedNiagaraSystem,
					FName("AnimIndex_PauseFrame_Playrate_MatFx_Array"),
					Data.AnimIndex_PauseFrame_Playrate_MatFx_Array
				);

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(
					Data.SpawnedNiagaraSystem,
					FName("AnimTimeStamp_Array"),
					Data.AnimTimeStamp_Array
				);

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(
					Data.SpawnedNiagaraSystem,
					FName("AnimLerp0_AnimLerp1_Team_Dissolve_Array"),
					Data.AnimLerp0_AnimLerp1_Team_Dissolve_Array
				);


				// ------------------HealthBar---------------------------------

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
					Data.SpawnedNiagaraSystem,
					FName("HealthBar_Opacity_CurrentRatio_TargetRatio_Array"),
					Data.HealthBar_Opacity_CurrentRatio_TargetRatio_Array
				);

				// ------------------Pop Text----------------------------------

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
					Data.SpawnedNiagaraSystem,
					FName("Text_Location_Array"),
					Data.Text_Location_Array
				);

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(
					Data.SpawnedNiagaraSystem,
					FName("Text_Value_Style_Scale_Offset_Array"),
					Data.Text_Value_Style_Scale_Offset_Array
				);

				// ------------------Others------------------------------------

				UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayBool(
					Data.SpawnedNiagaraSystem,
					FName("InsidePool_Array"),
					Data.InsidePool_Array
				);
			});
	}
	#pragma endregion

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------Helpers--------------------------------------------------------

void ABattleFrameBattleControl::DefineFilters()
{
	// this is a bit inconvenient but good for performance
	bIsFilterReady = true;

	AgentStatFilter = FFilter::Make<FStatistics>();
	AgentAppeaFilter = FFilter::Make<FAgent, FRendering, FLocated, FDirected, FScaled, FAppear, FAppearing, FAnimation, FActivated>();

	AgentSleepFilter = FFilter::Make<FAgent, FLocated, FDirected, FScaled, FCollider, FSleep, FSleeping, FTrace, FTracing, FMove, FMoving, FRendering, FActivated>().Exclude<FAppearing, FDying>();
	AgentPatrolFilter = FFilter::Make<FAgent, FLocated, FDirected, FScaled, FCollider, FPatrol, FPatrolling, FTrace, FTracing, FMove, FMoving, FRendering, FActivated>().Exclude<FAppearing, FSleeping, FDying>();
	AgentMoveFilter = FFilter::Make<FAgent, FRendering, FAnimation, FMove, FMoving, FChase, FLocated, FDirected, FScaled, FCollider, FAttack, FTrace, FTracing, FNavigation, FNavigating, FAvoidance, FAvoiding, FDefence, FPatrol, FGridData, FSlowing, FActivated>();
	SubjectFilterBase = FFilter::Make<FLocated, FDirected, FScaled, FCollider, FAvoidance, FAvoiding, FGridData, FActivated>().Exclude<FSphereObstacle, FBoxObstacle>();

	AgentTraceFilter = FFilter::Make<FAgent, FLocated, FDirected, FScaled, FCollider, FSleep, FPatrol, FTrace, FTracing, FMoving, FRendering, FActivated>().Exclude<FAppearing, FDying>();
	AgentAttackFilter = FFilter::Make<FAgent, FAttack, FRendering, FLocated, FDirected, FCollider, FScaled, FTrace, FActivated>().Exclude<FAppearing, FSleeping, FPatrolling, FDying>();
	AgentAttackingFilter = FFilter::Make<FAgent, FAttack, FRendering, FLocated, FDirected, FScaled, FAnimation, FAttacking, FMove, FMoving, FTrace, FTracing, FDebuff, FDamage, FDefence, FSlowing, FActivated>().Exclude<FAppearing, FSleeping, FPatrolling, FDying>();

	SubjectBeingHitFilter = FFilter::Make<FLocated, FScaled, FHealth, FBeingHit, FActivated>();

	AgentDeathFilter = FFilter::Make<FAgent, FRendering, FDeath, FLocated, FDirected, FScaled, FDying, FTrace, FTracing, FMove, FMoving, FAnimating, FCurves, FActivated>();
	
	AgentStateMachineFilter = FFilter::Make<FAgent, FAnimation, FRendering, FAppear, FAttack, FDeath, FMoving, FSlowing, FActivated>();
	AgentRenderFilter = FFilter::Make<FAgent, FRendering, FLocated, FDirected, FScaled, FCollider, FAnimation, FHealth, FHealthBar, FPoppingText, FActivated>();

	TemporalDamageFilter = FFilter::Make<FTemporalDamage>();
	SlowFilter = FFilter::Make<FSlow>();

	SpawnProjectileFilter = FFilter::Make<FProjectileConfig_Final>();
	SpawnActorFilter = FFilter::Make<FActorSpawnConfig_Final>();
	SpawnFxFilter = FFilter::Make<FFxConfig_Final>();
	PlaySoundFilter = FFilter::Make<FSoundConfig_Final>();

	RenderBatchFilter = FFilter::Make<FAgentRenderBatchData>();
	SpeedLimitOverrideFilter = FFilter::Make<FCollider, FLocated, FSphereObstacle>();

	ProjectileFilter = FFilter::Make<FProjectile, FProjectileParams, FProjectileParamsRT, FLocated, FDirected, FScaled, FActivated>();
}

// 这里缺一个GetRandomPointInNavigableRadius实现
FVector ABattleFrameBattleControl::FindNewPatrolGoalLocation(const FPatrol Patrol, const FCollider Collider, const FTrace Trace, const FTracing Tracing, const FLocated Located, const FScaled Scaled, int32 MaxAttempts)
{
	// Early out if no neighbor grid available
	if (!IsValid(Tracing.NeighborGrid))
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Distance = FMath::FRandRange(Patrol.PatrolRadiusMin, Patrol.PatrolRadiusMax);
		return Patrol.Origin + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);
	}

	FVector BestCandidate = Patrol.Origin;
	float BestDistanceSq = 0.f;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		// Generate random position in patrol ring
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Distance = FMath::FRandRange(Patrol.PatrolRadiusMin, Patrol.PatrolRadiusMax);
		const FVector Candidate = Patrol.Origin + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);

		// Track farthest candidate as fallback
		const float CurrentDistanceSq = (Candidate - Located.Location).SizeSquared();

		if (CurrentDistanceSq > BestDistanceSq)
		{
			BestCandidate = Candidate;
			BestDistanceSq = CurrentDistanceSq;
		}
	}

	// Return best candidate if all attempts hit obstacles
	return BestCandidate;
}

bool ABattleFrameBattleControl::GetInterpedWorldLocation(AFlowField* flowField, const FVector& location, const float angleThreshold, FVector& outInterpolatedWorldLoc)
{
	// 初始化输出为无效值
	outInterpolatedWorldLoc = FVector::ZeroVector;

	// 检查是否已开始游戏
	if (flowField->bIsBeginPlay) return false;

	// 计算相对位置
	FVector relativeLocation = (location - flowField->actorLoc).RotateAngleAxis(-flowField->actorRot.Yaw, FVector(0, 0, 1)) + flowField->offsetLoc;
	float cellRadius = flowField->cellSize / 2.0f;

	// 计算连续网格坐标
	float continuousGridX = (relativeLocation.X - cellRadius) / flowField->cellSize;
	float continuousGridY = (relativeLocation.Y - cellRadius) / flowField->cellSize;

	// 获取左下角索引和小数部分
	int32 baseX = FMath::FloorToInt(continuousGridX);
	int32 baseY = FMath::FloorToInt(continuousGridY);
	float fractionX = continuousGridX - baseX;
	float fractionY = continuousGridY - baseY;

	// 检查四个点是否都在网格内
	if (baseX >= 0 && (baseX + 1) < flowField->xNum &&
		baseY >= 0 && (baseY + 1) < flowField->yNum)
	{
		// 获取四个角点的单元格
		bool isValid00, isValid10, isValid01, isValid11;
		FCellStruct& cell00 = flowField->GetCellAtCoord(FVector2D(baseX, baseY), isValid00);
		FCellStruct& cell10 = flowField->GetCellAtCoord(FVector2D(baseX + 1, baseY), isValid10);
		FCellStruct& cell01 = flowField->GetCellAtCoord(FVector2D(baseX, baseY + 1), isValid01);
		FCellStruct& cell11 = flowField->GetCellAtCoord(FVector2D(baseX + 1, baseY + 1), isValid11);

		// 确保所有单元格都有效
		if (isValid00 && isValid10 && isValid01 && isValid11)
		{
			// 双线性插值位置
			FVector interpBottom = FMath::Lerp(cell00.worldLoc, cell10.worldLoc, fractionX);
			FVector interpTop = FMath::Lerp(cell01.worldLoc, cell11.worldLoc, fractionX);
			outInterpolatedWorldLoc = FMath::Lerp(interpBottom, interpTop, fractionY);

			// 计算与四个角点的最大坡度
			float maxSlopeAngle = 0.0f;
			const TArray<FVector> cornerPoints = {
				cell00.worldLoc,
				cell10.worldLoc,
				cell01.worldLoc,
				cell11.worldLoc
			};

			for (const FVector& cornerPoint : cornerPoints)
			{
				// 计算水平距离（忽略Z轴）
				FVector horizontalVec = cornerPoint - outInterpolatedWorldLoc;
				horizontalVec.Z = 0.0f;
				const float horizontalDistance = horizontalVec.Size();

				// 跳过距离过小的点（避免除以0）
				if (horizontalDistance < 0) continue;

				// 计算高度差
				const float heightDiff = FMath::Abs(cornerPoint.Z - outInterpolatedWorldLoc.Z);

				// 计算坡度角度（atan(高度差/水平距离)）
				const float slopeAngle = FMath::RadiansToDegrees(FMath::Atan(heightDiff / horizontalDistance));

				// 更新最大坡度
				if (slopeAngle > maxSlopeAngle)
				{
					maxSlopeAngle = slopeAngle;
				}
			}

			//UE_LOG(LogTemp, Warning, TEXT("maxSlopeAngle = %.2f degrees"), maxSlopeAngle);

			// 检查坡度是否超过阈值
			if (maxSlopeAngle > angleThreshold)
			{
				return false; // 坡度太陡，无效位置
			}

			return true; // 有效位置且坡度可接受
		}
	}

	return false; // 基础条件不满足
}

void ABattleFrameBattleControl::DrawDebugSector(UWorld* World, const FVector& Center, const FVector& Direction, float Radius, float AngleDegrees, float Height, const FColor& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	if (!World || AngleDegrees <= 0.f || Radius <= 0.f || Height <= 0.f) return;

	// 确保角度在合理范围
	AngleDegrees = FMath::Clamp(AngleDegrees, 1.f, 360.f);
	const float HalfAngle = FMath::DegreesToRadians(AngleDegrees) * 0.5f;

	// 计算方向向量
	FVector Forward = Direction.GetSafeNormal2D();
	if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;

	// 计算顶面和底面中心位置（以Center为中心对称）
	const float HalfHeight = Height * 0.5f;
	const FVector TopCenter = Center + FVector(0, 0, HalfHeight);
	const FVector BottomCenter = Center - FVector(0, 0, HalfHeight);

	// 计算起始和结束方向
	const FQuat StartQuat = FQuat(FVector::UpVector, -HalfAngle);
	const FQuat EndQuat = FQuat(FVector::UpVector, HalfAngle);

	FVector StartDir = StartQuat.RotateVector(Forward);
	FVector EndDir = EndQuat.RotateVector(Forward);

	// 计算关键点
	const FVector BottomStart = BottomCenter + StartDir * Radius;
	const FVector BottomEnd = BottomCenter + EndDir * Radius;
	const FVector TopStart = TopCenter + StartDir * Radius;
	const FVector TopEnd = TopCenter + EndDir * Radius;

	// ========== 绘制底面扇形 ==========
	// 绘制底面弧线
	const int32 NumSegments = FMath::CeilToInt(AngleDegrees / 5.f) + 1;
	const float AngleStep = 2 * HalfAngle / (NumSegments - 1);

	FVector LastBottomPoint = BottomStart;

	for (int32 i = 1; i < NumSegments; ++i)
	{
		const float CurrentAngle = -HalfAngle + AngleStep * i;
		const FQuat RotQuat = FQuat(FVector::UpVector, CurrentAngle);
		FVector CurrentDir = RotQuat.RotateVector(Forward);
		FVector CurrentPoint = BottomCenter + CurrentDir * Radius;

		DrawDebugLine(
			World,
			LastBottomPoint,
			CurrentPoint,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		LastBottomPoint = CurrentPoint;
	}

	// ========== 绘制顶面扇形 ==========
	FVector LastTopPoint = TopStart;

	// 绘制顶面弧线
	for (int32 i = 1; i < NumSegments; ++i)
	{
		const float CurrentAngle = -HalfAngle + AngleStep * i;
		const FQuat RotQuat = FQuat(FVector::UpVector, CurrentAngle);
		FVector CurrentDir = RotQuat.RotateVector(Forward);
		FVector CurrentPoint = TopCenter + CurrentDir * Radius;

		DrawDebugLine(
			World,
			LastTopPoint,
			CurrentPoint,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		LastTopPoint = CurrentPoint;
	}

	// ========== 连接三个关键位置 ==========

	if (AngleDegrees != 360)
	{
		// 连接两个圆心（底面圆心 <-> 顶面圆心）
		//DrawDebugLine(
		//	World,
		//	BottomCenter,
		//	TopCenter,
		//	Color,
		//	bPersistentLines,
		//	LifeTime,
		//	DepthPriority,
		//	Thickness
		//);

		// 绘制顶面两条半径线
		DrawDebugLine(
			World,
			TopCenter,
			TopStart,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		DrawDebugLine(
			World,
			TopCenter,
			TopEnd,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		// 绘制底面两条半径线
		DrawDebugLine(
			World,
			BottomCenter,
			BottomStart,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		DrawDebugLine(
			World,
			BottomCenter,
			BottomEnd,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		// 连接起点（底面起点 <-> 顶面起点）
		DrawDebugLine(
			World,
			BottomStart,
			TopStart,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);

		// 连接终点（底面终点 <-> 顶面终点）
		DrawDebugLine(
			World,
			BottomEnd,
			TopEnd,
			Color,
			bPersistentLines,
			LifeTime,
			DepthPriority,
			Thickness
		);
	}
}

void ABattleFrameBattleControl::CopyPasteAnimData(FAnimating& Animating, int32 From, int32 To)
{
	// 确保From和To在有效范围内 (0-2)
	if (From < 0 || From > 2 || To < 0 || To > 2) return;
	if (From == To) return; // 相同索引无需拷贝

	// 定义动画参数结构体简化拷贝逻辑
	struct FAnimParams
	{
		float* Index = nullptr;
		float* PlayRate = nullptr;
		float* CurrentTime = nullptr;
		float* OffsetTime = nullptr;
		float* PauseTime = nullptr;
	};

	// 初始化三组动画参数
	FAnimParams Source;
	FAnimParams Dest;

	// 根据From索引设置源参数指针
	switch (From)
	{
	case 0:
		Source = { &Animating.AnimIndex0, &Animating.AnimPlayRate0, &Animating.AnimCurrentTime0, &Animating.AnimOffsetTime0, &Animating.AnimPauseFrame0 };
		break;
	case 1:
		Source = { &Animating.AnimIndex1, &Animating.AnimPlayRate1, &Animating.AnimCurrentTime1, &Animating.AnimOffsetTime1, &Animating.AnimPauseFrame1 };
		break;
	case 2:
		Source = { &Animating.AnimIndex2, &Animating.AnimPlayRate2, &Animating.AnimCurrentTime2, &Animating.AnimOffsetTime2, &Animating.AnimPauseFrame2 };
		break;
	default: // 添加default分支避免警告
		return; // 实际上不会执行到这里
	}

	// 根据To索引设置目标参数指针
	switch (To)
	{
	case 0:
		Dest = { &Animating.AnimIndex0, &Animating.AnimPlayRate0, &Animating.AnimCurrentTime0, &Animating.AnimOffsetTime0, &Animating.AnimPauseFrame0 };
		break;
	case 1:
		Dest = { &Animating.AnimIndex1, &Animating.AnimPlayRate1, &Animating.AnimCurrentTime1, &Animating.AnimOffsetTime1, &Animating.AnimPauseFrame1 };
		break;
	case 2:
		Dest = { &Animating.AnimIndex2, &Animating.AnimPlayRate2, &Animating.AnimCurrentTime2, &Animating.AnimOffsetTime2, &Animating.AnimPauseFrame2 };
		break;
	default: // 添加default分支避免警告
		return; // 实际上不会执行到这里
	}

	// 执行参数拷贝
	*Dest.Index = *Source.Index;
	*Dest.PlayRate = *Source.PlayRate;
	*Dest.CurrentTime = *Source.CurrentTime;
	*Dest.OffsetTime = *Source.OffsetTime;
	*Dest.PauseTime = *Source.PauseTime;
}


//-------------------------------------------------------Damager--------------------------------------------------------

void ABattleFrameBattleControl::ApplyPointDamageAndDebuff(const FSubjectArray& Subjects, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Point& Damage, const FDebuff_Point& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyPointDamageAndDebuff");
	// 使用TSet存储唯一敌人句柄
	TSet<FSubjectHandle> UniqueHandles;

	// 将IgnoreSubjects转换为TSet以提高查找效率
	const TSet<FSubjectHandle> IgnoreSet(IgnoreSubjects.Subjects);

	for (const auto& Overlapper : Subjects.Subjects)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		// 使用TSet的Contains替代数组的Contains
		if (IgnoreSet.Contains(Overlapper)) continue;

		int32 PreviousNum = UniqueHandles.Num();
		UniqueHandles.Add(Overlapper);

		if (UniqueHandles.Num() == PreviousNum) continue;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		// 击退
		FVector HitDirection = FVector::OneVector;

		if (bHasLocated) HitDirection = (Location - HitFromLocation).GetSafeNormal2D();

		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth, EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
				case EDmgType::Normal:
					BaseDamage = Damage.Damage * NormalDmgMult;
					break;
				case EDmgType::Fire:
					BaseDamage = Damage.Damage * FireDmgMult;
					break;
				case EDmgType::Ice:
					BaseDamage = Damage.Damage * IceDmgMult;
					break;
				case EDmgType::Poison:
					BaseDamage = Damage.Damage * PoisonDmgMult;
					break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = BaseDamage + PercentageDamage;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------

			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
					case EDmgType::Normal:
						TotalTemporalDmg *= NormalDmgMult;
						break;
					case EDmgType::Fire:
						TotalTemporalDmg *= FireDmgMult;
						break;
					case EDmgType::Ice:
						TotalTemporalDmg *= IceDmgMult;
						break;
					case EDmgType::Poison:
						TotalTemporalDmg *= PoisonDmgMult;
						break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubject(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto Moving = Overlapper.GetTrait<FMoving>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);
				Moving.LaunchVelSum += KnockbackForce; // 累加击退力

				Overlapper.SetTrait(Moving);
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;

			Mechanism->SpawnSubject(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto Sleep = Overlapper.GetTrait<FSleep>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.SetTrait(Sleep);
					Overlapper.RemoveTrait<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(),WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTrait(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}

void ABattleFrameBattleControl::ApplyPointDamageAndDebuffDeferred(const FSubjectArray& Subjects, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Point& Damage, const FDebuff_Point& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyPointDamageAndDebuff");
	// 使用TSet存储唯一敌人句柄
	TSet<FSubjectHandle> UniqueHandles;

	// 将IgnoreSubjects转换为TSet以提高查找效率
	const TSet<FSubjectHandle> IgnoreSet(IgnoreSubjects.Subjects);

	for (const auto& Overlapper : Subjects.Subjects)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		// 使用TSet的Contains替代数组的Contains
		if (IgnoreSet.Contains(Overlapper)) continue;

		int32 PreviousNum = UniqueHandles.Num();
		UniqueHandles.Add(Overlapper);

		if (UniqueHandles.Num() == PreviousNum) continue;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FVector HitDirection = FVector::ZeroVector;

		if (bHasLocated)
		{
			HitDirection = (Location - HitFromLocation).GetSafeNormal2D();
		}

		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth, EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto& Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
				case EDmgType::Normal:
					BaseDamage = Damage.Damage * NormalDmgMult;
					break;
				case EDmgType::Fire:
					BaseDamage = Damage.Damage * FireDmgMult;
					break;
				case EDmgType::Ice:
					BaseDamage = Damage.Damage * IceDmgMult;
					break;
				case EDmgType::Poison:
					BaseDamage = Damage.Damage * PoisonDmgMult;
					break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = BaseDamage + PercentageDamage;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto& TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------

			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
					case EDmgType::Normal:
						TotalTemporalDmg *= NormalDmgMult;
						break;
					case EDmgType::Fire:
						TotalTemporalDmg *= FireDmgMult;
						break;
					case EDmgType::Ice:
						TotalTemporalDmg *= IceDmgMult;
						break;
					case EDmgType::Poison:
						TotalTemporalDmg *= PoisonDmgMult;
						break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubjectDeferred(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		// 击退
		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto& Moving = Overlapper.GetTraitRef<FMoving, EParadigm::Unsafe>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);

				Moving.Lock();
				Moving.LaunchVelSum += KnockbackForce; // 累加击退力
				Moving.Unlock();
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for deferred spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;

			Mechanism->SpawnSubjectDeferred(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto& Sleep = Overlapper.GetTraitRef<FSleep, EParadigm::Unsafe>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.RemoveTraitDeferred<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTraitDeferred(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}

void ABattleFrameBattleControl::ApplyRadialDamageAndDebuff(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& Origin, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Radial& Damage, const FDebuff_Radial& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyRadialDamageAndDebuff");
	// sphere trace at Origin
	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ANeighborGridActor> It(World); It; ++It)
			{
				ANeighborGridActor* NeighborGridActor = *It;
				NeighborGridComponent = NeighborGridActor->GetComponentByClass<UNeighborGridComponent>();
				break;
			}
		}

		if (!IsValid(NeighborGridComponent)) return;
	}

	bool bHit;
	TArray<FTraceResult> TraceResults;

	UBattleFrameFunctionLibraryRT::SphereTraceForSubjects
	(
		bHit,
		TraceResults,
		NeighborGridComponent,
		KeepCount,
		Origin,
		Damage.DmgRadius,
		!Damage.Filter.ObstacleObjectType.IsEmpty(),
		Origin,
		0.01,
		ESortMode::None,
		Origin,
		IgnoreSubjects,
		Damage.Filter,
		FTraceDrawDebugConfig()
	);

	if (!bHit) return;

	for (const auto& TraceResult : TraceResults)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		const auto& Overlapper = TraceResult.Subject;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		// 击退
		FVector HitDirection = FVector::OneVector;
		if (bHasLocated) HitDirection = (Location - HitFromLocation).GetSafeNormal2D();

		// 距离衰减
		float Distance = FVector::Distance(Origin, Location);
		TRange<float> InputRange(0,Damage.DmgRadius);
		TRange<float> OutputRange(1, 0);
		float DmgFalloffMult = Damage.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange,OutputRange, Distance) : 1;
		float DebuffFalloffMult = Debuff.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;

		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth, EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
				case EDmgType::Normal:
					BaseDamage = Damage.Damage * NormalDmgMult;
					break;
				case EDmgType::Fire:
					BaseDamage = Damage.Damage * FireDmgMult;
					break;
				case EDmgType::Ice:
					BaseDamage = Damage.Damage * IceDmgMult;
					break;
				case EDmgType::Poison:
					BaseDamage = Damage.Damage * PoisonDmgMult;
					break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = (BaseDamage + PercentageDamage) * DmgFalloffMult;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------
			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
				case EDmgType::Normal:
					TotalTemporalDmg *= NormalDmgMult;
					break;
				case EDmgType::Fire:
					TotalTemporalDmg *= FireDmgMult;
					break;
				case EDmgType::Ice:
					TotalTemporalDmg *= IceDmgMult;
					break;
				case EDmgType::Poison:
					TotalTemporalDmg *= PoisonDmgMult;
					break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg * DebuffFalloffMult;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubject(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto Moving = Overlapper.GetTrait<FMoving>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);
				Moving.LaunchVelSum += KnockbackForce * DebuffFalloffMult; // 累加击退力

				Overlapper.SetTrait(Moving);
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength * DebuffFalloffMult;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;
			Mechanism->SpawnSubject(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto Sleep = Overlapper.GetTrait<FSleep>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.SetTrait(Sleep);
					Overlapper.RemoveTrait<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTrait(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}

void ABattleFrameBattleControl::ApplyRadialDamageAndDebuffDeferred(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& Origin, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Radial& Damage, const FDebuff_Radial& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyRadialDamageAndDebuff");
	// sphere trace at Origin

	if (!IsValid(NeighborGridComponent)) return;

	bool bHit;
	TArray<FTraceResult> TraceResults;

	UBattleFrameFunctionLibraryRT::SphereTraceForSubjects
	(
		bHit,
		TraceResults,
		NeighborGridComponent,
		KeepCount,
		Origin,
		Damage.DmgRadius,
		!Damage.Filter.ObstacleObjectType.IsEmpty(),
		Origin,
		0.01,
		ESortMode::None,
		Origin,
		IgnoreSubjects,
		Damage.Filter,
		FTraceDrawDebugConfig()
	);

	if (!bHit) return;

	for (const auto& TraceResult : TraceResults)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		const auto& Overlapper = TraceResult.Subject;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		// 击退
		FVector HitDirection = FVector::OneVector;
		if (bHasLocated) HitDirection = (Location - HitFromLocation).GetSafeNormal2D();

		// 距离衰减
		float Distance = FVector::Distance(Origin, Location);
		TRange<float> InputRange(0, Damage.DmgRadius);
		TRange<float> OutputRange(1, 0);
		float DmgFalloffMult = Damage.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;
		float DebuffFalloffMult = Debuff.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;

		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth, EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
				case EDmgType::Normal:
					BaseDamage = Damage.Damage * NormalDmgMult;
					break;
				case EDmgType::Fire:
					BaseDamage = Damage.Damage * FireDmgMult;
					break;
				case EDmgType::Ice:
					BaseDamage = Damage.Damage * IceDmgMult;
					break;
				case EDmgType::Poison:
					BaseDamage = Damage.Damage * PoisonDmgMult;
					break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = (BaseDamage + PercentageDamage) * DmgFalloffMult;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------
			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
				case EDmgType::Normal:
					TotalTemporalDmg *= NormalDmgMult;
					break;
				case EDmgType::Fire:
					TotalTemporalDmg *= FireDmgMult;
					break;
				case EDmgType::Ice:
					TotalTemporalDmg *= IceDmgMult;
					break;
				case EDmgType::Poison:
					TotalTemporalDmg *= PoisonDmgMult;
					break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg * DebuffFalloffMult;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubjectDeferred(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto Moving = Overlapper.GetTrait<FMoving>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);
				Moving.LaunchVelSum += KnockbackForce * DebuffFalloffMult; // 累加击退力

				Overlapper.SetTraitDeferred(Moving);
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength * DebuffFalloffMult;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;

			Mechanism->SpawnSubjectDeferred(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto Sleep = Overlapper.GetTrait<FSleep>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.SetTraitDeferred(Sleep);
					Overlapper.RemoveTraitDeferred<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTraitDeferred(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}

void ABattleFrameBattleControl::ApplyBeamDamageAndDebuff(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& StartLocation, const FVector& EndLocation, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Beam& Damage, const FDebuff_Beam& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyRadialDamageAndDebuff");
	// sphere trace at Origin
	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ANeighborGridActor> It(World); It; ++It)
			{
				ANeighborGridActor* NeighborGridActor = *It;
				NeighborGridComponent = NeighborGridActor->GetComponentByClass<UNeighborGridComponent>();
				break;
			}
		}

		if (!IsValid(NeighborGridComponent)) return;
	}

	bool bHit;
	TArray<FTraceResult> TraceResults;

	UBattleFrameFunctionLibraryRT::SphereSweepForSubjects
	(
		bHit,
		TraceResults,
		NeighborGridComponent,
		KeepCount,
		StartLocation,
		EndLocation,
		Damage.DmgRadius,
		!Damage.Filter.ObstacleObjectType.IsEmpty(),
		StartLocation,
		0.01,
		ESortMode::None,
		StartLocation,
		IgnoreSubjects,
		Damage.Filter,
		FTraceDrawDebugConfig()
	);

	if (!bHit) return;

	for (const auto& TraceResult : TraceResults)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		const auto& Overlapper = TraceResult.Subject;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		// 击退
		FVector HitDirection = FVector::OneVector;
		if (bHasLocated) HitDirection = (Location - HitFromLocation).GetSafeNormal2D();

		// 距离衰减
		float Distance = FMath::PointDistToLine(StartLocation, EndLocation, Location);
		TRange<float> InputRange(0, Damage.DmgRadius);
		TRange<float> OutputRange(1, 0);
		float DmgFalloffMult = Damage.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;
		float DebuffFalloffMult = Debuff.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;

		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth,EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
				case EDmgType::Normal:
					BaseDamage = Damage.Damage * NormalDmgMult;
					break;
				case EDmgType::Fire:
					BaseDamage = Damage.Damage * FireDmgMult;
					break;
				case EDmgType::Ice:
					BaseDamage = Damage.Damage * IceDmgMult;
					break;
				case EDmgType::Poison:
					BaseDamage = Damage.Damage * PoisonDmgMult;
					break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = (BaseDamage + PercentageDamage) * DmgFalloffMult;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------
			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
				case EDmgType::Normal:
					TotalTemporalDmg *= NormalDmgMult;
					break;
				case EDmgType::Fire:
					TotalTemporalDmg *= FireDmgMult;
					break;
				case EDmgType::Ice:
					TotalTemporalDmg *= IceDmgMult;
					break;
				case EDmgType::Poison:
					TotalTemporalDmg *= PoisonDmgMult;
					break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg * DebuffFalloffMult;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubject(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto Moving = Overlapper.GetTrait<FMoving>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);
				Moving.LaunchVelSum += KnockbackForce * DebuffFalloffMult; // 累加击退力

				Overlapper.SetTrait(Moving);
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength * DebuffFalloffMult;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;
			Mechanism->SpawnSubject(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto Sleep = Overlapper.GetTrait<FSleep>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.SetTrait(Sleep);
					Overlapper.RemoveTrait<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubject(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTrait(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}

void ABattleFrameBattleControl::ApplyBeamDamageAndDebuffDeferred(UNeighborGridComponent* NeighborGridComponent, const int32 KeepCount, const FVector& StartLocation, const FVector& EndLocation, const FSubjectArray& IgnoreSubjects, const FSubjectHandle DmgInstigator, const FSubjectHandle DmgCauser, const FVector& HitFromLocation, const FDamage_Beam& Damage, const FDebuff_Beam& Debuff, TArray<FDmgResult>& DamageResults)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ApplyRadialDamageAndDebuff");
	// sphere trace at Origin

	if (!IsValid(NeighborGridComponent)) return;

	bool bHit;
	TArray<FTraceResult> TraceResults;

	UBattleFrameFunctionLibraryRT::SphereSweepForSubjects
	(
		bHit,
		TraceResults,
		NeighborGridComponent,
		KeepCount,
		StartLocation,
		EndLocation,
		Damage.DmgRadius,
		!Damage.Filter.ObstacleObjectType.IsEmpty(),
		StartLocation,
		0.01,
		ESortMode::None,
		StartLocation,
		IgnoreSubjects,
		Damage.Filter,
		FTraceDrawDebugConfig()
	);

	if (!bHit) return;

	for (const auto& TraceResult : TraceResults)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE_STR("ForEachOverlapper");
		const auto& Overlapper = TraceResult.Subject;

		if (!Overlapper.IsValid()) continue;

		// Pre-calculate all trait checks
		const bool bHasHealth = Overlapper.HasTrait<FHealth>();
		const bool bHasLocated = Overlapper.HasTrait<FLocated>();
		const bool bHasDirected = Overlapper.HasTrait<FDirected>();
		const bool bHasScaled = Overlapper.HasTrait<FScaled>();
		const bool bHasCollider = Overlapper.HasTrait<FCollider>();
		const bool bHasGridData = Overlapper.HasTrait<FGridData>();
		const bool bHasDefence = Overlapper.HasTrait<FDefence>();
		const bool bHasTextPopUp = Overlapper.HasTrait<FTextPopUp>();
		const bool bHasMoving = Overlapper.HasTrait<FMoving>();
		const bool bHasSlowing = Overlapper.HasTrait<FSlowing>();
		const bool bHasAnimation = Overlapper.HasTrait<FAnimation>();
		const bool bHasSleep = Overlapper.HasTrait<FSleep>();
		const bool bHasSleeping = Overlapper.HasTrait<FSleeping>();
		const bool bHasHit = Overlapper.HasTrait<FHit>();
		const bool bHasBeingHit = Overlapper.HasTrait<FBeingHit>();
		const bool bHasHitGlow = Overlapper.HasFlag(HitGlowFlag);
		const bool bHasHitJiggle = Overlapper.HasFlag(HitJiggleFlag);
		const bool bHasHitAnim = Overlapper.HasFlag(HitAnimFlag);
		const bool bHasPatrolling = Overlapper.HasTrait<FPatrolling>();
		const bool bHasTrace = Overlapper.HasTrait<FTrace>();
		const bool bHasIsSubjective = Overlapper.HasTrait<FIsSubjective>();

		FVector Location = bHasLocated ? Overlapper.GetTrait<FLocated>().Location : FVector::ZeroVector;
		FVector Direction = bHasDirected ? Overlapper.GetTrait<FDirected>().Direction : FVector::ZeroVector;

		FDmgResult DmgResult;
		DmgResult.DamagedSubject = Overlapper;

		FBeingHit NewBeingHit = !bHasBeingHit ? FBeingHit() : Overlapper.GetTrait<FBeingHit>();

		// 击退
		FVector HitDirection = FVector::OneVector;
		if (bHasLocated) HitDirection = (Location - HitFromLocation).GetSafeNormal2D();

		// 距离衰减
		float Distance = FMath::PointDistToLine(StartLocation, EndLocation, Location);
		TRange<float> InputRange(0, Damage.DmgRadius);
		TRange<float> OutputRange(1, 0);
		float DmgFalloffMult = Damage.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;
		float DebuffFalloffMult = Debuff.bUseFalloff ? FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Distance) : 1;


		//-------------伤害和抗性------------

		float NormalDmgMult = 1;
		float FireDmgMult = 1;
		float IceDmgMult = 1;
		float PoisonDmgMult = 1;
		float PercentDmgMult = 1;

		if (bHasHealth)
		{
			auto& Health = Overlapper.GetTraitRef<FHealth, EParadigm::Unsafe>();

			// 抗性 如果有的话
			if (bHasDefence)
			{
				const auto Defence = Overlapper.GetTrait<FDefence>();

				NormalDmgMult = 1 - Defence.NormalDmgImmune;
				FireDmgMult = 1 - Defence.FireDmgImmune;
				IceDmgMult = 1 - Defence.IceDmgImmune;
				PoisonDmgMult = 1 - Defence.PoisonDmgImmune;
				PercentDmgMult = 1.f - Defence.PercentDmgImmune;
			}

			// 基础伤害
			float BaseDamage = 0;

			switch (Damage.DmgType)
			{
			case EDmgType::Normal:
				BaseDamage = Damage.Damage * NormalDmgMult;
				break;
			case EDmgType::Fire:
				BaseDamage = Damage.Damage * FireDmgMult;
				break;
			case EDmgType::Ice:
				BaseDamage = Damage.Damage * IceDmgMult;
				break;
			case EDmgType::Poison:
				BaseDamage = Damage.Damage * PoisonDmgMult;
				break;
			}

			// 百分比伤害
			float PercentageDamage = Health.Maximum * Damage.PercentDmg * PercentDmgMult;

			// 总伤害
			float CombinedDamage = (BaseDamage + PercentageDamage) * DmgFalloffMult;

			// 考虑暴击后伤害
			auto [bIsCrit, PostCritDamage] = ProcessCritDamage(CombinedDamage, Damage.CritDmgMult, Damage.CritProbability);

			// 限制伤害以不大于剩余血量
			float ClampedDamage = FMath::Min(PostCritDamage, Health.Current);

			DmgResult.IsCritical = bIsCrit;
			DmgResult.DmgDealt = ClampedDamage;

			// 应用伤害
			Health.DamageToTake.Enqueue(ClampedDamage);
			Health.HitDirection.Enqueue(HitDirection);

			// 记录伤害施加者
			Health.DamageInstigator.Enqueue(DmgInstigator);
			DmgResult.InstigatorSubject = DmgInstigator;
			DmgResult.CauserSubject = DmgCauser;

			// ------------生成文字--------------

			if (bHasTextPopUp && bHasLocated)
			{
				const auto TextPopUp = Overlapper.GetTrait<FTextPopUp>();

				if (TextPopUp.Enable)
				{
					float Style = 0;

					if (!bIsCrit)
					{
						if (PostCritDamage < TextPopUp.WhiteTextBelowPercent)
						{
							Style = 0;
						}
						else if (PostCritDamage < TextPopUp.OrangeTextAbovePercent)
						{
							Style = 1;
						}
						else
						{
							Style = 2;
						}
					}
					else
					{
						Style = 3;
					}

					float Radius = bHasGridData ? Overlapper.GetTrait<FGridData>().Radius : 0;
					QueueText(FTextPopConfig(Overlapper, PostCritDamage, Style, TextPopUp.TextScale, Radius * 1.1, Location));
				}
			}

			//--------------Debuff--------------
			// 持续伤害
			if (Debuff.TemporalDmgParams.bDealTemporalDmg)
			{
				// Record for spawning of TemporalDamage
				FTemporalDamage TemporalDamage;

				float TotalTemporalDmg = Debuff.TemporalDmgParams.TemporalDmg;

				switch (Damage.DmgType)
				{
				case EDmgType::Normal:
					TotalTemporalDmg *= NormalDmgMult;
					break;
				case EDmgType::Fire:
					TotalTemporalDmg *= FireDmgMult;
					break;
				case EDmgType::Ice:
					TotalTemporalDmg *= IceDmgMult;
					break;
				case EDmgType::Poison:
					TotalTemporalDmg *= PoisonDmgMult;
					break;
				}

				TemporalDamage.TotalTemporalDamage = TotalTemporalDmg * DebuffFalloffMult;

				if (TemporalDamage.TotalTemporalDamage > 0)
				{
					TemporalDamage.TemporalDamageTarget = Overlapper;
					TemporalDamage.RemainingTemporalDamage = TemporalDamage.TotalTemporalDamage;

					if (DmgInstigator.IsValid())
					{
						TemporalDamage.TemporalDamageInstigator = DmgInstigator;
					}
					else
					{
						TemporalDamage.TemporalDamageInstigator = FSubjectHandle();
					}

					TemporalDamage.TemporalDmgSegment = Debuff.TemporalDmgParams.TemporalDmgSegment;
					TemporalDamage.TemporalDmgInterval = Debuff.TemporalDmgParams.TemporalDmgInterval;
					TemporalDamage.DmgType = Damage.DmgType;

					Mechanism->SpawnSubjectDeferred(TemporalDamage);
				}
			}
		}

		//--------------Debuff--------------

		if (Debuff.LaunchParams.bCanLaunch)
		{
			if (bHasMoving)
			{
				auto Moving = Overlapper.GetTrait<FMoving>();

				FVector KnockbackForce = FVector(Debuff.LaunchParams.LaunchSpeed.X, Debuff.LaunchParams.LaunchSpeed.X, 1) * HitDirection + FVector(0, 0, Debuff.LaunchParams.LaunchSpeed.Y);
				Moving.LaunchVelSum += KnockbackForce * DebuffFalloffMult; // 累加击退力

				Overlapper.SetTraitDeferred(Moving);
			}
		}

		// 减速
		if (Debuff.SlowParams.bCanSlow && bHasSlowing)
		{
			// Record for spawning of Slow
			FSlow Slow;

			Slow.SlowTarget = Overlapper;
			Slow.SlowStrength = Debuff.SlowParams.SlowStrength * DebuffFalloffMult;
			Slow.SlowTimeout = Debuff.SlowParams.SlowTime;
			Slow.DmgType = Damage.DmgType;
			Mechanism->SpawnSubjectDeferred(Slow);
		}

		//-----------其它效果------------

		if (bHasSleeping)// wake on hit
		{
			if (bHasSleep)
			{
				auto Sleep = Overlapper.GetTrait<FSleep>();

				if (Sleep.bWakeOnHit)
				{
					Sleep.bEnable = false;
					Overlapper.SetTraitDeferred(Sleep);
					Overlapper.RemoveTraitDeferred<FSleeping>();
				}
			}
		}

		if (bHasHit)
		{
			const auto Hit = Overlapper.GetTrait<FHit>();

			// Actor
			for (const FActorSpawnConfig& Config : Hit.SpawnActor)
			{
				FActorSpawnConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Fx
			for (const FFxConfig& Config : Hit.SpawnFx)
			{
				FFxConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Sound
			for (const FSoundConfig& Config : Hit.PlaySound)
			{
				FSoundConfig_Final NewConfig(Config);
				NewConfig.OwnerSubject = FSubjectHandle(Overlapper);
				NewConfig.AttachToSubject = FSubjectHandle(Overlapper);
				const FTransform WorldTransform(HitDirection.ToOrientationQuat(), Location);
				NewConfig.SpawnTransform = ABattleFrameBattleControl::LocalOffsetToWorld(Direction.ToOrientationQuat(), WorldTransform.GetLocation(), NewConfig.Transform);
				NewConfig.InitialRelativeTransform = NewConfig.SpawnTransform.GetRelativeTransform(WorldTransform);
				
				Mechanism->SpawnSubjectDeferred(NewConfig);
			}

			// Glow
			if (Hit.bCanGlow && !bHasHitGlow)
			{
				Overlapper.SetFlag(HitGlowFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetGlow();
				}
			}

			// Jiggle
			if (Hit.JiggleStr != 0 && !bHasHitJiggle)
			{
				Overlapper.SetFlag(HitJiggleFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetJiggle();
				}
			}

			// Anim
			if (Hit.bPlayAnim && !bHasHitAnim)
			{
				Overlapper.SetFlag(HitAnimFlag);

				if (bHasBeingHit)
				{
					NewBeingHit.ResetAnim();
				}
			}
		}

		Overlapper.SetTraitDeferred(NewBeingHit);

		if (bHasIsSubjective)
		{
			FHitData HitData;
			HitData.SelfSubject = DmgResult.DamagedSubject;
			HitData.InstigatorSubject = DmgResult.InstigatorSubject;
			HitData.CauserSubject = DmgResult.CauserSubject;
			HitData.IsCritical = DmgResult.IsCritical;
			HitData.IsKill = DmgResult.IsKill;
			HitData.DmgDealt = DmgResult.DmgDealt;
			OnHitQueue.Enqueue(HitData);
		}

		DamageResults.Add(DmgResult);
	}
}


//---------------------------------------------------A* Pathfinding-----------------------------------------------------

bool ABattleFrameBattleControl::FindPathAStar(AFlowField* FlowField, const FVector& StartLocation, const FVector& GoalLocation, TArray<FVector>& OutPath)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("FindPathAStar");
	if (FlowField->bIsBeginPlay) return false;

	OutPath.Empty();

	// 验证坐标有效性
	auto IsValidCoord = [&](FVector2D Coord) -> bool
		{
			return Coord.X >= 0 && Coord.X < FlowField->xNum && Coord.Y >= 0 && Coord.Y < FlowField->yNum;
		};

	FVector2D StartCoord;
	FlowField->WorldToGrid(StartLocation, StartCoord);
	FVector2D GoalCoord;
	FlowField->WorldToGrid(GoalLocation, GoalCoord);

	if (!IsValidCoord(StartCoord) || !IsValidCoord(GoalCoord)) return false;

	// 获取起点终点索引
	const int32 StartIndex = FlowField->CoordToIndex(StartCoord);
	const int32 GoalIndex = FlowField->CoordToIndex(GoalCoord);

	// 检查起点终点是否为障碍
	//if (FlowField->CurrentCellsArray[StartIndex].cost == 255 || FlowField->CurrentCellsArray[GoalIndex].cost == 255)
	//{
	//    return false;
	//}

	// 初始化数据结构
	TArray<float> gScore;
	TArray<FVector2D> CameFrom;
	gScore.Init(FLT_MAX, FlowField->CurrentCellsArray.Num());
	CameFrom.Init(FVector2D(-1, -1), FlowField->CurrentCellsArray.Num());

	// 自定义优先队列比较器
	struct FPriorityNode
	{
		float Priority;
		FVector2D Coord;
	};

	struct FPriorityCompare
	{
		bool operator()(const FPriorityNode& A, const FPriorityNode& B) const
		{
			return A.Priority > B.Priority;
		}
	};

	std::priority_queue<FPriorityNode, std::vector<FPriorityNode>, FPriorityCompare> OpenSet;

	// 初始化起点
	gScore[StartIndex] = 0;
	const float StartH = FVector2D::Distance(StartCoord, GoalCoord);
	OpenSet.push({ StartH, StartCoord });

	// A*主循环
	while (!OpenSet.empty())
	{
		const FVector2D CurrentCoord = OpenSet.top().Coord;
		OpenSet.pop();
		const int32 CurrentIndex = FlowField->CoordToIndex(CurrentCoord);

		// 到达终点
		if (CurrentCoord == GoalCoord)
		{
			// 重建路径
			TArray<FVector2D> PathCoords;
			FVector2D TraceCoord = GoalCoord;
			while (TraceCoord != StartCoord)
			{
				PathCoords.Add(TraceCoord);
				TraceCoord = CameFrom[FlowField->CoordToIndex(TraceCoord)];
			}
			PathCoords.Add(StartCoord);
			Algo::Reverse(PathCoords);

			// 转换为世界坐标
			TArray<FVector> MidPoints;
			
			for (const FVector2D& Coord : PathCoords)
			{
				MidPoints.Add(FlowField->CurrentCellsArray[FlowField->CoordToIndex(Coord)].worldLoc);
			}

			if (MidPoints.Num() > 1)
			{
				OutPath.Append(MidPoints);
				OutPath.Add(GoalLocation);
				int32 Num = OutPath.Num();
				OutPath[Num - 1].Z = OutPath[Num - 2].Z;
			}

			return MidPoints.Num() > 1;
		}

		// 生成邻居坐标 (8方向或4方向)
		TArray<FVector2D> NeighborCoords;
		if (FlowField->Style == EStyle::AdjacentFirst)
		{
			NeighborCoords = {
				CurrentCoord + FVector2D(1, -1),  // 右上
				CurrentCoord + FVector2D(1, 1),   // 右下
				CurrentCoord + FVector2D(-1, 1),  // 左下
				CurrentCoord + FVector2D(-1, -1), // 左上
				CurrentCoord + FVector2D(0, -1),  // 上
				CurrentCoord + FVector2D(1, 0),   // 右
				CurrentCoord + FVector2D(0, 1),   // 下
				CurrentCoord + FVector2D(-1, 0)   // 左
			};
		}
		else
		{
			NeighborCoords = {
				CurrentCoord + FVector2D(0, -1),  // 上
				CurrentCoord + FVector2D(1, 0),   // 右
				CurrentCoord + FVector2D(0, 1),   // 下
				CurrentCoord + FVector2D(-1, 0)   // 左
			};
		}

		// 处理邻居
		for (int32 i = 0; i < NeighborCoords.Num(); ++i)
		{
			const FVector2D& NeighborCoord = NeighborCoords[i];

			// 跳过无效坐标
			if (!IsValidCoord(NeighborCoord)) continue;

			const int32 NeighborIndex = FlowField->CoordToIndex(NeighborCoord);
			FCellStruct& NeighborCell = FlowField->CurrentCellsArray[NeighborIndex];
			FCellStruct& CurrentCell = FlowField->CurrentCellsArray[CurrentIndex];

			// 跳过障碍
			//if (FlowField->bIgnoreInternalObstacleCells && NeighborCell.cost == 255) continue;

			auto IsValidDiagonal = [&](TArray<FVector2D> neighborCoords, int32 indexA, int32 indexB) -> bool
				{
					bool result = true;

					if (IsValidCoord(neighborCoords[indexA]))
					{
						if (FlowField->CurrentCellsArray[FlowField->CoordToIndex(neighborCoords[indexA])].cost == 255)
						{
							result = false;
						}
					}
					if (IsValidCoord(neighborCoords[indexB]))
					{
						if (FlowField->CurrentCellsArray[FlowField->CoordToIndex(neighborCoords[indexB])].cost == 255)
						{
							result = false;
						}
					}
					return result;
				};

			// 对角线移动检查
			if (FlowField->Style == EStyle::AdjacentFirst && i < 4) // 前4个是斜角
			{
				bool bSkipDiagonal = false;
				switch (i)
				{
				case 0: bSkipDiagonal = !IsValidDiagonal(NeighborCoords, 4, 5); break; // 右上：检查上、右
				case 1: bSkipDiagonal = !IsValidDiagonal(NeighborCoords, 5, 6); break; // 右下：检查右、下
				case 2: bSkipDiagonal = !IsValidDiagonal(NeighborCoords, 6, 7); break; // 左下：检查下、左
				case 3: bSkipDiagonal = !IsValidDiagonal(NeighborCoords, 7, 4); break; // 左上：检查左、上
				}
				if (bSkipDiagonal) continue;
			}

			// 坡度检查
			const float HeightDiff = FMath::Abs(CurrentCell.worldLoc.Z - NeighborCell.worldLoc.Z);
			const float HorizontalDist = FVector2D::Distance(FVector2D(CurrentCell.worldLoc.X, CurrentCell.worldLoc.Y),FVector2D(NeighborCell.worldLoc.X, NeighborCell.worldLoc.Y));

			if (HorizontalDist > SMALL_NUMBER)
			{
				const float SlopeAngle = FMath::RadiansToDegrees(FMath::Atan(HeightDiff / HorizontalDist));
				if (SlopeAngle > FlowField->maxWalkableAngle && CurrentCell.cost != 255) continue;
			}

			// 计算移动成本
			const float MoveCost = NeighborCell.cost/*(NeighborCell.cost == 255) ? FLT_MAX : static_cast<float>(NeighborCell.cost)*/;
			const float TentativeG = gScore[CurrentIndex] + MoveCost;

			// 发现更优路径
			if (TentativeG < gScore[NeighborIndex])
			{
				CameFrom[NeighborIndex] = CurrentCoord;
				gScore[NeighborIndex] = TentativeG;
				const float H = FVector2D::Distance(NeighborCoord, GoalCoord);
				OpenSet.push({ TentativeG + H, NeighborCoord });
			}
		}
	}

	return false; // 未找到路径
}

void ABattleFrameBattleControl::GetSteeringDirection(const FVector& CurrentLocation, const FVector& GoalLocation, const TArray<FVector>& PathPoints, float MoveSpeed, float LookAheadDistance, float PathRadius, float AcceptanceRadius, FVector& SteeringDirection, bool& bHasPath, bool& bIsCurrentNearPath, bool& bIsGoalNearEnd)
{
	// 1. 是否有路径
	if (PathPoints.Num() < 2)
	{
		SteeringDirection = FVector::ZeroVector;
		bHasPath = false;
		return;
	}
	
	bHasPath = true;

	// 2. 找到当前位置在路径上的最近点
	int32 ClosestSegmentIndex = 0;
	float MinDistanceSq = FLT_MAX;
	FVector ClosestPointOnPath = FVector::ZeroVector;

	for (int32 i = 0; i < PathPoints.Num() - 1; i++)
	{
		const FVector& StartPoint = PathPoints[i];
		const FVector& EndPoint = PathPoints[i + 1];

		FVector ClosestPoint = FindClosestPointOnSegment(CurrentLocation, StartPoint, EndPoint);
		float DistanceSq = FVector::DistSquared(CurrentLocation, ClosestPoint);

		if (DistanceSq < MinDistanceSq)
		{
			MinDistanceSq = DistanceSq;
			ClosestPointOnPath = ClosestPoint;
			ClosestSegmentIndex = i;
		}
	}

	// 3. 验证位置条件
	bIsCurrentNearPath = FVector::DistSquared2D(CurrentLocation, ClosestPointOnPath) <= FMath::Square(PathRadius);
	bIsGoalNearEnd = FVector::DistSquared2D(GoalLocation, PathPoints.Last()) <= FMath::Square(AcceptanceRadius);

	// 4. 计算预测目标点
	const float PredictDistance = FMath::Max(LookAheadDistance, MoveSpeed * 0.5f);
	FVector TargetPoint = ClosestPointOnPath;
	float RemainingDistance = PredictDistance;
	int32 CurrentSegmentIndex = ClosestSegmentIndex;

	while (RemainingDistance > 0 && CurrentSegmentIndex < PathPoints.Num() - 1)
	{
		const FVector SegmentStart = (CurrentSegmentIndex == ClosestSegmentIndex) ? TargetPoint : PathPoints[CurrentSegmentIndex];
		const FVector SegmentEnd = PathPoints[CurrentSegmentIndex + 1];

		const FVector SegmentDir = (SegmentEnd - SegmentStart).GetSafeNormal();
		const float SegmentLength = FVector::Distance(SegmentStart, SegmentEnd);

		if (SegmentLength >= RemainingDistance)
		{
			TargetPoint = SegmentStart + SegmentDir * RemainingDistance;
			RemainingDistance = 0;
		}
		else
		{
			TargetPoint = SegmentEnd;
			RemainingDistance -= SegmentLength;
			CurrentSegmentIndex++;
		}
	}

	// 5. 计算最终方向
	SteeringDirection = (TargetPoint - CurrentLocation).GetSafeNormal2D();
}

FVector ABattleFrameBattleControl::FindClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint)
{
	const FVector Segment = EndPoint - StartPoint;
	const float SegmentLengthSq = Segment.SizeSquared();

	if (SegmentLengthSq < 0) return StartPoint;

	const float t = FMath::Clamp(FVector::DotProduct(Point - StartPoint, Segment) / SegmentLengthSq, 0.0f, 1.0f);
	return StartPoint + t * Segment;
}


//-------------------------------RVO2-2D Copyright 2023, EastFoxStudio. All Rights Reserved-------------------------------

void ABattleFrameBattleControl::ComputeAvoidingVelocity(FAvoidance& Avoidance, FAvoiding& Avoiding, const TArray<FGridData>& SubjectNeighbors, const TArray<FGridData>& ObstacleNeighbors, float TimeStep)
{
	FAvoiding SelfAvoiding = Avoiding;
	FAvoidance SelfAvoidance = Avoidance;

	int32 Reserve = FMath::Clamp(SubjectNeighbors.Num() + ObstacleNeighbors.Num(), 1, FLT_MAX);
	SelfAvoidance.OrcaLines.reserve(Reserve);

	/* Create obstacle ORCA lines. */
	if (!ObstacleNeighbors.IsEmpty())
	{
		const float invTimeHorizonObst = 1.0f / SelfAvoidance.RVO_TimeHorizon_Obstacle;

		for (const auto& Data : ObstacleNeighbors)
		{
			FBoxObstacle* obstacle1 = Data.SubjectHandle.GetTraitPtr<FBoxObstacle, EParadigm::Unsafe>();
			FBoxObstacle* obstacle2 = obstacle1->nextObstacle_.GetTraitPtr<FBoxObstacle, EParadigm::Unsafe>();

			const RVO::Vector2 relativePosition1 = obstacle1->point_ - SelfAvoiding.Position;
			const RVO::Vector2 relativePosition2 = obstacle2->point_ - SelfAvoiding.Position;

			/*
			 * Check if velocity obstacle of obstacle is already taken care of by
			 * previously constructed obstacle ORCA lines.
			 */
			bool alreadyCovered = false;

			for (size_t j = 0; j < SelfAvoidance.OrcaLines.size(); ++j) {
				if (RVO::det(invTimeHorizonObst * relativePosition1 - SelfAvoidance.OrcaLines[j].point, SelfAvoidance.OrcaLines[j].direction) - invTimeHorizonObst * SelfAvoiding.Radius >= -RVO_EPSILON && det(invTimeHorizonObst * relativePosition2 - SelfAvoidance.OrcaLines[j].point, SelfAvoidance.OrcaLines[j].direction) - invTimeHorizonObst * SelfAvoiding.Radius >= -RVO_EPSILON) {
					alreadyCovered = true;
					break;
				}
			}

			if (alreadyCovered) {
				continue;
			}

			/* Not yet covered. Check for collisions. */

			const float distSq1 = RVO::absSq(relativePosition1);
			const float distSq2 = RVO::absSq(relativePosition2);

			const float radiusSq = RVO::sqr(SelfAvoiding.Radius);

			const RVO::Vector2 obstacleVector = obstacle2->point_ - obstacle1->point_;
			const float s = (-relativePosition1 * obstacleVector) / absSq(obstacleVector);
			const float distSqLine = absSq(-relativePosition1 - s * obstacleVector);

			RVO::Line line;

			if (s < 0.0f && distSq1 <= radiusSq) {
				/* Collision with left vertex. Ignore if non-convex. */
				if (obstacle1->isConvex_) {
					line.point = RVO::Vector2(0.0f, 0.0f);
					line.direction = normalize(RVO::Vector2(-relativePosition1.y(), relativePosition1.x()));
					SelfAvoidance.OrcaLines.push_back(line);
				}
				continue;
			}
			else if (s > 1.0f && distSq2 <= radiusSq) {
				/* Collision with right vertex. Ignore if non-convex
				 * or if it will be taken care of by neighoring obstace */
				if (obstacle2->isConvex_ && det(relativePosition2, obstacle2->unitDir_) >= 0.0f) {
					line.point = RVO::Vector2(0.0f, 0.0f);
					line.direction = normalize(RVO::Vector2(-relativePosition2.y(), relativePosition2.x()));
					SelfAvoidance.OrcaLines.push_back(line);
				}
				continue;
			}
			else if (s >= 0.0f && s < 1.0f && distSqLine <= radiusSq) {
				/* Collision with obstacle segment. */
				line.point = RVO::Vector2(0.0f, 0.0f);
				line.direction = -obstacle1->unitDir_;
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}

			/*
			 * No collision.
			 * Compute legs. When obliquely viewed, both legs can come from a single
			 * vertex. Legs extend cut-off line when nonconvex vertex.
			 */

			RVO::Vector2 leftLegDirection, rightLegDirection;

			if (s < 0.0f && distSqLine <= radiusSq) {
				/*
				 * Obstacle viewed obliquely so that left vertex
				 * defines velocity obstacle.
				 */
				if (!obstacle1->isConvex_) {
					/* Ignore obstacle. */
					continue;
				}

				obstacle2 = obstacle1;

				const float leg1 = std::sqrt(distSq1 - radiusSq);
				leftLegDirection = RVO::Vector2(relativePosition1.x() * leg1 - relativePosition1.y() * SelfAvoiding.Radius, relativePosition1.x() * SelfAvoiding.Radius + relativePosition1.y() * leg1) / distSq1;
				rightLegDirection = RVO::Vector2(relativePosition1.x() * leg1 + relativePosition1.y() * SelfAvoiding.Radius, -relativePosition1.x() * SelfAvoiding.Radius + relativePosition1.y() * leg1) / distSq1;
			}
			else if (s > 1.0f && distSqLine <= radiusSq) {
				/*
				 * Obstacle viewed obliquely so that
				 * right vertex defines velocity obstacle.
				 */
				if (!obstacle2->isConvex_) {
					/* Ignore obstacle. */
					continue;
				}

				obstacle1 = obstacle2;

				const float leg2 = std::sqrt(distSq2 - radiusSq);
				leftLegDirection = RVO::Vector2(relativePosition2.x() * leg2 - relativePosition2.y() * SelfAvoiding.Radius, relativePosition2.x() * SelfAvoiding.Radius + relativePosition2.y() * leg2) / distSq2;
				rightLegDirection = RVO::Vector2(relativePosition2.x() * leg2 + relativePosition2.y() * SelfAvoiding.Radius, -relativePosition2.x() * SelfAvoiding.Radius + relativePosition2.y() * leg2) / distSq2;
			}
			else {
				/* Usual situation. */
				if (obstacle1->isConvex_) {
					const float leg1 = std::sqrt(distSq1 - radiusSq);
					leftLegDirection = RVO::Vector2(relativePosition1.x() * leg1 - relativePosition1.y() * SelfAvoiding.Radius, relativePosition1.x() * SelfAvoiding.Radius + relativePosition1.y() * leg1) / distSq1;
				}
				else {
					/* Left vertex non-convex; left leg extends cut-off line. */
					leftLegDirection = -obstacle1->unitDir_;
				}

				if (obstacle2->isConvex_) {
					const float leg2 = std::sqrt(distSq2 - radiusSq);
					rightLegDirection = RVO::Vector2(relativePosition2.x() * leg2 + relativePosition2.y() * SelfAvoiding.Radius, -relativePosition2.x() * SelfAvoiding.Radius + relativePosition2.y() * leg2) / distSq2;
				}
				else {
					/* Right vertex non-convex; right leg extends cut-off line. */
					rightLegDirection = obstacle1->unitDir_;
				}
			}

			/*
			 * Legs can never point into neighboring edge when convex vertex,
			 * take cutoff-line of neighboring edge instead. If velocity projected on
			 * "foreign" leg, no constraint is added.
			 */

			const FSubjectHandle leftNeighbor = obstacle1->prevObstacle_;

			bool isLeftLegForeign = false;
			bool isRightLegForeign = false;

			if (obstacle1->isConvex_ && det(leftLegDirection, -obstacle1->unitDir_) >= 0.0f) {
				/* Left leg points into obstacle. */
				leftLegDirection = -obstacle1->unitDir_;
				isLeftLegForeign = true;
			}

			if (obstacle2->isConvex_ && det(rightLegDirection, obstacle2->unitDir_) <= 0.0f) {
				/* Right leg points into obstacle. */
				rightLegDirection = obstacle2->unitDir_;
				isRightLegForeign = true;
			}

			/* Compute cut-off centers. */
			const RVO::Vector2 leftCutoff = invTimeHorizonObst * (obstacle1->point_ - SelfAvoiding.Position);
			const RVO::Vector2 rightCutoff = invTimeHorizonObst * (obstacle2->point_ - SelfAvoiding.Position);
			const RVO::Vector2 cutoffVec = rightCutoff - leftCutoff;

			/* Project current velocity on velocity obstacle. */

			/* Check if current velocity is projected on cutoff circles. */
			const float t = (obstacle1 == obstacle2 ? 0.5f : ((SelfAvoiding.CurrentVelocity - leftCutoff) * cutoffVec) / absSq(cutoffVec));
			const float tLeft = ((SelfAvoiding.CurrentVelocity - leftCutoff) * leftLegDirection);
			const float tRight = ((SelfAvoiding.CurrentVelocity - rightCutoff) * rightLegDirection);

			if ((t < 0.0f && tLeft < 0.0f) || (obstacle1 == obstacle2 && tLeft < 0.0f && tRight < 0.0f)) {
				/* Project on left cut-off circle. */
				const RVO::Vector2 unitW = normalize(SelfAvoiding.CurrentVelocity - leftCutoff);

				line.direction = RVO::Vector2(unitW.y(), -unitW.x());
				line.point = leftCutoff + SelfAvoiding.Radius * invTimeHorizonObst * unitW;
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}
			else if (t > 1.0f && tRight < 0.0f) {
				/* Project on right cut-off circle. */
				const RVO::Vector2 unitW = normalize(SelfAvoiding.CurrentVelocity - rightCutoff);

				line.direction = RVO::Vector2(unitW.y(), -unitW.x());
				line.point = rightCutoff + SelfAvoiding.Radius * invTimeHorizonObst * unitW;
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}

			/*
			 * Project on left leg, right leg, or cut-off line, whichever is closest
			 * to velocity.
			 */
			const float distSqCutoff = ((t < 0.0f || t > 1.0f || obstacle1 == obstacle2) ? std::numeric_limits<float>::infinity() : absSq(SelfAvoiding.CurrentVelocity - (leftCutoff + t * cutoffVec)));
			const float distSqLeft = ((tLeft < 0.0f) ? std::numeric_limits<float>::infinity() : absSq(SelfAvoiding.CurrentVelocity - (leftCutoff + tLeft * leftLegDirection)));
			const float distSqRight = ((tRight < 0.0f) ? std::numeric_limits<float>::infinity() : absSq(SelfAvoiding.CurrentVelocity - (rightCutoff + tRight * rightLegDirection)));

			if (distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight) {
				/* Project on cut-off line. */
				line.direction = -obstacle1->unitDir_;
				line.point = leftCutoff + SelfAvoiding.Radius * invTimeHorizonObst * RVO::Vector2(-line.direction.y(), line.direction.x());
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}
			else if (distSqLeft <= distSqRight) {
				/* Project on left leg. */
				if (isLeftLegForeign) {
					continue;
				}

				line.direction = leftLegDirection;
				line.point = leftCutoff + SelfAvoiding.Radius * invTimeHorizonObst * RVO::Vector2(-line.direction.y(), line.direction.x());
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}
			else {
				/* Project on right leg. */
				if (isRightLegForeign) {
					continue;
				}

				line.direction = -rightLegDirection;
				line.point = rightCutoff + SelfAvoiding.Radius * invTimeHorizonObst * RVO::Vector2(-line.direction.y(), line.direction.x());
				SelfAvoidance.OrcaLines.push_back(line);
				continue;
			}
		}
	}

	const size_t numObstLines = SelfAvoidance.OrcaLines.size();

	/* Create agent ORCA lines. */
	if (LIKELY(!SubjectNeighbors.IsEmpty()))
	{
		const float invTimeHorizon = 1.0f / SelfAvoidance.RVO_TimeHorizon_Agent;

		for (const auto& Data : SubjectNeighbors)
		{
			const auto& OtherAvoiding = Data.SubjectHandle.GetTraitRef<FAvoiding,EParadigm::Unsafe>();
			const RVO::Vector2 relativePosition = OtherAvoiding.Position - SelfAvoiding.Position;
			const RVO::Vector2 relativeVelocity = SelfAvoiding.CurrentVelocity - OtherAvoiding.CurrentVelocity;
			const float distSq = absSq(relativePosition);
			const float combinedRadius = SelfAvoiding.Radius + OtherAvoiding.Radius;
			const float combinedRadiusSq = RVO::sqr(combinedRadius);

			RVO::Line line;
			RVO::Vector2 u;

			if (distSq > combinedRadiusSq) {
				/* No collision. */
				const RVO::Vector2 w = relativeVelocity - invTimeHorizon * relativePosition;
				/* Vector from cutoff center to relative velocity. */
				const float wLengthSq = RVO::absSq(w);

				const float dotProduct1 = w * relativePosition;

				if (dotProduct1 < 0.0f && RVO::sqr(dotProduct1) > combinedRadiusSq * wLengthSq) {
					/* Project on cut-off circle. */
					const float wLength = std::sqrt(wLengthSq);
					const RVO::Vector2 unitW = w / wLength;

					line.direction = RVO::Vector2(unitW.y(), -unitW.x());
					u = (combinedRadius * invTimeHorizon - wLength) * unitW;
				}
				else {
					/* Project on legs. */
					const float leg = std::sqrt(distSq - combinedRadiusSq);

					if (det(relativePosition, w) > 0.0f) {
						/* Project on left leg. */
						line.direction = RVO::Vector2(relativePosition.x() * leg - relativePosition.y() * combinedRadius, relativePosition.x() * combinedRadius + relativePosition.y() * leg) / distSq;
					}
					else {
						/* Project on right leg. */
						line.direction = -RVO::Vector2(relativePosition.x() * leg + relativePosition.y() * combinedRadius, -relativePosition.x() * combinedRadius + relativePosition.y() * leg) / distSq;
					}

					const float dotProduct2 = relativeVelocity * line.direction;

					u = dotProduct2 * line.direction - relativeVelocity;
				}
			}
			else {
				/* Collision. Project on cut-off circle of time timeStep. */
				const float invTimeStep = 1.0f / TimeStep;

				/* Vector from cutoff center to relative velocity. */
				const RVO::Vector2 w = relativeVelocity - invTimeStep * relativePosition;

				const float wLength = abs(w);
				const RVO::Vector2 unitW = w / wLength;

				line.direction = RVO::Vector2(unitW.y(), -unitW.x());
				u = (combinedRadius * invTimeStep - wLength) * unitW;
			}

			float Ratio = OtherAvoiding.bCanAvoid ? 0.5f : 1.f;
			line.point = SelfAvoiding.CurrentVelocity + Ratio * u;
			SelfAvoidance.OrcaLines.push_back(line);
		}
	}

	size_t lineFail = LinearProgram2(SelfAvoidance.OrcaLines, SelfAvoidance.MaxSpeed, SelfAvoidance.DesiredVelocity, false, SelfAvoidance.AvoidingVelocity);

	if (lineFail < SelfAvoidance.OrcaLines.size()) 
	{
		LinearProgram3(SelfAvoidance.OrcaLines, numObstLines, lineFail, SelfAvoidance.MaxSpeed, SelfAvoidance.AvoidingVelocity);
	}

	//SelfAvoidance.OrcaLines.clear();
	Avoidance.AvoidingVelocity = SelfAvoidance.AvoidingVelocity;
}

bool ABattleFrameBattleControl::LinearProgram1(const std::vector<RVO::Line>& lines, size_t lineNo, float radius, const RVO::Vector2& optVelocity, bool directionOpt, RVO::Vector2& result)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("linearProgram1");
	const float dotProduct = lines[lineNo].point * lines[lineNo].direction;
	const float discriminant = RVO::sqr(dotProduct) + RVO::sqr(radius) - absSq(lines[lineNo].point);

	if (discriminant < 0.0f) {
		/* Max speed circle fully invalidates line lineNo. */
		return false;
	}

	const float sqrtDiscriminant = std::sqrt(discriminant);
	float tLeft = -dotProduct - sqrtDiscriminant;
	float tRight = -dotProduct + sqrtDiscriminant;

	for (size_t i = 0; i < lineNo; ++i) {
		const float denominator = det(lines[lineNo].direction, lines[i].direction);
		const float numerator = det(lines[i].direction, lines[lineNo].point - lines[i].point);

		if (std::fabs(denominator) <= RVO_EPSILON) {
			/* Lines lineNo and i are (almost) parallel. */
			if (numerator < 0.0f) {
				return false;
			}
			else {
				continue;
			}
		}

		const float t = numerator / denominator;

		if (denominator >= 0.0f) {
			/* Line i bounds line lineNo on the right. */
			tRight = std::min(tRight, t);
		}
		else {
			/* Line i bounds line lineNo on the left. */
			tLeft = std::max(tLeft, t);
		}

		if (tLeft > tRight) {
			return false;
		}
	}

	if (directionOpt) {
		/* Optimize direction. */
		if (optVelocity * lines[lineNo].direction > 0.0f) {
			/* Take right extreme. */
			result = lines[lineNo].point + tRight * lines[lineNo].direction;
		}
		else {
			/* Take left extreme. */
			result = lines[lineNo].point + tLeft * lines[lineNo].direction;
		}
	}
	else {
		/* Optimize closest point. */
		const float t = lines[lineNo].direction * (optVelocity - lines[lineNo].point);

		if (t < tLeft) {
			result = lines[lineNo].point + tLeft * lines[lineNo].direction;
		}
		else if (t > tRight) {
			result = lines[lineNo].point + tRight * lines[lineNo].direction;
		}
		else {
			result = lines[lineNo].point + t * lines[lineNo].direction;
		}
	}

	return true;
}

size_t ABattleFrameBattleControl::LinearProgram2(const std::vector<RVO::Line>& lines, float radius, const RVO::Vector2& optVelocity, bool directionOpt, RVO::Vector2& result)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("linearProgram2");
	if (directionOpt) {
		/*
		 * Optimize direction. Note that the optimization velocity is of unit
		 * length in this case.
		 */
		result = optVelocity * radius;
	}
	else if (RVO::absSq(optVelocity) > RVO::sqr(radius)) {
		/* Optimize closest point and outside circle. */
		result = normalize(optVelocity) * radius;
	}
	else {
		/* Optimize closest point and inside circle. */
		result = optVelocity;
	}

	for (size_t i = 0; i < lines.size(); ++i) {
		if (det(lines[i].direction, lines[i].point - result) > 0.0f) {
			/* Result does not satisfy constraint i. Compute new optimal result. */
			const RVO::Vector2 tempResult = result;

			if (!LinearProgram1(lines, i, radius, optVelocity, directionOpt, result)) {
				result = tempResult;
				return i;
			}
		}
	}

	return lines.size();
}

void ABattleFrameBattleControl::LinearProgram3(const std::vector<RVO::Line>& lines, size_t numObstLines, size_t beginLine, float radius, RVO::Vector2& result)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("linearProgram3");
	float distance = 0.0f;

	for (size_t i = beginLine; i < lines.size(); ++i) {
		if (det(lines[i].direction, lines[i].point - result) > distance) {
			/* Result does not satisfy constraint of line i. */
			std::vector<RVO::Line> projLines(lines.begin(), lines.begin() + static_cast<ptrdiff_t>(numObstLines));

			for (size_t j = numObstLines; j < i; ++j) {
				RVO::Line line;

				float determinant = det(lines[i].direction, lines[j].direction);

				if (std::fabs(determinant) <= RVO_EPSILON) {
					/* Line i and line j are parallel. */
					if (lines[i].direction * lines[j].direction > 0.0f) {
						/* Line i and line j point in the same direction. */
						continue;
					}
					else {
						/* Line i and line j point in opposite direction. */
						line.point = 0.5f * (lines[i].point + lines[j].point);
					}
				}
				else {
					line.point = lines[i].point + (det(lines[j].direction, lines[i].point - lines[j].point) / determinant) * lines[i].direction;
				}

				line.direction = normalize(lines[j].direction - lines[i].direction);
				projLines.push_back(line);
			}

			const RVO::Vector2 tempResult = result;

			if (LinearProgram2(projLines, radius, RVO::Vector2(-lines[i].direction.y(), lines[i].direction.x()), true, result) < projLines.size()) {
				/* This should in principle not happen.  The result is by definition
				 * already in the feasible region of this linear program. If it fails,
				 * it is due to small floating point error, and the current result is
				 * kept.
				 */
				result = tempResult;
			}

			distance = det(lines[i].direction, lines[i].point - result);
		}
	}
}
