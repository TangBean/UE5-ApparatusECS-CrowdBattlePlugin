/*
 * APPARATIST
 * Created: 2023-02-02 16:26:26
 * Author: Vladislav Dmitrievich Turbanov (vladislav@turbanov.ru)
 * Community forums: https://talk.turbanov.ru
 * Copyright 2019 - 2023, SP Vladislav Dmitrievich Turbanov
 * Made in Russia, Moscow City, Chekhov City
 */

 /*
  * BattleFrame
  * Refactor: 2025
  * Author: Leroy Works
  */

#include "NeighborGridComponent.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "BitMask.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "BattleFrameFunctionLibraryRT.h"
#include "BattleFrameBattleControl.h"
#include "Kismet/BlueprintAsyncActionBase.h"

UNeighborGridComponent::UNeighborGridComponent()
{
	bWantsInitializeComponent = true;
}

void UNeighborGridComponent::BeginPlay()
{
	Super::BeginPlay();
	DefineFilters();
}

void UNeighborGridComponent::InitializeComponent()
{
	DoInitializeCells();
	GetBounds();
}

void UNeighborGridComponent::DefineFilters()
{
	RegisterNeighborGrid_Trace_Filter = FFilter::Make<FLocated, FTrace, FActivated>();
	RegisterNeighborGrid_SphereObstacle_Filter = FFilter::Make<FLocated, FSphereObstacle>();
	RegisterSubjectFilter = FFilter::Make<FLocated, FScaled, FCollider, FGridData, FActivated>().Exclude<FSphereObstacle>();
	RegisterSphereObstaclesFilter = FFilter::Make<FLocated, FCollider, FGridData, FSphereObstacle>();
	RegisterBoxObstaclesFilter = FFilter::Make<FBoxObstacle, FLocated, FGridData>();
	SubjectFilterBase = FFilter::Make<FLocated, FCollider, FAvoidance, FAvoiding, FGridData, FActivated>().Exclude<FSphereObstacle, FBoxObstacle>().ExcludeFlag(DeathDisableCollisionFlag);
	SphereObstacleFilter = FFilter::Make<FSphereObstacle, FGridData, FAvoidance, FAvoiding, FLocated, FCollider>();
	BoxObstacleFilter = FFilter::Make<FBoxObstacle, FGridData, FLocated>();
	DecoupleFilter = FFilter::Make<FAgent, FLocated, FDirected, FCollider, FMove, FMoving, FAvoidance, FAvoiding, FGridData, FActivated>().Exclude<FAppearing>();
}


//--------------------------------------------Tracing----------------------------------------------------------------

// Multi Trace For Subjects
void UNeighborGridComponent::SphereTraceForSubjects
(
	int32 KeepCount,
	const FVector& Origin,
	const float Radius,
	const bool bCheckObstacle,
	const FVector& CheckOrigin,
	const float CheckRadius,
	ESortMode SortMode,
	const FVector& SortOrigin,
	const FSubjectArray& IgnoreSubjects,
	const FBFFilter& Filter,
	const FTraceDrawDebugConfig& DrawDebugConfig,
	bool& Hit,
	TArray<FTraceResult>& Results
) const
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("SphereTraceForSubjects");
	Results.Reset();

	// 特殊处理标志
	const bool bSingleResult = (KeepCount == 1);
	const bool bNoCountLimit = (KeepCount == -1);

	// 将忽略列表转换为集合以便快速查找
	const TSet<FSubjectHandle> IgnoreSet(IgnoreSubjects.Subjects);

	// 扩展搜索范围 - 使用各轴独立的CellSize
	const FVector CellRadius = CellSize * 0.5f;
	const float MaxCellRadius = FMath::Max3(CellRadius.X, CellRadius.Y, CellRadius.Z);
	const float ExpandedRadius = Radius + MaxCellRadius * FMath::Sqrt(2.0f);
	const FVector Range(ExpandedRadius);

	const FIntVector CoordMin = LocationToCoord(Origin - Range);
	const FIntVector CoordMax = LocationToCoord(Origin + Range);

	// 跟踪最佳结果（用于KeepCount=1的情况）
	FTraceResult BestResult;
	float BestDistSq = (SortMode == ESortMode::NearToFar) ? FLT_MAX : -FLT_MAX;

	// 临时存储所有结果（用于需要排序或随机的情况）
	TArray<FTraceResult> TempResults;

	// 预收集候选格子并按距离排序
	TArray<FIntVector> CandidateCells;

	for (int32 z = CoordMin.Z; z <= CoordMax.Z; ++z)
	{
		for (int32 y = CoordMin.Y; y <= CoordMax.Y; ++y)
		{
			for (int32 x = CoordMin.X; x <= CoordMax.X; ++x)
			{
				const FIntVector Coord(x, y, z);
				if (!IsInside(Coord)) continue;

				const FVector CellCenter = CoordToLocation(Coord);
				const float DistSq = FVector::DistSquared(CellCenter, Origin);

				if (DistSq > FMath::Square(ExpandedRadius)) continue;

				CandidateCells.Add(Coord);
			}
		}
	}

	// 根据SortMode对格子进行排序
	if (SortMode != ESortMode::None)
	{
		CandidateCells.Sort([this, SortOrigin, SortMode](const FIntVector& A, const FIntVector& B) {
			const float DistSqA = FVector::DistSquared(CoordToLocation(A), SortOrigin);
			const float DistSqB = FVector::DistSquared(CoordToLocation(B), SortOrigin);
			return SortMode == ESortMode::NearToFar ? DistSqA < DistSqB : DistSqA > DistSqB;
			});
	}

	// 计算提前终止阈值
	float ThresholdDistanceSq = FLT_MAX;

	if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
	{
		// 计算阈值：当前最远结果的距离 + 2倍格子对角线距离（使用最大轴尺寸）
		const float MaxCellSize = CellSize.GetMax();
		const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
		ThresholdDistanceSq = FMath::Square(ThresholdDistance);
	}

	// 遍历检测
	TArray<FHitResult> VisibilityResults;
	TArray<uint32> SeenHashes;
	SeenHashes.Reserve(32);

	FFilter SubjectFilter;
	SubjectFilter.Include(Filter.IncludeTraits);
	SubjectFilter.Exclude(Filter.ExcludeTraits);

	for (const FIntVector& Coord : CandidateCells)
	{
		// 提前终止检查
		if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
		{
			const float CellDistSq = FVector::DistSquared(CoordToLocation(Coord), SortOrigin);
			if ((SortMode == ESortMode::NearToFar && CellDistSq > ThresholdDistanceSq) || (SortMode == ESortMode::FarToNear && CellDistSq < ThresholdDistanceSq)) break;
		}

		const auto& CellData = GetCellAt(SubjectCells, Coord);
		if (!CellData.Fingerprint.Matches(Filter.IncludeTraits)) continue;

		for (const FGridData& SubjectData : CellData.Subjects)
		{
			const FSubjectHandle Subject = SubjectData.SubjectHandle;

			// 有效性检查
			if (UNLIKELY(!Subject.IsValid())) continue;

			// 忽略列表检查
			if (UNLIKELY(IgnoreSet.Contains(Subject))) continue;

			// 距离检查
			const FVector SubjectPos = FVector(SubjectData.Location);
			const float SubjectRadius = SubjectData.Radius;
			const FVector Delta = SubjectPos - Origin;
			const float DistSq = Delta.SizeSquared();
			const float CombinedRadius = Radius + SubjectRadius;
			if (DistSq > FMath::Square(CombinedRadius)) continue;

			// 去重
			if (SeenHashes.Contains(SubjectData.SubjectHash)) continue;
			SeenHashes.Add(SubjectData.SubjectHash);

			// 特征过滤
			if (!Subject.Matches(SubjectFilter)) continue;

			// 障碍物检查
			const FVector CheckOriginToSubjectDir = (SubjectPos - CheckOrigin).GetSafeNormal();
			const FVector CheckOriginToSubjectSurfacePoint = SubjectPos - (CheckOriginToSubjectDir * SubjectRadius);

			if (bCheckObstacle && !Filter.ObstacleObjectType.IsEmpty())
			{
				FHitResult VisibilityResult;

				bool bVisibilityHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
					GetWorld(),
					CheckOrigin,
					CheckOriginToSubjectSurfacePoint,
					CheckRadius,
					Filter.ObstacleObjectType,
					true,
					TArray<TObjectPtr<AActor>>(),
					EDrawDebugTrace::None,
					VisibilityResult,
					true,
					FLinearColor::Gray,
					FLinearColor::Red,
					1);

				if (DrawDebugConfig.bDrawDebugShape)
				{
					VisibilityResults.Add(VisibilityResult);
				}

				if (bVisibilityHit) continue;
			}

			const float CurrentDistSq = FVector::DistSquared(SortOrigin, SubjectPos);

			const FVector TraceOriginToSubjectDir = (SubjectPos - Origin).GetSafeNormal();
			const FVector TraceHitLocation = SubjectPos - (TraceOriginToSubjectDir * SubjectRadius);

			if (bSingleResult)
			{
				bool bIsBetter = false;
				if (SortMode == ESortMode::NearToFar)
				{
					bIsBetter = (CurrentDistSq < BestDistSq);
				}
				else if (SortMode == ESortMode::FarToNear)
				{
					bIsBetter = (CurrentDistSq > BestDistSq);
				}
				else // ESortMode::None
				{
					bIsBetter = true;
				}

				if (bIsBetter)
				{
					BestResult.Subject = Subject;
					BestResult.SubjectLocation = SubjectPos;
					BestResult.HitLocation = TraceHitLocation;
					BestResult.ShapeLocation = Origin;
					BestResult.CachedDistSq = CurrentDistSq;
					BestDistSq = CurrentDistSq;

					// 更新阈值
					if (!bNoCountLimit && KeepCount > 0)
					{
						const float MaxCellSize = CellSize.GetMax();
						const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
						ThresholdDistanceSq = FMath::Square(ThresholdDistance);
					}
				}
			}
			else
			{
				FTraceResult Result;
				Result.Subject = Subject;
				Result.SubjectLocation = SubjectPos;
				Result.HitLocation = TraceHitLocation;
				Result.ShapeLocation = Origin;
				Result.CachedDistSq = CurrentDistSq;
				TempResults.Add(Result);

				// 当收集到足够结果时更新阈值
				if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None && TempResults.Num() >= KeepCount)
				{
					// 找到当前第KeepCount个最佳结果的距离
					float CurrentThresholdDistSq = 0.0f;
					if (SortMode == ESortMode::NearToFar)
					{
						CurrentThresholdDistSq = TempResults[KeepCount - 1].CachedDistSq;
					}
					else // FarToNear
					{
						CurrentThresholdDistSq = TempResults.Last().CachedDistSq;
					}

					// 更新阈值
					const float MaxCellSize = CellSize.GetMax();
					const float ThresholdDistance = FMath::Sqrt(CurrentThresholdDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
					ThresholdDistanceSq = FMath::Square(ThresholdDistance);
				}
			}
		}
	}

	// 处理结果
	if (bSingleResult)
	{
		if (BestDistSq != FLT_MAX && BestDistSq != -FLT_MAX)
		{
			Results.Add(BestResult);
		}
	}
	else if (TempResults.Num() > 0)
	{
		// 处理排序或随机化
		if (SortMode != ESortMode::None)
		{
			// 应用排序（无论KeepCount是否为-1）
			TempResults.Sort([SortMode](const FTraceResult& A, const FTraceResult& B) {
				return SortMode == ESortMode::NearToFar ?
					A.CachedDistSq < B.CachedDistSq :
					A.CachedDistSq > B.CachedDistSq;
				});
		}
		else
		{
			// 随机打乱结果
			if (TempResults.Num() > 1)
			{
				for (int32 i = TempResults.Num() - 1; i > 0; --i)
				{
					const int32 j = FMath::Rand() % (i + 1);
					TempResults.Swap(i, j);
				}
			}
		}

		// 应用数量限制（只有当KeepCount>0时）
		if (!bNoCountLimit && KeepCount > 0 && TempResults.Num() > KeepCount)
		{
			TempResults.SetNum(KeepCount);
		}

		Results.Append(TempResults);
	}

	Hit = !Results.IsEmpty();

	if (DrawDebugConfig.bDrawDebugShape)
	{
		// trace range
		FDebugSphereConfig Config;
		Config.Color = DrawDebugConfig.Color;
		Config.Location = Origin;
		Config.Radius = Radius;
		Config.Duration = DrawDebugConfig.Duration;
		ABattleFrameBattleControl::GetInstance()->DebugSphereQueue.Enqueue(Config);

		// trace results
		for (const auto& Result : Results)
		{
			FVector OtherLocation = Result.Subject.GetTrait<FLocated>().Location;
			FDebugSphereConfig SphereConfig;
			SphereConfig.Color = DrawDebugConfig.Color;
			SphereConfig.Location = Result.Subject.GetTrait<FLocated>().Location;
			SphereConfig.Radius = Result.Subject.GetTrait<FGridData>().Radius;
			SphereConfig.Duration = DrawDebugConfig.Duration;
			ABattleFrameBattleControl::GetInstance()->DebugSphereQueue.Enqueue(SphereConfig);
		}

		// visibility check results
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
			float CylinderHeight = FMath::Max(0.0f, TotalDistance + 2.0f * CheckRadius);

			// 计算胶囊体中心位置（两点中点）
			FVector ShapeLoc = (VisibilityResult.TraceStart + VisibilityResult.TraceEnd) * 0.5f;

			// 配置调试胶囊体参数
			FDebugCapsuleConfig CapsuleConfig;
			CapsuleConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			CapsuleConfig.Location = ShapeLoc;
			CapsuleConfig.Rotation = ShapeRot;
			CapsuleConfig.Radius = CheckRadius;
			CapsuleConfig.Height = CylinderHeight;
			CapsuleConfig.LineThickness = DrawDebugConfig.LineThickness;
			CapsuleConfig.Duration = DrawDebugConfig.Duration;

			// 加入调试队列
			ABattleFrameBattleControl::GetInstance()->DebugCapsuleQueue.Enqueue(CapsuleConfig);

			// 绘制碰撞点
			FDebugPointConfig PointConfig;
			PointConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			PointConfig.Duration = DrawDebugConfig.Duration;
			PointConfig.Location = VisibilityResult.bBlockingHit ? VisibilityResult.ImpactPoint : VisibilityResult.TraceEnd;
			PointConfig.Size = DrawDebugConfig.HitPointSize;

			ABattleFrameBattleControl::GetInstance()->DebugPointQueue.Enqueue(PointConfig);
		}
	}
}

// Multi Sweep Trace For Subjects
void UNeighborGridComponent::SphereSweepForSubjects(
	int32 KeepCount,
	const FVector& Start,
	const FVector& End,
	const float Radius,
	const bool bCheckObstacle,
	const FVector& CheckOrigin,
	const float CheckRadius,
	ESortMode SortMode,
	const FVector& SortOrigin,
	const FSubjectArray& IgnoreSubjects,
	const FBFFilter& Filter,
	const FTraceDrawDebugConfig& DrawDebugConfig,
	bool& Hit,
	TArray<FTraceResult>& Results
) const
{
	Results.Reset();
	Hit = false; // 确保初始值为false

	// 特殊处理标志
	const bool bSingleResult = (KeepCount == 1);
	const bool bNoCountLimit = (KeepCount == -1);

	// 将忽略列表转换为集合以便快速查找
	const TSet<FSubjectHandle> IgnoreSet(IgnoreSubjects.Subjects);

	// 获取沿扫描路径的网格
	TArray<FIntVector> GridCells = SphereSweepForCells(Start, End, Radius);
	const FVector TraceDir = (End - Start).GetSafeNormal();
	const float TraceLength = FVector::Distance(Start, End);

	// 跟踪最佳结果（用于KeepCount=1的情况）
	FTraceResult BestResult;
	float BestDistSq = (SortMode == ESortMode::NearToFar) ? FLT_MAX : -FLT_MAX;

	// 临时存储所有结果
	TArray<FTraceResult> TempResults;

	// 根据SortMode对网格进行排序（新增）
	if (SortMode != ESortMode::None)
	{
		GridCells.Sort([this, SortOrigin, SortMode](const FIntVector& A, const FIntVector& B) {
			const float DistSqA = FVector::DistSquared(CoordToLocation(A), SortOrigin);
			const float DistSqB = FVector::DistSquared(CoordToLocation(B), SortOrigin);
			return SortMode == ESortMode::NearToFar ? DistSqA < DistSqB : DistSqA > DistSqB;
			});
	}

	// 计算提前终止阈值（新增）
	float ThresholdDistanceSq = FLT_MAX;

	if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
	{
		// 计算阈值：当前最远结果的距离 + 2倍格子对角线距离
		const float MaxCellSize = CellSize.GetMax();
		const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
		ThresholdDistanceSq = FMath::Square(ThresholdDistance);
	}

	// 遍历网格
	TArray<FHitResult> VisibilityResults;
	TArray<uint32> SeenHashes;
	SeenHashes.Reserve(32);

	FFilter SubjectFilter;
	SubjectFilter.Include(Filter.IncludeTraits);
	SubjectFilter.Exclude(Filter.ExcludeTraits);

	for (const FIntVector& CellIndex : GridCells)
	{
		// 提前终止检查（新增）
		if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
		{
			const float CellDistSq = FVector::DistSquared(CoordToLocation(CellIndex), SortOrigin);
			if ((SortMode == ESortMode::NearToFar && CellDistSq > ThresholdDistanceSq) || (SortMode == ESortMode::FarToNear && CellDistSq < ThresholdDistanceSq)) break;
		}

		const auto& CellData = GetCellAt(SubjectCells, CellIndex);

		if (!CellData.Fingerprint.Matches(Filter.IncludeTraits)) continue;

		for (const FGridData& Data : CellData.Subjects)
		{
			const FSubjectHandle Subject = Data.SubjectHandle;

			// 有效性检查
			if (UNLIKELY(!Subject.IsValid())) continue;

			// 忽略列表检查
			if (UNLIKELY(IgnoreSet.Contains(Subject))) continue;

			// 距离检查
			const FVector SubjectPos = FVector(Data.Location);
			float SubjectRadius = Data.Radius;
			const FVector ToSubject = SubjectPos - Start;
			const float ProjOnTrace = FVector::DotProduct(ToSubject, TraceDir);
			const float ProjThreshold = SubjectRadius + Radius;
			if (LIKELY(ProjOnTrace < -ProjThreshold || ProjOnTrace > TraceLength + ProjThreshold)) continue; // 粗过滤
		
			const float ClampedProj = FMath::Clamp(ProjOnTrace, 0.0f, TraceLength);
			const FVector NearestPoint = Start + ClampedProj * TraceDir;
			const float CombinedRadSq = FMath::Square(Radius + SubjectRadius);
			if (LIKELY(FVector::DistSquared(NearestPoint, SubjectPos) >= CombinedRadSq)) continue; // 精确过滤

			// 去重
			if (SeenHashes.Contains(Data.SubjectHash)) continue;
			SeenHashes.Add(Data.SubjectHash);

			// 特征检查
			if (!Subject.Matches(SubjectFilter)) continue;

			// 障碍物检查
			const FVector CheckOriginToSubjectDir = (SubjectPos - CheckOrigin).GetSafeNormal();
			const FVector CheckOriginToSubjectSurfacePoint = SubjectPos - (CheckOriginToSubjectDir * SubjectRadius);

			if (bCheckObstacle && !Filter.ObstacleObjectType.IsEmpty())
			{
				FHitResult VisibilityResult;
				bool bVisibilityHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
					GetWorld(),
					CheckOrigin,
					CheckOriginToSubjectSurfacePoint,
					CheckRadius,
					Filter.ObstacleObjectType,
					true,
					TArray<TObjectPtr<AActor>>(),
					EDrawDebugTrace::None,
					VisibilityResult,
					true,
					FLinearColor::Gray,
					FLinearColor::Red,
					0.5);

				if (DrawDebugConfig.bDrawDebugShape)
				{
					VisibilityResults.Add(VisibilityResult);
				}

				if (UNLIKELY(bVisibilityHit)) continue;
			}

			const float CurrentDistSq = FVector::DistSquared(SortOrigin, SubjectPos);

			const FVector TraceClosestPointOnLine = FMath::ClosestPointOnLine(Start, End, SubjectPos);
			const FVector TraceClosestPointToSubjectDir = (SubjectPos - TraceClosestPointOnLine).GetSafeNormal();
			const FVector TraceHitLocation = SubjectPos - (TraceClosestPointToSubjectDir * SubjectRadius);
			const FVector TraceShapeLocation = TraceHitLocation - (TraceClosestPointToSubjectDir * Radius);

			// 单结果处理（新增）
			if (bSingleResult)
			{
				bool bIsBetter = false;

				if (SortMode == ESortMode::NearToFar)
				{
					bIsBetter = (CurrentDistSq < BestDistSq);
				}
				else if (SortMode == ESortMode::FarToNear)
				{
					bIsBetter = (CurrentDistSq > BestDistSq);
				}
				else // ESortMode::None
				{
					bIsBetter = true;
				}

				if (bIsBetter)
				{
					BestResult.Subject = Subject;
					BestResult.SubjectLocation = SubjectPos;
					BestResult.HitLocation = TraceHitLocation;
					BestResult.ShapeLocation = TraceShapeLocation;
					BestResult.CachedDistSq = CurrentDistSq;
					BestDistSq = CurrentDistSq;

					// 更新阈值
					if (!bNoCountLimit && KeepCount > 0)
					{
						const float MaxCellSize = CellSize.GetMax();
						const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
						ThresholdDistanceSq = FMath::Square(ThresholdDistance);
					}
				}
			}
			else // 多结果处理
			{
				FTraceResult Result;
				Result.Subject = Subject;
				Result.SubjectLocation = SubjectPos;
				Result.HitLocation = TraceHitLocation;
				Result.ShapeLocation = TraceShapeLocation;
				Result.CachedDistSq = CurrentDistSq;
				TempResults.Add(Result);

				// 当收集到足够结果时更新阈值（新增）
				if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None && TempResults.Num() >= KeepCount)
				{
					// 找到当前第KeepCount个最佳结果的距离
					float CurrentThresholdDistSq = 0.0f;
					if (SortMode == ESortMode::NearToFar)
					{
						// 注意：此时TempResults尚未排序，需要先排序
						TempResults.Sort([](const FTraceResult& A, const FTraceResult& B) {
							return A.CachedDistSq < B.CachedDistSq;
							});
						CurrentThresholdDistSq = TempResults[KeepCount - 1].CachedDistSq;
					}
					else // FarToNear
					{
						TempResults.Sort([](const FTraceResult& A, const FTraceResult& B) {
							return A.CachedDistSq > B.CachedDistSq;
							});
						CurrentThresholdDistSq = TempResults[KeepCount - 1].CachedDistSq;
					}

					// 更新阈值
					const float MaxCellSize = CellSize.GetMax();
					const float ThresholdDistance = FMath::Sqrt(CurrentThresholdDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
					ThresholdDistanceSq = FMath::Square(ThresholdDistance);
				}
			}
		}
	}

	// 处理结果
	if (bSingleResult)
	{
		if (BestDistSq != FLT_MAX && BestDistSq != -FLT_MAX)
		{
			Results.Add(BestResult);
			Hit = true;
		}
	}
	else if (TempResults.Num() > 0)
	{
		// 处理排序或随机化
		if (SortMode != ESortMode::None)
		{
			TempResults.Sort([SortMode](const FTraceResult& A, const FTraceResult& B) {
				return SortMode == ESortMode::NearToFar ?
					A.CachedDistSq < B.CachedDistSq :
					A.CachedDistSq > B.CachedDistSq;
				});
		}
		else
		{
			// 随机打乱结果
			if (TempResults.Num() > 1)
			{
				for (int32 i = TempResults.Num() - 1; i > 0; --i)
				{
					const int32 j = FMath::Rand() % (i + 1);
					TempResults.Swap(i, j);
				}
			}
		}

		// 应用数量限制
		if (!bNoCountLimit && KeepCount > 0 && TempResults.Num() > KeepCount)
		{
			TempResults.SetNum(KeepCount);
		}

		Results = TempResults;
		Hit = true;
	}

	if (DrawDebugConfig.bDrawDebugShape)
	{
		// trace range
		{
			// 计算起点到终点的向量
			FVector TraceDirection = End - Start;
			float TraceDistance = TraceDirection.Size();

			// 处理零距离情况（使用默认旋转）
			FRotator ShapeRot = FRotator::ZeroRotator;

			if (TraceDistance > 0)
			{
				TraceDirection /= TraceDistance;
				ShapeRot = FRotationMatrix::MakeFromZ(TraceDirection).Rotator();
			}

			// 计算圆柱部分高度（总高度减去两端的半球）
			float CylinderHeight = FMath::Max(0.0f, TraceDistance + 2.0f * Radius);

			// 计算胶囊体中心位置（两点中点）
			FVector ShapeLoc = (Start + End) * 0.5f;

			// 配置调试胶囊体参数
			FDebugCapsuleConfig CapsuleConfig;
			CapsuleConfig.Color = DrawDebugConfig.Color;
			CapsuleConfig.Location = ShapeLoc;
			CapsuleConfig.Rotation = ShapeRot;  // 修正后的旋转
			CapsuleConfig.Radius = Radius;
			CapsuleConfig.Height = CylinderHeight;  // 圆柱部分高度
			CapsuleConfig.LineThickness = DrawDebugConfig.LineThickness;
			CapsuleConfig.Duration = DrawDebugConfig.Duration;

			// 加入调试队列
			ABattleFrameBattleControl::GetInstance()->DebugCapsuleQueue.Enqueue(CapsuleConfig);
		}

		// trace results
		for (const auto& Result : Results)
		{
			FDebugSphereConfig SphereConfig;
			SphereConfig.Color = DrawDebugConfig.Color;
			SphereConfig.Location = Result.Subject.GetTrait<FLocated>().Location;
			SphereConfig.Radius = Result.Subject.GetTrait<FGridData>().Radius;
			SphereConfig.Duration = DrawDebugConfig.Duration;
			SphereConfig.LineThickness = DrawDebugConfig.LineThickness;
			ABattleFrameBattleControl::GetInstance()->DebugSphereQueue.Enqueue(SphereConfig);
		}

		// visibility check results
		for (const auto& VisibilityResult : VisibilityResults)
		{
			// 计算起点到终点的向量
			FVector VisCheckDirection = VisibilityResult.TraceEnd - VisibilityResult.TraceStart;
			float VisCheckDistance = VisCheckDirection.Size();

			// 处理零距离情况（使用默认旋转）
			FRotator ShapeRot = FRotator::ZeroRotator;

			if (VisCheckDistance > 0)
			{
				VisCheckDirection /= VisCheckDistance;
				ShapeRot = FRotationMatrix::MakeFromZ(VisCheckDirection).Rotator();
			}

			// 计算圆柱部分高度（总高度减去两端的半球）
			float CylinderHeight = FMath::Max(0.0f, VisCheckDistance + 2.0f * CheckRadius);

			// 计算胶囊体中心位置（两点中点）
			FVector ShapeLoc = (VisibilityResult.TraceStart + VisibilityResult.TraceEnd) * 0.5f;

			// 配置调试胶囊体参数
			FDebugCapsuleConfig CapsuleConfig;
			CapsuleConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			CapsuleConfig.Location = ShapeLoc;
			CapsuleConfig.Rotation = ShapeRot;
			CapsuleConfig.Radius = CheckRadius;
			CapsuleConfig.Height = CylinderHeight;
			CapsuleConfig.LineThickness = DrawDebugConfig.LineThickness;
			CapsuleConfig.Duration = DrawDebugConfig.Duration;

			// 加入调试队列
			ABattleFrameBattleControl::GetInstance()->DebugCapsuleQueue.Enqueue(CapsuleConfig);

			// 绘制碰撞点
			FDebugPointConfig PointConfig;
			PointConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			PointConfig.Duration = DrawDebugConfig.Duration;
			PointConfig.Location = VisibilityResult.bBlockingHit ? VisibilityResult.ImpactPoint : VisibilityResult.TraceEnd;
			PointConfig.Size = DrawDebugConfig.HitPointSize;

			ABattleFrameBattleControl::GetInstance()->DebugPointQueue.Enqueue(PointConfig);
		}
	}
}

void UNeighborGridComponent::SectorTraceForSubjects
(
	int32 KeepCount,
	const FVector& Origin,
	const float Radius,
	const float Height,
	const FVector& Direction,
	const float Angle,
	const bool bCheckObstacle,
	const FVector& CheckOrigin,
	const float CheckRadius,
	ESortMode SortMode,
	const FVector& SortOrigin,
	const FSubjectArray& IgnoreSubjects,
	const FBFFilter& Filter,
	const FTraceDrawDebugConfig& DrawDebugConfig,
	bool& Hit,
	TArray<FTraceResult>& Results
) const
{
	Results.Reset();

	// 特殊处理标志
	const bool bFullCircle = FMath::IsNearlyEqual(Angle, 360.0f, KINDA_SMALL_NUMBER);
	const bool bSingleResult = (KeepCount == 1);
	const bool bNoCountLimit = (KeepCount == -1);

	// 将忽略列表转换为集合以便快速查找
	const TSet<FSubjectHandle> IgnoreSet(IgnoreSubjects.Subjects);

	const FVector NormalizedDir = Direction.GetSafeNormal2D();
	const float HalfAngleRad = FMath::DegreesToRadians(Angle * 0.5f);
	const float CosHalfAngle = FMath::Cos(HalfAngleRad);

	// 计算扇形的两个边界方向（如果不是全圆）
	FVector LeftBoundDir, RightBoundDir;

	if (!bFullCircle)
	{
		LeftBoundDir = NormalizedDir.RotateAngleAxis(Angle * 0.5f, FVector::UpVector);
		RightBoundDir = NormalizedDir.RotateAngleAxis(-Angle * 0.5f, FVector::UpVector);
	}

	// 扩展搜索范围 - 使用各轴独立的CellSize
	const FVector CellRadius = CellSize * 0.5f;
	const float ExpandedRadiusXY = Radius + FMath::Max(CellRadius.X, CellRadius.Y) * FMath::Sqrt(2.0f);
	const float ExpandedHeight = Height / 2.0f + CellRadius.Z;
	const FVector Range(ExpandedRadiusXY, ExpandedRadiusXY, ExpandedHeight);

	const FIntVector CoordMin = LocationToCoord(Origin - Range);
	const FIntVector CoordMax = LocationToCoord(Origin + Range);

	// 跟踪最佳结果（用于KeepCount=1的情况）
	FTraceResult BestResult;
	float BestDistSq = (SortMode == ESortMode::NearToFar) ? FLT_MAX : -FLT_MAX;

	// 临时存储所有结果（用于需要排序或随机的情况）
	TArray<FTraceResult> TempResults;

	// 预收集候选格子并按距离排序
	TArray<FIntVector> CandidateCells;

	for (int32 z = CoordMin.Z; z <= CoordMax.Z; ++z)
	{
		for (int32 x = CoordMin.X; x <= CoordMax.X; ++x)
		{
			for (int32 y = CoordMin.Y; y <= CoordMax.Y; ++y)
			{
				const FIntVector Coord(x, y, z);
				if (!IsInside(Coord)) continue;

				const FVector CellCenter = CoordToLocation(Coord);
				const FVector DeltaXY = (CellCenter - Origin) * FVector(1, 1, 0);
				const float DistSqXY = DeltaXY.SizeSquared();

				if (DistSqXY > FMath::Square(ExpandedRadiusXY)) continue;

				const float VerticalDist = FMath::Abs(CellCenter.Z - Origin.Z);
				if (VerticalDist > ExpandedHeight) continue;

				if (!bFullCircle && DistSqXY > SMALL_NUMBER)
				{
					const FVector ToCellDirXY = DeltaXY.GetSafeNormal();
					const float DotProduct = FVector::DotProduct(NormalizedDir, ToCellDirXY);

					if (DotProduct < CosHalfAngle)
					{
						FVector ClosestPoint;
						float DistToLeftBound = FMath::PointDistToLine(CellCenter, Origin, LeftBoundDir, ClosestPoint);

						if (DistToLeftBound >= FMath::Max(CellRadius.X, CellRadius.Y))
						{
							float DistToRightBound = FMath::PointDistToLine(CellCenter, Origin, RightBoundDir, ClosestPoint);
							if (DistToRightBound >= FMath::Max(CellRadius.X, CellRadius.Y))
							{
								const FVector CellMin = CoordToLocation(Coord) - CellRadius;
								const FVector CellMax = CoordToLocation(Coord) + CellRadius;

								if (!(CellMin.X <= Origin.X && Origin.X <= CellMax.X && CellMin.Y <= Origin.Y && Origin.Y <= CellMax.Y)) continue;
							}
						}
					}
				}
				CandidateCells.Add(Coord);
			}
		}
	}

	// 根据SortMode对格子进行排序
	if (SortMode != ESortMode::None)
	{
		CandidateCells.Sort([this, SortOrigin, SortMode](const FIntVector& A, const FIntVector& B) {
			const float DistSqA = FVector::DistSquared(CoordToLocation(A), SortOrigin);
			const float DistSqB = FVector::DistSquared(CoordToLocation(B), SortOrigin);
			return SortMode == ESortMode::NearToFar ? DistSqA < DistSqB : DistSqA > DistSqB;
			});
	}

	// 计算提前终止阈值
	float ThresholdDistanceSq = FLT_MAX;

	if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
	{
		// 计算阈值：当前最远结果的距离 + 2倍格子对角线距离（使用最大轴尺寸）
		const float MaxCellSize = CellSize.GetMax();
		const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
		ThresholdDistanceSq = FMath::Square(ThresholdDistance);
	}

	// 遍历检测
	TArray<FHitResult> VisibilityResults;
	TArray<uint32> SeenHashes;
	SeenHashes.Reserve(32);

	FFilter SubjectFilter;
	SubjectFilter.Include(Filter.IncludeTraits);
	SubjectFilter.Exclude(Filter.ExcludeTraits);

	for (const FIntVector& Coord : CandidateCells)
	{
		// 提前终止检查
		if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None)
		{
			const float CellDistSq = FVector::DistSquared(CoordToLocation(Coord), SortOrigin);
			if ((SortMode == ESortMode::NearToFar && CellDistSq > ThresholdDistanceSq) || (SortMode == ESortMode::FarToNear && CellDistSq < ThresholdDistanceSq)) break;
		}

		const auto& CellData = GetCellAt(SubjectCells, Coord);
		if (!CellData.Fingerprint.Matches(Filter.IncludeTraits)) continue;

		for (const FGridData& SubjectData : CellData.Subjects)
		{
			const FSubjectHandle Subject = SubjectData.SubjectHandle;

			// 有效性检查
			if (UNLIKELY(!Subject.IsValid())) continue;

			// 忽略列表检查
			if (UNLIKELY(IgnoreSet.Contains(Subject))) continue;

			// 高度检查
			const FVector SubjectPos = FVector(SubjectData.Location);
			const float SubjectRadius = SubjectData.Radius;
			const float VerticalDist = FMath::Abs(SubjectPos.Z - Origin.Z);
			if (LIKELY(VerticalDist > (Height / 2.0f + SubjectRadius))) continue;

			// 距离检查
			const FVector DeltaXY = (SubjectPos - Origin) * FVector(1, 1, 0);
			const float DistSqXY = DeltaXY.SizeSquared();
			const float CombinedRadius = Radius + SubjectRadius;
			if (LIKELY(DistSqXY > FMath::Square(CombinedRadius))) continue;

			// 角度检查
			if (!bFullCircle && DistSqXY > SMALL_NUMBER)
			{
				const FVector ToSubjectDirXY = DeltaXY.GetSafeNormal();
				const float DotProduct = FVector::DotProduct(NormalizedDir, ToSubjectDirXY);
				if (DotProduct < CosHalfAngle) continue;
			}

			// 去重
			if (SeenHashes.Contains(SubjectData.SubjectHash)) continue;
			SeenHashes.Add(SubjectData.SubjectHash);

			// 特征检查
			if (!Subject.Matches(SubjectFilter)) continue;

			// 障碍物检查
			const FVector CheckOriginToSubjectDir = (SubjectPos - CheckOrigin).GetSafeNormal();
			const FVector CheckOriginToSubjectSurfacePoint = SubjectPos - (CheckOriginToSubjectDir * SubjectRadius);

			if (bCheckObstacle && !Filter.ObstacleObjectType.IsEmpty())
			{
				FHitResult VisibilityResult;
				bool bVisibilityHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
					GetWorld(),
					CheckOrigin,
					CheckOriginToSubjectSurfacePoint,
					CheckRadius,
					Filter.ObstacleObjectType,
					true,
					TArray<TObjectPtr<AActor>>(),
					EDrawDebugTrace::None,
					VisibilityResult,
					true,
					FLinearColor::Gray,
					FLinearColor::Red,
					1);

				if (DrawDebugConfig.bDrawDebugShape)
				{
					VisibilityResults.Add(VisibilityResult);
				}

				if (bVisibilityHit) continue;
			}

			const float CurrentDistSq = FVector::DistSquared(SortOrigin, SubjectPos);

			const FVector TraceOriginToSubjectDir = (SubjectPos - Origin).GetSafeNormal();
			const FVector TraceHitLocation = SubjectPos - (TraceOriginToSubjectDir * SubjectRadius);

			if (bSingleResult)
			{
				bool bIsBetter = false;

				if (SortMode == ESortMode::NearToFar)
				{
					bIsBetter = (CurrentDistSq < BestDistSq);
				}
				else if (SortMode == ESortMode::FarToNear)
				{
					bIsBetter = (CurrentDistSq > BestDistSq);
				}
				else // ESortMode::None
				{
					bIsBetter = true;
				}

				if (bIsBetter)
				{
					BestResult.Subject = Subject;
					BestResult.SubjectLocation = SubjectPos;
					BestResult.HitLocation = TraceHitLocation;
					BestResult.ShapeLocation = Origin;
					BestResult.CachedDistSq = CurrentDistSq;
					BestDistSq = CurrentDistSq;

					// 更新阈值
					if (!bNoCountLimit && KeepCount > 0)
					{
						const float MaxCellSize = CellSize.GetMax();
						const float ThresholdDistance = FMath::Sqrt(BestDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
						ThresholdDistanceSq = FMath::Square(ThresholdDistance);
					}
				}
			}
			else
			{
				FTraceResult Result;
				Result.Subject = Subject;
				Result.SubjectLocation = SubjectPos;
				Result.HitLocation = TraceHitLocation;
				Result.ShapeLocation = Origin;
				Result.CachedDistSq = CurrentDistSq;
				TempResults.Add(Result);

				// 当收集到足够结果时更新阈值
				if (!bNoCountLimit && KeepCount > 0 && SortMode != ESortMode::None && TempResults.Num() >= KeepCount)
				{
					// 找到当前第KeepCount个最佳结果的距离
					float CurrentThresholdDistSq = 0.0f;

					if (SortMode == ESortMode::NearToFar)
					{
						CurrentThresholdDistSq = TempResults[KeepCount - 1].CachedDistSq;
					}
					else // FarToNear
					{
						CurrentThresholdDistSq = TempResults.Last().CachedDistSq;
					}

					// 更新阈值
					const float MaxCellSize = CellSize.GetMax();
					const float ThresholdDistance = FMath::Sqrt(CurrentThresholdDistSq) + 2.0f * MaxCellSize * FMath::Sqrt(2.0f);
					ThresholdDistanceSq = FMath::Square(ThresholdDistance);
				}
			}
		}
	}

	// 处理结果
	if (bSingleResult)
	{
		if (BestDistSq != FLT_MAX && BestDistSq != -FLT_MAX)
		{
			Results.Add(BestResult);
		}
	}
	else if (TempResults.Num() > 0)
	{
		// 处理排序或随机化
		if (SortMode != ESortMode::None)
		{
			// 应用排序（无论KeepCount是否为-1）
			TempResults.Sort([SortMode](const FTraceResult& A, const FTraceResult& B) {
				return SortMode == ESortMode::NearToFar ?
					A.CachedDistSq < B.CachedDistSq :
					A.CachedDistSq > B.CachedDistSq;
				});
		}
		else
		{
			// 随机打乱结果
			if (TempResults.Num() > 1)
			{
				for (int32 i = TempResults.Num() - 1; i > 0; --i)
				{
					const int32 j = FMath::Rand() % (i + 1);
					TempResults.Swap(i, j);
				}
			}
		}

		// 应用数量限制（只有当KeepCount>0时）
		if (!bNoCountLimit && KeepCount > 0 && TempResults.Num() > KeepCount)
		{
			TempResults.SetNum(KeepCount);
		}

		Results.Append(TempResults);
	}

	Hit = !Results.IsEmpty();

	if (DrawDebugConfig.bDrawDebugShape)
	{
		FDebugSectorConfig SectorConfig;
		SectorConfig.Location = Origin;
		SectorConfig.Radius = Radius;
		SectorConfig.Height = Height;
		SectorConfig.Direction = Direction;
		SectorConfig.Angle = Angle;
		SectorConfig.Duration = DrawDebugConfig.Duration;
		SectorConfig.Color = DrawDebugConfig.Color;
		SectorConfig.LineThickness = DrawDebugConfig.LineThickness;

		ABattleFrameBattleControl::GetInstance()->DebugSectorQueue.Enqueue(SectorConfig);

		// trace results
		for (const auto& Result : Results)
		{
			FDebugSphereConfig SphereConfig;
			SphereConfig.Color = DrawDebugConfig.Color;
			SphereConfig.Location = Result.Subject.GetTrait<FLocated>().Location;
			SphereConfig.Radius = Result.Subject.GetTrait<FGridData>().Radius;
			SphereConfig.Duration = DrawDebugConfig.Duration;
			ABattleFrameBattleControl::GetInstance()->DebugSphereQueue.Enqueue(SphereConfig);
		}

		// visibility check results
		for (const auto& VisibilityResult : VisibilityResults)
		{
			// 计算起点到终点的向量
			FVector VisCheckDirection = VisibilityResult.TraceEnd - VisibilityResult.TraceStart;
			float VisCheckDistance = VisCheckDirection.Size();

			// 处理零距离情况（使用默认旋转）
			FRotator ShapeRot = FRotator::ZeroRotator;

			if (VisCheckDistance > 0)
			{
				VisCheckDirection /= VisCheckDistance;
				ShapeRot = FRotationMatrix::MakeFromZ(VisCheckDirection).Rotator();
			}

			// 计算圆柱部分高度（总高度减去两端的半球）
			float CylinderHeight = FMath::Max(0.0f, VisCheckDistance + 2.0f * CheckRadius);

			// 计算胶囊体中心位置（两点中点）
			FVector ShapeLoc = (VisibilityResult.TraceStart + VisibilityResult.TraceEnd) * 0.5f;

			// 配置调试胶囊体参数
			FDebugCapsuleConfig CapsuleConfig;
			CapsuleConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			CapsuleConfig.Location = ShapeLoc;
			CapsuleConfig.Rotation = ShapeRot;
			CapsuleConfig.Radius = CheckRadius;
			CapsuleConfig.Height = CylinderHeight;
			CapsuleConfig.LineThickness = DrawDebugConfig.LineThickness;
			CapsuleConfig.Duration = DrawDebugConfig.Duration;

			// 加入调试队列
			ABattleFrameBattleControl::GetInstance()->DebugCapsuleQueue.Enqueue(CapsuleConfig);

			// 绘制碰撞点
			FDebugPointConfig PointConfig;
			PointConfig.Color = VisibilityResult.bBlockingHit ? FColor::Red : FColor::Green;
			PointConfig.Duration = DrawDebugConfig.Duration;
			PointConfig.Location = VisibilityResult.bBlockingHit ? VisibilityResult.ImpactPoint : VisibilityResult.TraceEnd;
			PointConfig.Size = DrawDebugConfig.HitPointSize;

			ABattleFrameBattleControl::GetInstance()->DebugPointQueue.Enqueue(PointConfig);
		}
	}
}

// To Do : 1.Sphere Trace For Subjects(can filter by direction angle)  2.Sphere Sweep For Subjects  3.Sphere Sweep For Subjects Async  4.Sector Trace For Subjects  5.Sector Trace For Subjects Async 
// 
// 1. IgnoreList  2.VisibilityCheck  3.AngleCheck  4.KeepCount  5.SortByDist  6.Async


//--------------------------------------------Avoidance---------------------------------------------------------------

void UNeighborGridComponent::Update()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("RVO2 Update");

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("ResetCells");

		ParallelFor(OccupiedCellsQueues.Num(), [&](int32 Index)
		{
			int32 CellIndex;

			while (OccupiedCellsQueues[Index].Dequeue(CellIndex))                            
			{
				auto& SubjectCell = SubjectCells[CellIndex];
				SubjectCell.Empty();

				auto& ObstacleCell = ObstacleCells[CellIndex];
				ObstacleCell.Empty();
			}			
		});
	}

	AMechanism* Mechanism = GetMechanism();

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("RegisterSubject");

		auto Chain = Mechanism->EnchainSolid(RegisterSubjectFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		// 定义注册单元格的lambda函数
		auto RegisterCell = [&](int32 CellIndex, const FGridData& GridData) 
		{
			bool bShouldRegister = false;
			auto& Cell = SubjectCells[CellIndex];

			Cell.Lock();
			if (!Cell.bRegistered) 
			{
				bShouldRegister = true;
				Cell.bRegistered = true;
			}
			Cell.Fingerprint.Add(GridData.SubjectHandle.GetFingerprint());
			Cell.Subjects.Add(GridData);
			Cell.Unlock();

			if (bShouldRegister) 
			{
				OccupiedCellsQueues[CellIndex % MaxThreadsAllowed].Enqueue(CellIndex);
			}
		};

		Chain->OperateConcurrently([&](FSolidSubjectHandle Subject, FLocated& Located, FScaled& Scaled, FCollider& Collider, FGridData& GridData)
		{
			const FVector& Location = Located.Location;

			if (!IsInside(Location)) return;

			if (Subject.HasTrait<FTracing>()) 
			{
				auto& Tracing = Subject.GetTraitRef<FTracing>();
				Tracing.Lock();
				Tracing.NeighborGrid = this;
				Tracing.Unlock();
			}

			GridData.Location = FVector3f(Location);
			GridData.Radius = Collider.Radius * Scaled.Scale;

			// 处理Avoidance逻辑
			if (Subject.HasTrait<FAvoidance>() && Subject.HasTrait<FAvoiding>()) 
			{
				auto& Avoidance = Subject.GetTraitRef<FAvoidance>();
				auto& Avoiding = Subject.GetTraitRef<FAvoiding>();

				Avoiding.Position = RVO::Vector2(Location.X, Location.Y);
				Avoiding.Radius = GridData.Radius * Avoidance.AvoidDistMult;

				if (Subject.HasTrait<FMoving>()) 
				{
					auto& Moving = Subject.GetTraitRef<FMoving>();
					Avoiding.bCanAvoid = !Moving.bLaunching && !Moving.bPushedBack;
				}
			}

			// 使用统一的单元格注册逻辑
			if (!Subject.HasFlag(RegisterMultipleFlag)) 
			{
				RegisterCell(LocationToIndex(Location), GridData);
			}
			else 
			{
				const FVector Range = FVector(GridData.Radius);
				const FIntVector CoordMin = LocationToCoord(Location - Range);
				const FIntVector CoordMax = LocationToCoord(Location + Range);

				for (int32 z = CoordMin.Z; z <= CoordMax.Z; ++z) 
				{
					for (int32 y = CoordMin.Y; y <= CoordMax.Y; ++y) 
					{
						for (int32 x = CoordMin.X; x <= CoordMax.X; ++x) 
						{
							const FIntVector CurrentCoord(x, y, z);

							if (IsInside(CurrentCoord)) 
							{
								RegisterCell(CoordToIndex(CurrentCoord), GridData);
							}
						}
					}
				}
			}

			if (Collider.bDrawDebugShape)
			{
				FDebugSphereConfig Config;
				Config.Radius = GridData.Radius;
				Config.Location = Located.Location;
				Config.Color = FColor::Red;
				Config.LineThickness = 0.f;
				ABattleFrameBattleControl::GetInstance()->DebugSphereQueue.Enqueue(Config);
			}

		}, ThreadsCount, BatchSize);
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("RegisterSphereObstacles");

		auto Chain = Mechanism->EnchainSolid(RegisterSphereObstaclesFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, MinBatchSizeAllowed, ThreadsCount, BatchSize);

		Chain->OperateConcurrently([&](FSolidSubjectHandle Subject, FSphereObstacle& SphereObstacle, FLocated& Located, FCollider& Collider, FAvoidance& Avoidance, FAvoiding& Avoiding, FGridData& GridData)
		{
			if (SphereObstacle.bStatic && SphereObstacle.bRegistered) return; // if static, we only register once

			const auto& Location = Located.Location;

			SphereObstacle.Lock();
			SphereObstacle.NeighborGrid = this;// when sphere obstacle override speed limit, it uses this neighbor grid instance.
			SphereObstacle.Unlock();

			GridData.Location = FVector3f(Location);
			GridData.Radius = Collider.Radius;

			Avoiding.Position = RVO::Vector2(Location.X, Location.Y);
			Avoiding.Radius = Collider.Radius * Avoidance.AvoidDistMult;

			const FVector Range = FVector(Collider.Radius);

			// Compute the range of involved grid cells
			const FIntVector CoordMin = LocationToCoord(Location - Range);
			const FIntVector CoordMax = LocationToCoord(Location + Range);

			for (int32 i = CoordMin.Z; i <= CoordMax.Z; ++i)
			{
				for (int32 j = CoordMin.Y; j <= CoordMax.Y; ++j)
				{
					for (int32 k = CoordMin.X; k <= CoordMax.X; ++k)
					{
						const auto CurrentCoord = FIntVector(k, j, i);

						if (UNLIKELY(!IsInside(CurrentCoord))) continue;

						bool bShouldRegister = false;

						int32 CellIndex = CoordToIndex(CurrentCoord);

						if (!SphereObstacle.bStatic)
						{
							auto& ObstacleCell = ObstacleCells[CellIndex];

							ObstacleCell.Lock();
							ObstacleCell.Subjects.Add(GridData);
							if (!ObstacleCell.bRegistered)
							{
								bShouldRegister = true;
								ObstacleCell.bRegistered = true;
							}
							ObstacleCell.Unlock();
						}
						else
						{
							auto& StaticObstacleCell = StaticObstacleCells[CellIndex];

							StaticObstacleCell.Lock();
							StaticObstacleCell.Subjects.Add(GridData);
							if (!StaticObstacleCell.bRegistered)
							{
								bShouldRegister = true;
								StaticObstacleCell.bRegistered = true;
							}
							StaticObstacleCell.Unlock();
						}

						if (bShouldRegister && !SphereObstacle.bStatic)
						{
							OccupiedCellsQueues[CellIndex % MaxThreadsAllowed].Enqueue(CellIndex);
						}
					}
				}
			}

			SphereObstacle.bRegistered = true;

		}, ThreadsCount, BatchSize);
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("RegisterBoxObstacles");

		auto Chain = Mechanism->EnchainSolid(RegisterBoxObstaclesFilter);
		UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(Chain->IterableNum(), MaxThreadsAllowed, 1, ThreadsCount, BatchSize);

		Chain->OperateConcurrently([&](FSolidSubjectHandle Subject, FBoxObstacle& BoxObstacle, FGridData& GridData)
		{
			if (BoxObstacle.bStatic && BoxObstacle.bRegistered) return; // if static, we only register once

			const auto& Location = FVector(BoxObstacle.point_.x(), BoxObstacle.point_.y(), BoxObstacle.pointZ_);
			GridData.Location = FVector3f(Location);

			const RVO::Vector2& NextLocation = BoxObstacle.nextPoint_;
			const float ObstacleHeight = BoxObstacle.height_;

			const float StartZ = Location.Z;
			const float EndZ = StartZ + ObstacleHeight;

			TSet<FIntVector> AllGridCells;
			TArray<FIntVector> AllGridCellsArray;

			float CurrentLayerZ = StartZ;

			// 使用CellSize.Z作为Z轴步长
			while (CurrentLayerZ < EndZ)
			{
				const FVector LayerCurrentPoint = FVector(Location.X, Location.Y, CurrentLayerZ);
				const FVector LayerNextPoint = FVector(NextLocation.x(), NextLocation.y(), CurrentLayerZ);

				// 使用最大轴尺寸的2倍作为扫描半径
				const float SweepRadius = CellSize.GetMax() * 2.0f;
				auto LayerCells = SphereSweepForCells(LayerCurrentPoint, LayerNextPoint, SweepRadius);

				for (const auto& Coord : LayerCells)
				{
					AllGridCells.Add(Coord);
				}

				CurrentLayerZ += CellSize.Z; // 使用Z轴尺寸作为步长
			}

			AllGridCellsArray = AllGridCells.Array();

			ParallelFor(AllGridCellsArray.Num(), [&](int32 Index)
				{
					const FIntVector& CurrentCoord = AllGridCellsArray[Index];

					if (UNLIKELY(!IsInside(CurrentCoord))) return;

					bool bShouldRegister = false;

					int32 CellIndex = CoordToIndex(CurrentCoord);

					if (!BoxObstacle.bStatic)
					{
						auto& ObstacleCell = ObstacleCells[CellIndex];

						ObstacleCell.Lock();
						ObstacleCell.Subjects.Add(GridData);
						if (!ObstacleCell.bRegistered)
						{
							bShouldRegister = true;
							ObstacleCell.bRegistered = true;
						}
						ObstacleCell.Unlock();
					}
					else
					{
						auto& StaticObstacleCell = StaticObstacleCells[CellIndex];

						StaticObstacleCell.Lock();
						StaticObstacleCell.Subjects.Add(GridData);
						if (!StaticObstacleCell.bRegistered)
						{
							bShouldRegister = true;
							StaticObstacleCell.bRegistered = true;
						}
						StaticObstacleCell.Unlock();
					}

					if (bShouldRegister && !BoxObstacle.bStatic)
					{
						OccupiedCellsQueues[CellIndex % MaxThreadsAllowed].Enqueue(CellIndex);
					}

				});

			BoxObstacle.bRegistered = true;

		}, ThreadsCount, BatchSize);
	}
}

