// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleFrameFunctionLibraryRT.h"
#include "SubjectHandle.h"
#include "SubjectRecord.h"
#include "EngineUtils.h"
#include "Machine.h"
#include "Mechanism.h"
#include "Traits/SubType.h"
#include "BattleFrameBattleControl.h"
#include "AgentConfigDataAsset.h"
#include "ProjectileConfigDataAsset.h"
#include "NeighborGridActor.h"
#include "NeighborGridComponent.h"
#include "Async/Async.h"
#include "Engine/Engine.h"

//---------------------------------Spawning-------------------------------

TArray<FSubjectHandle> UBattleFrameFunctionLibraryRT::SpawnAgentsByConfigRectangular
(
	AAgentSpawner* AgentSpawner,
	bool bAutoActivate,
	TSoftObjectPtr<UAgentConfigDataAsset> DataAsset,
	int32 Quantity,
	int32 Team,
	UPARAM(ref) const FVector& Origin,
	UPARAM(ref) const FVector2D& Region,
	UPARAM(ref) const FVector2D& LaunchVelocity,
	EInitialDirection InitialDirection,
	UPARAM(ref) const FVector2D& CustomDirection,
	UPARAM(ref) const FSpawnerMult& Multipliers
)
{
	// 如果 BattleControl 无效，尝试从 World 查找
	if (!AgentSpawner)
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			for (TActorIterator<AAgentSpawner> It(World); It; ++It)
			{
				AgentSpawner = *It;
				break; // 只取第一个
			}
		}

		if (!AgentSpawner) return TArray<FSubjectHandle>();
	}

	return AgentSpawner->SpawnAgentsByConfigRectangular(bAutoActivate, DataAsset, Quantity, Team, Origin, Region, LaunchVelocity, InitialDirection, CustomDirection, Multipliers);
}


//-------------------------------Apply Dmg and Debuff-------------------------------

void UBattleFrameFunctionLibraryRT::ApplyPointDamageAndDebuff
(
	TArray<FDmgResult>& DamageResults,
	ABattleFrameBattleControl* BattleControl,
	UPARAM(ref) const FSubjectArray& Subjects,
	UPARAM(ref) const FSubjectArray& IgnoreSubjects,
	UPARAM(ref) const FSubjectHandle DmgInstigator,
	UPARAM(ref) const FSubjectHandle DmgCauser,
	UPARAM(ref) const FVector& HitFromLocation,
	UPARAM(ref) const FDamage_Point& Damage,
	UPARAM(ref) const FDebuff_Point& Debuff
)
{
	DamageResults.Reset();

	// 如果 BattleControl 无效，尝试从 World 查找
	if (!IsValid(BattleControl))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			for (TActorIterator<ABattleFrameBattleControl> It(World); It; ++It)
			{
				BattleControl = *It;
				break; // 只取第一个
			}
		}

		if (!IsValid(BattleControl)) return;
	}

	// 直接填充 DamageResults
	BattleControl->ApplyPointDamageAndDebuff(Subjects, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff, DamageResults);
}

void UBattleFrameFunctionLibraryRT::ApplyRadialDamageAndDebuff
(
	TArray<FDmgResult>& DamageResults,
	ABattleFrameBattleControl* BattleControl,
	UNeighborGridComponent* NeighborGridComponent,
	UPARAM(ref) const int32 KeepCount,
	UPARAM(ref) const FVector& Origin,
	UPARAM(ref) const FSubjectArray& IgnoreSubjects,
	UPARAM(ref) const FSubjectHandle DmgInstigator,
	UPARAM(ref) const FSubjectHandle DmgCauser,
	UPARAM(ref) const FVector& HitFromLocation,
	UPARAM(ref) const FDamage_Radial& Damage,
	UPARAM(ref) const FDebuff_Radial& Debuff
)
{
	DamageResults.Reset();

	// 如果 BattleControl 无效，尝试从 World 查找
	if (!IsValid(BattleControl))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			for (TActorIterator<ABattleFrameBattleControl> It(World); It; ++It)
			{
				BattleControl = *It;
				break; // 只取第一个
			}
		}

		if (!IsValid(BattleControl)) return;
	}

	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
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

	BattleControl->ApplyRadialDamageAndDebuff(NeighborGridComponent, KeepCount, Origin, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff, DamageResults);
}

void UBattleFrameFunctionLibraryRT::ApplyBeamDamageAndDebuff
(
	TArray<FDmgResult>& DamageResults, 
	ABattleFrameBattleControl* BattleControl, 
	UNeighborGridComponent* NeighborGridComponent,
	UPARAM(ref) const int32 KeepCount,
	UPARAM(ref) const FVector& StartLocation, 
	UPARAM(ref) const FVector& EndLocation, 
	UPARAM(ref) const FSubjectArray& IgnoreSubjects, 
	UPARAM(ref) const FSubjectHandle DmgInstigator, 
	UPARAM(ref) const FSubjectHandle DmgCauser, 
	UPARAM(ref) const FVector& HitFromLocation, 
	UPARAM(ref) const FDamage_Beam& Damage,
	UPARAM(ref) const FDebuff_Beam& Debuff
)
{
	DamageResults.Reset();

	// 如果 BattleControl 无效，尝试从 World 查找
	if (!IsValid(BattleControl))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			for (TActorIterator<ABattleFrameBattleControl> It(World); It; ++It)
			{
				BattleControl = *It;
				break; // 只取第一个
			}
		}

		if (!IsValid(BattleControl)) return;
	}

	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
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

	BattleControl->ApplyBeamDamageAndDebuff(NeighborGridComponent, KeepCount, StartLocation, EndLocation, IgnoreSubjects, DmgInstigator, DmgCauser, HitFromLocation, Damage, Debuff, DamageResults);
}


//------------------------------Projectile------------------------------

// 解算抛射物发射速度（静态目标）- 给定仰角求速度
void UBattleFrameFunctionLibraryRT::SolveProjectileVelocityFromPitch
(
	bool& Succeed,
	FVector& LaunchVelocity,
	FVector FromPoint, 
	FVector ToPoint, 
	float Gravity, 
	float PitchAngle
)
{
	Gravity *= -1;

	const float aRadians = FMath::DegreesToRadians(PitchAngle);

	// 计算2D距离和高度差
	const float distance2D = FVector::DistXY(FromPoint, ToPoint);
	const float deltaZ = ToPoint.Z - FromPoint.Z;

	// 弹道方程分母
	const float sin2a = FMath::Sin(2 * aRadians);
	const float cos2a = FMath::Cos(aRadians) * FMath::Cos(aRadians);
	const float denominator = (distance2D * sin2a) - (2 * deltaZ * cos2a);

	// 浮点安全检查
	if (FMath::Abs(denominator) < 0)
	{
		Succeed = false;
		return;
	}

	// 计算初速度平方（检查物理有效性）
	const float v0Sq = (distance2D * distance2D * Gravity) / denominator;

	if (v0Sq < 0) // 无实数解
	{
		Succeed = false;
		return;
	}

	const float initialSpeed = FMath::Sqrt(v0Sq);

	// 构建速度向量
	FRotator lookAtRotation = FRotationMatrix::MakeFromX(ToPoint - FromPoint).Rotator();
	lookAtRotation.Pitch = PitchAngle;
	LaunchVelocity = lookAtRotation.Vector() * initialSpeed;

	Succeed = true;
}

// 解算抛射物发射速度（静态目标）- 给定速度求仰角
void UBattleFrameFunctionLibraryRT::SolveProjectileVelocityFromSpeed
(
	bool& Succeed,
	FVector& LaunchVelocity,
	FVector FromPoint,
	FVector ToPoint,
	float Gravity,
	float Speed,
	bool bFavorHighArc
)
{
	Gravity *= -1;

	// 计算2D水平距离和高度差
	const float distance2D = FVector::DistXY(FromPoint, ToPoint);
	const float deltaZ = ToPoint.Z - FromPoint.Z;
	const float SpeedSq = Speed * Speed;

	// 处理水平距离接近0的特殊情况（垂直发射）
	if (distance2D < KINDA_SMALL_NUMBER)
	{
		// 纯垂直弹道处理
		if (deltaZ > 0) // 目标在上方
		{
			// 检查是否满足最小速度要求: v^2 >= 2gh
			if (SpeedSq < 2 * Gravity * deltaZ) 
			{
				Succeed = false; // 速度不足以到达目标高度
				return;
			}
			else
			{
				LaunchVelocity = FVector(0.f, 0.f, Speed); // 垂直向上
				Succeed = true;
			}
		}
		else if (deltaZ < 0) // 目标在下方
		{
			LaunchVelocity = FVector(0.f, 0.f, -Speed); // 垂直向下
			Succeed = true;
		}
		else
		{
			Succeed = false; // 起点终点重合且速度>0，无解
			return;
		}
	}

	// 计算抛射物方程判别式
	// 公式: discriminant = v^4 - g*(g*x^2 + 2*y*v^2)
	const float discriminant = (SpeedSq * SpeedSq) - Gravity * (Gravity * distance2D * distance2D + 2 * deltaZ * SpeedSq);

	// 检查物理有效性（实数解）
	if (discriminant < 0.f) 
	{
		Succeed = false; // 无实数解
		return;
	}

	const float sqrtDiscriminant = FMath::Sqrt(discriminant);
	const float gravityDist = Gravity * distance2D;

	// 计算两个可能发射角的正切值
	const float tanHigh = (SpeedSq + sqrtDiscriminant) / gravityDist; // 高抛解
	const float tanLow = (SpeedSq - sqrtDiscriminant) / gravityDist;  // 低抛解

	// 计算两种解的水平速度分量平方
	const float magXYSqHigh = SpeedSq / (1.f + tanHigh * tanHigh);
	const float magXYSqLow = SpeedSq / (1.f + tanLow * tanLow);

	// 根据bFavorHighArc选择抛射方案
	float selectedTanTheta = bFavorHighArc ?
		(magXYSqHigh < magXYSqLow ? tanHigh : tanLow) : // 高抛：选水平速度小的
		(magXYSqHigh > magXYSqLow ? tanHigh : tanLow);  // 低抛：选水平速度大的

	// 计算最终发射角度（弧度转角度）
	const float pitchRadians = FMath::Atan(selectedTanTheta);
	const float pitchAngle = FMath::RadiansToDegrees(pitchRadians);

	// 构建水平方向旋转器
	FRotator lookAtRotation = FRotationMatrix::MakeFromX(ToPoint - FromPoint).Rotator();
	lookAtRotation.Pitch = pitchAngle; // 应用计算出的仰角

	// 生成速度向量（方向*速度）
	LaunchVelocity = lookAtRotation.Vector() * Speed;

	Succeed = true;
}

// 解算抛射物发射速度（带移动预测）- 给定仰角求速度
void UBattleFrameFunctionLibraryRT::SolveProjectileVelocityFromPitchWithPrediction
(
	bool& Succeed,
	FVector& LaunchVelocity,
	FVector FromPoint,
	FVector ToPoint,
	FVector TargetVelocity,
	int32 Iterations,
	float Gravity,
	float PitchAngle
)
{
	Gravity *= -1;

	const float aRadians = FMath::DegreesToRadians(PitchAngle);
	FVector predictedPosition = ToPoint;  // 使用临时变量
	FVector resultVelocity = FVector::ZeroVector;

	for (int32 i = 0; i < Iterations; ++i)
	{
		// 计算当前预测位置的距离
		const float distance2D = FVector::DistXY(FromPoint, predictedPosition);
		const float deltaZ = predictedPosition.Z - FromPoint.Z;

		// 弹道方程分母
		const float sin2a = FMath::Sin(2 * aRadians);
		const float cos2a = FMath::Cos(aRadians) * FMath::Cos(aRadians);
		const float denominator = (distance2D * sin2a) - (2 * deltaZ * cos2a);

		// 浮点安全检查
		if (FMath::Abs(denominator) < 0)
		{
			Succeed = false;
			return;
		}

		// 计算初速度平方
		const float v0Sq = (distance2D * distance2D * Gravity) / denominator;

		if (v0Sq < 0)
		{
			Succeed = false;
			return;
		}

		const float initialSpeed = FMath::Sqrt(v0Sq);

		// 计算飞行时间 (使用完整分母)
		const float time = distance2D / (FMath::Cos(aRadians) * initialSpeed);

		// 计算下一帧预测位置（保持原始目标位置不变）
		const FVector newPredictedPosition = ToPoint + TargetVelocity * time;

		// 构建当前迭代速度向量
		FRotator lookAtRotation = FRotationMatrix::MakeFromX(predictedPosition - FromPoint).Rotator();
		lookAtRotation.Pitch = PitchAngle;
		LaunchVelocity = lookAtRotation.Vector() * initialSpeed;

		// 更新下次迭代的预测位置
		predictedPosition = newPredictedPosition;
	}

	Succeed = true;
}

// 解算抛射物发射速度（带移动预测）- 给定速度求仰角
void UBattleFrameFunctionLibraryRT::SolveProjectileVelocityFromSpeedWithPrediction
(
	bool& Succeed,
	FVector& LaunchVelocity,
	FVector FromPoint,
	FVector ToPoint,
	FVector TargetVelocity,
	int32 Iterations,
	float Gravity,
	float Speed,
	bool bFavorHighArc
)
{
	Gravity *= -1;

	FVector predictedPosition = ToPoint;  // 初始预测位置

	for (int32 i = 0; i < Iterations; ++i)
	{
		// 计算当前预测位置的水平距离和高度差
		const float distance2D = FVector::DistXY(FromPoint, predictedPosition);
		const float deltaZ = predictedPosition.Z - FromPoint.Z;
		const float SpeedSq = Speed * Speed;

		// 处理水平距离接近0的特殊情况
		if (distance2D < KINDA_SMALL_NUMBER)
		{
			// 垂直发射处理
			if (deltaZ > 0 && SpeedSq >= 2 * Gravity * deltaZ) 
			{
				LaunchVelocity = FVector(0.f, 0.f, Speed);
				Succeed = true;
			}
			else if (deltaZ < 0) 
			{
				LaunchVelocity = FVector(0.f, 0.f, -Speed);
				Succeed = true;
			}
			else
			{
				Succeed = false;
				return;
			}
		}

		// 计算弹道方程的判别式
		const float discriminant = (SpeedSq * SpeedSq) - Gravity * (Gravity * distance2D * distance2D + 2 * deltaZ * SpeedSq);

		// 检查物理有效性
		if (discriminant < 0.f) 
		{
			Succeed = false;
			return;
		}

		const float sqrtDiscriminant = FMath::Sqrt(discriminant);
		const float gravityDist = Gravity * distance2D;

		// 计算两个可能发射角的正切值
		const float tanHigh = (SpeedSq + sqrtDiscriminant) / gravityDist;
		const float tanLow = (SpeedSq - sqrtDiscriminant) / gravityDist;

		// 计算两种解的水平速度分量平方
		const float magXYSqHigh = SpeedSq / (1.f + tanHigh * tanHigh);
		const float magXYSqLow = SpeedSq / (1.f + tanLow * tanLow);

		// 根据bFavorHighArc选择抛射方案
		float selectedTanTheta = bFavorHighArc ?
			(magXYSqHigh < magXYSqLow ? tanHigh : tanLow) :
			(magXYSqHigh > magXYSqLow ? tanHigh : tanLow);

		// 计算最终发射角度（弧度）
		const float pitchRadians = FMath::Atan(selectedTanTheta);

		// 构建水平方向旋转器
		FRotator lookAtRotation = FRotationMatrix::MakeFromX(predictedPosition - FromPoint).Rotator();
		lookAtRotation.Pitch = FMath::RadiansToDegrees(pitchRadians);

		// 生成当前迭代速度向量
		const FVector currentVelocity = lookAtRotation.Vector() * Speed;

		// 计算水平速度分量
		const float horizontalSpeed = FMath::Cos(pitchRadians) * Speed;

		// 计算飞行时间（基于水平距离）
		const float flightTime = distance2D / horizontalSpeed;

		// 更新预测位置（目标当前位置 + 目标速度 * 飞行时间）
		predictedPosition = ToPoint + TargetVelocity * flightTime;

		// 最后一次迭代设置最终发射速度
		if (i == Iterations - 1) 
		{
			LaunchVelocity = currentVelocity;
		}
	}

	Succeed = true;
}

// 计算抛射体在指定时间的位置
void UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Interped
(
	bool& bHasArrived,
	FVector& CurrentLocation,
	FVector FromPoint,
	FVector& ToPoint,
	FSubjectHandle ToTarget,
	FRuntimeFloatCurve XOffset,
	float XOffsetMult,
	FRuntimeFloatCurve YOffset,
	float YOffsetMult,
	FRuntimeFloatCurve ZOffset,
	float ZOffsetMult,
	float InitialTime,
	float CurrentTime,
	float Speed
)
{
	bool bHasValidTarget = ToTarget.IsValid() && ToTarget.HasTrait<FLocated>();
	ToPoint = bHasValidTarget ? ToTarget.GetTrait<FLocated>().Location : ToPoint;

	// 计算到达时间 = 初始时间 + 持续时间
	float Duration = (ToPoint - FromPoint).Size() / Speed;
	float ArrivalTime = InitialTime + Duration;

	// 计算时间进度比例 (0.0~1.0)
	float Alpha = 0.0f;

	if (Duration > 0.0f)
	{
		Alpha = FMath::Clamp((CurrentTime - InitialTime) / Duration, 0.0f, 1.0f);
	}
	else if (CurrentTime >= InitialTime) // 当持续时间为0时的边界处理
	{
		Alpha = 1.0f;
	}

	// 基础位置插值
	FVector LerpPos = FMath::Lerp(FromPoint, ToPoint, Alpha);

	// 计算水平运动方向向量（忽略Z轴）
	FVector HorizontalDir = (ToPoint - FromPoint);
	HorizontalDir.Z = 0;

	FVector RightVector = FVector::ZeroVector;
	FVector ForwardVector = FVector::ZeroVector;

	if (!HorizontalDir.IsNearlyZero(0.001f))
	{
		// 获取水平方向的前后向量和垂直向量
		HorizontalDir.Normalize();
		ForwardVector = HorizontalDir; // 前后方向（Y偏移方向）
		RightVector = FVector(-HorizontalDir.Y, HorizontalDir.X, 0); // 左右方向（X偏移方向）
	}

	// 从曲线获取独立的X/Y偏移值
	float XOffsetValue = XOffset.GetRichCurveConst()->Eval(Alpha) * XOffsetMult; // 左右偏移量
	float YOffsetValue = YOffset.GetRichCurveConst()->Eval(Alpha) * YOffsetMult; // 前后偏移量
	float ZOffsetValue = ZOffset.GetRichCurveConst()->Eval(Alpha) * ZOffsetMult; // 垂直偏移量

	// 计算最终位置：
	// 基础位置 + 左右偏移(X) + 前后偏移(Y) + 垂直偏移(Z)
	CurrentLocation = LerpPos
		+ (RightVector * XOffsetValue)   // 应用X偏移（左右方向）
		+ (ForwardVector * YOffsetValue) // 应用Y偏移（前后方向）
		+ FVector(0, 0, ZOffsetValue);   // 垂直偏移

	// 检测是否到达目标
	bHasArrived = (CurrentLocation - ToPoint).Size() <= KINDA_SMALL_NUMBER;
}

void UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Ballistic
(
	bool& bHasArrived,
	FVector& CurrentLocation,
	FVector FromPoint,
	FVector ToPoint,
	float InitialTime,
	float CurrentTime,
	float Gravity,
	FVector InitialVelocity
)
{
	// 计算经过的时间
	float DeltaTime = CurrentTime - InitialTime;

	// 弹道运动方程计算位移
	FVector Displacement = InitialVelocity * DeltaTime;
	Displacement.Z += 0.5f * Gravity * FMath::Square(DeltaTime);

	CurrentLocation = FromPoint + Displacement;

	// 计算目标向量（起点到终点的方向）
	FVector TargetVector = ToPoint - FromPoint;
	float TargetDistSquared = TargetVector.SizeSquared();

	// 计算当前位置在目标方向上的投影比例
	FVector CurrentVector = CurrentLocation - FromPoint;
	float ProjectionScalar = FVector::DotProduct(CurrentVector, TargetVector) / TargetDistSquared;

	// 当投影比例≥1时（越过目标点）
	if (ProjectionScalar >= 1.0f)
	{
		bHasArrived = true;
	}
	else
	{
		bHasArrived = false;
	}
}

void UBattleFrameFunctionLibraryRT::GetProjectilePositionAtTime_Tracking
(
	bool& bHasArrived,
	FVector& CurrentLocation,
	FVector& CurrentVelocity,
	FVector FromPoint,
	FVector& ToPoint,
	FSubjectHandle ToTarget,
	float Acceleration,
	float MaxSpeed,
	float DeltaTime,
	float ArrivalThreshold // 新增参数: 用于指定距离阈值
)
{
	bHasArrived = false;
	CurrentLocation = FVector::ZeroVector;

	// 获取目标位置和速度（动态目标优先）
	bool bHasValidTarget = ToTarget.IsValid() && ToTarget.HasTrait<FLocated>();
	ToPoint = bHasValidTarget ? ToTarget.GetTrait<FLocated>().Location : ToPoint;
	FVector TargetVelocity = bHasValidTarget && ToTarget.HasTrait<FMoving>() ? ToTarget.GetTrait<FMoving>().CurrentVelocity : FVector::ZeroVector;

	// 处理起点终点重合
	FVector ToFromVector = ToPoint - FromPoint;
	float TargetDistSquared = ToFromVector.SizeSquared();

	if (TargetDistSquared < ArrivalThreshold * ArrivalThreshold) // 使用阈值判断代替 KINDA_SMALL_NUMBER
	{
		CurrentLocation = ToPoint;
		bHasArrived = true;
		return;
	}

	// 计算相对运动参数
	float DistanceToTarget = (ToPoint - FromPoint).Size();
	float Duration = DistanceToTarget / CurrentVelocity.Size();
	FVector PredictedToPoint = ToPoint + TargetVelocity * Duration;
	FVector DirectionToTarget = (PredictedToPoint - FromPoint).GetSafeNormal();

	// 分解当前速度
	FVector VelocityAlongTarget = FVector::DotProduct(CurrentVelocity, DirectionToTarget) * DirectionToTarget;
	FVector VelocityPerpendicular = CurrentVelocity - VelocityAlongTarget;

	// 计算新的加速度以尽快纠正速度方向
	FVector CorrectingAcceleration = -VelocityPerpendicular / Duration;
	FVector NewAcceleration = CorrectingAcceleration + DirectionToTarget * Acceleration;

	// 确保加速度不超过设定的最大加速度
	if (NewAcceleration.Size() > Acceleration) 
	{
		NewAcceleration = NewAcceleration.GetSafeNormal() * Acceleration;
	}

	// 更新速度
	FVector NewVelocity = CurrentVelocity + NewAcceleration * DeltaTime;
	float CurrentSpeed = NewVelocity.Size();

	// 限制最大速度
	if (CurrentSpeed > MaxSpeed)
	{
		NewVelocity = NewVelocity.GetSafeNormal() * MaxSpeed;
	}

	// 计算新位置
	FVector NewPosition = FromPoint + NewVelocity * DeltaTime;

	// 抵达判断
	float RemainingDistanceSquared = (ToPoint - NewPosition).SizeSquared();

	// 更新输出参数
	CurrentVelocity = NewVelocity;
	CurrentLocation = NewPosition;

	// 如果剩余距离小于阈值，则设置已抵达
	bHasArrived = RemainingDistanceSquared <= ArrivalThreshold * ArrivalThreshold;
}

void UBattleFrameFunctionLibraryRT::SpawnProjectileByConfig(bool& Successful, FSubjectHandle& SpawnedProjectile, UProjectileConfigDataAsset* ProjectileConfigDataAsset)
{
	UWorld* World = GEngine->GetCurrentPlayWorld();

	if (!World)
	{
		Successful = false;
		SpawnedProjectile = FSubjectHandle();
		return;
	}

	UProjectileConfigDataAsset* Config = ProjectileConfigDataAsset;

	if (!Config)
	{
		Successful = false;
		SpawnedProjectile = FSubjectHandle();
		return;
	}

	FSubjectRecord Record;
	Record.SetTrait(Config->Projectile);
	Record.SetTrait(Config->SubType);
	Record.SetTrait(Config->Located);
	Record.SetTrait(Config->Directed);
	Record.SetTrait(Config->Scaled);
	Record.SetTrait(Config->ProjectileParams);
	Record.SetTrait(Config->ProjectileParamsRT);
	Record.SetTrait(FIsAttachedFx());

	switch (Config->MovementMode)
	{
		case EProjectileMoveMode::Static:
			Record.SetTrait(Config->ProjectileMove_Static);
			break;
		case EProjectileMoveMode::Interped:
			Record.SetTrait(Config->ProjectileMove_Interped);
			Record.SetTrait(Config->ProjectileMoving_Interped);
			break;
		case EProjectileMoveMode::Ballistic:
			Record.SetTrait(Config->ProjectileMove_Ballistic);
			Record.SetTrait(Config->ProjectileMoving_Ballistic);
			break;
		case EProjectileMoveMode::Tracking:
			Record.SetTrait(Config->ProjectileMove_Tracking);
			Record.SetTrait(Config->ProjectileMoving_Tracking);
			break;
	}

	switch (Config->DamageMode)
	{
		case EProjectileDamageMode::Point:
			Record.SetTrait(Config->Damage_Point);
			Record.SetTrait(Config->Debuff_Point);
			break;
		case EProjectileDamageMode::Radial:
			Record.SetTrait(Config->Damage_Radial);
			Record.SetTrait(Config->Debuff_Radial);
			break;
		case EProjectileDamageMode::Beam:
			Record.SetTrait(Config->Damage_Beam);
			Record.SetTrait(Config->Debuff_Beam);
			break;
	}

	SetRecordSubTypeTraitByIndex(Config->SubType.Index,Record);

	AMechanism* Mechanism = UMachine::ObtainMechanism(World);

	if (!Mechanism)
	{
		Successful = false;
		SpawnedProjectile = FSubjectHandle();
		return;
	}

	SpawnedProjectile = Mechanism->SpawnSubject(Record);

	if (SpawnedProjectile.IsValid())
	{
		Successful = true;
	}
	else
	{
		Successful = false;
	}
}

// Static Movement 
void UBattleFrameFunctionLibraryRT::SpawnProjectile_Static(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects)
{
	UProjectileConfigDataAsset* Config = ProjectileConfigDataAsset;

	if (!IsValid(Config))
	{
		Successful = false;
		return;
	}

	if (Config->MovementMode != EProjectileMoveMode::Static)
	{
		Successful = false;
		return;
	}

	SpawnProjectileByConfig(Successful, ProjectileHandle, Config);

	if (!Successful) return;

	FProjectileParamsRT ProjectileParamsRT;
	ProjectileParamsRT.IgnoreSubjects = IgnoreSubjects;
	ProjectileParamsRT.Instigator = Instigator;
	ProjectileParamsRT.NeighborGridComponent = NeighborGridComponent;

	FLocated Located;
	Located.Location = FromPoint;
	Located.PreLocation = FromPoint;

	FScaled Scaled;
	Scaled.RenderScale *= ScaleMult;

	ProjectileHandle.SetTrait(Located);
	ProjectileHandle.SetTrait(Scaled);
	ProjectileHandle.SetTrait(ProjectileParamsRT);
	ProjectileHandle.SetTrait(FActivated());
}

// Interped Movement
void UBattleFrameFunctionLibraryRT::SpawnProjectile_Interped(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, float XOffsetMult, float YOffsetMult, float ZOffsetMult, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects)
{
	UProjectileConfigDataAsset* Config = ProjectileConfigDataAsset;

	if (!IsValid(Config))
	{
		Successful = false;
		return;
	}

	if (Config->MovementMode != EProjectileMoveMode::Interped)
	{
		Successful = false;
		return;
	}

	SpawnProjectileByConfig(Successful, ProjectileHandle, Config);

	if (!Successful) return;

	bool bHasValidTarget = ToTarget.IsValid() && ToTarget.HasTrait<FLocated>();
	ToPoint = bHasValidTarget ? ToTarget.GetTrait<FLocated>().Location : ToPoint;

	FProjectileParamsRT ProjectileParamsRT;
	ProjectileParamsRT.IgnoreSubjects = IgnoreSubjects;
	ProjectileParamsRT.Instigator = Instigator;
	ProjectileParamsRT.NeighborGridComponent = NeighborGridComponent;

	FProjectileMoving_Interped ProjectileMoving;
	ProjectileMoving.BirthTime = GEngine->GetCurrentPlayWorld()->GetTimeSeconds();
	ProjectileMoving.FromPoint = FromPoint;
	ProjectileMoving.ToPoint = ToPoint;
	ProjectileMoving.Target = ToTarget;

	FProjectileMove_Interped ProjectileMove;

	if (ProjectileHandle.HasTrait<FProjectileMove_Interped>())
	{
		ProjectileMove = ProjectileHandle.GetTrait<FProjectileMove_Interped>();
	}

	float Distance = FVector::Dist(FromPoint, ToPoint);
	ProjectileMoving.XOffsetMult = XOffsetMult * FMath::GetMappedRangeValueClamped(TRange<float>(ProjectileMove.XOffsetRangeMap[0], ProjectileMove.XOffsetRangeMap[2]), TRange<float>(ProjectileMove.XOffsetRangeMap[1], ProjectileMove.XOffsetRangeMap[3]), Distance);
	ProjectileMoving.YOffsetMult = YOffsetMult * FMath::GetMappedRangeValueClamped(TRange<float>(ProjectileMove.YOffsetRangeMap[0], ProjectileMove.YOffsetRangeMap[2]), TRange<float>(ProjectileMove.YOffsetRangeMap[1], ProjectileMove.YOffsetRangeMap[3]), Distance);
	ProjectileMoving.ZOffsetMult = ZOffsetMult * FMath::GetMappedRangeValueClamped(TRange<float>(ProjectileMove.ZOffsetRangeMap[0], ProjectileMove.ZOffsetRangeMap[2]), TRange<float>(ProjectileMove.ZOffsetRangeMap[1], ProjectileMove.ZOffsetRangeMap[3]), Distance);

	FLocated Located;
	Located.Location = FromPoint;
	Located.PreLocation = FromPoint;

	FDirected Directed;
	Directed.Direction = (ToPoint - FromPoint).GetSafeNormal();
	Directed.DesiredDirection = Directed.Direction;

	FScaled Scaled;
	Scaled.RenderScale *= ScaleMult;

	ProjectileHandle.SetTrait(Located);
	ProjectileHandle.SetTrait(Directed);
	ProjectileHandle.SetTrait(Scaled);
	ProjectileHandle.SetTrait(ProjectileParamsRT);
	ProjectileHandle.SetTrait(ProjectileMoving);
	ProjectileHandle.SetTrait(FActivated());
}

// Ballistic Movement
void UBattleFrameFunctionLibraryRT::SpawnProjectile_Ballistic(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, FVector TargetVelocity, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects)
{
	UProjectileConfigDataAsset* Config = ProjectileConfigDataAsset;

	if (!IsValid(Config))
	{
		Successful = false;
		return;
	}

	if (Config->MovementMode != EProjectileMoveMode::Ballistic)
	{
		Successful = false;
		return;
	}

	bool bHasValidTarget = ToTarget.IsValid() && ToTarget.HasTrait<FLocated>();
	ToPoint = bHasValidTarget ? ToTarget.GetTrait<FLocated>().Location : ToPoint;

	FVector InitialVelocity;

	switch (Config->ProjectileMove_Ballistic.SolveMode)
	{
	case EProjectileSolveMode::FromSpeed:
		SolveProjectileVelocityFromSpeedWithPrediction(Successful, InitialVelocity, FromPoint, ToPoint, TargetVelocity, Config->ProjectileMove_Ballistic.Iterations, Config->ProjectileMove_Ballistic.Gravity, Config->ProjectileMove_Ballistic.Speed, Config->ProjectileMove_Ballistic.bFavorHighArc);
		break;

	case EProjectileSolveMode::FromPitch:
		SolveProjectileVelocityFromPitchWithPrediction(Successful, InitialVelocity, FromPoint, ToPoint, TargetVelocity, Config->ProjectileMove_Ballistic.Iterations, Config->ProjectileMove_Ballistic.Gravity, Config->ProjectileMove_Ballistic.Pitch);
		break;
	}

	if (!Successful) return;

	SpawnProjectileByConfig(Successful, ProjectileHandle, Config);

	if (!Successful) return;

	FProjectileParamsRT ProjectileParamsRT;
	ProjectileParamsRT.IgnoreSubjects = IgnoreSubjects;
	ProjectileParamsRT.Instigator = Instigator;
	ProjectileParamsRT.NeighborGridComponent = NeighborGridComponent;

	FProjectileMoving_Ballistic ProjectileMoving;
	ProjectileMoving.BirthTime = GEngine->GetCurrentPlayWorld()->GetTimeSeconds();
	ProjectileMoving.FromPoint = FromPoint;
	ProjectileMoving.ToPoint = ToPoint;
	ProjectileMoving.InitialVelocity = InitialVelocity;

	FLocated Located;
	Located.Location = FromPoint;
	Located.PreLocation = FromPoint;

	FDirected Directed;
	Directed.Direction = InitialVelocity.GetSafeNormal();
	Directed.DesiredDirection = Directed.Direction;

	FScaled Scaled;
	Scaled.RenderScale *= ScaleMult;

	ProjectileHandle.SetTrait(Located);
	ProjectileHandle.SetTrait(Directed);
	ProjectileHandle.SetTrait(Scaled);
	ProjectileHandle.SetTrait(ProjectileParamsRT);
	ProjectileHandle.SetTrait(ProjectileMoving);
	ProjectileHandle.SetTrait(FActivated());
}

// Tracking Movement
void UBattleFrameFunctionLibraryRT::SpawnProjectile_Tracking(bool& Successful, FSubjectHandle& ProjectileHandle, UNeighborGridComponent* NeighborGridComponent, UProjectileConfigDataAsset* ProjectileConfigDataAsset, FVector ScaleMult, FVector FromPoint, FVector ToPoint, FSubjectHandle ToTarget, FVector Direction, FSubjectHandle Instigator, FSubjectArray IgnoreSubjects)
{
	UProjectileConfigDataAsset* Config = ProjectileConfigDataAsset;

	if (!IsValid(Config))
	{
		Successful = false;
		return;
	}

	if (Config->MovementMode != EProjectileMoveMode::Tracking)
	{
		Successful = false;
		return;
	}

	SpawnProjectileByConfig(Successful, ProjectileHandle, Config);

	if (!Successful) return;

	bool bHasValidTarget = ToTarget.IsValid() && ToTarget.HasTrait<FLocated>();

	ToPoint = bHasValidTarget ? ToTarget.GetTrait<FLocated>().Location : ToPoint;
	FVector TargetV = bHasValidTarget && ToTarget.HasTrait<FMoving>() ? ToTarget.GetTrait<FMoving>().CurrentVelocity : FVector::ZeroVector;

	FProjectileParamsRT ProjectileParamsRT;
	ProjectileParamsRT.IgnoreSubjects = IgnoreSubjects;
	ProjectileParamsRT.Instigator = Instigator;
	ProjectileParamsRT.NeighborGridComponent = NeighborGridComponent;

	FProjectileMoving_Tracking ProjectileMoving;
	ProjectileMoving.FromPoint = FromPoint;
	ProjectileMoving.ToPoint = ToPoint;
	ProjectileMoving.Target = ToTarget;
	ProjectileMoving.TargetVelocity = TargetV;
	ProjectileMoving.CurrentVelocity = Direction * Config->ProjectileMove_Tracking.Speed;

	FLocated Located;
	Located.Location = FromPoint;
	Located.PreLocation = FromPoint;

	FDirected Directed;
	Directed.Direction = Direction;
	Directed.DesiredDirection = Direction;

	FScaled Scaled;
	Scaled.RenderScale *= ScaleMult;

	ProjectileHandle.SetTrait(Located);
	ProjectileHandle.SetTrait(Directed);
	ProjectileHandle.SetTrait(Scaled);
	ProjectileHandle.SetTrait(ProjectileParamsRT);
	ProjectileHandle.SetTrait(ProjectileMoving);
	ProjectileHandle.SetTrait(FActivated());
}


//-------------------------------Sync Traces-------------------------------

void UBattleFrameFunctionLibraryRT::SphereTraceForSubjects
(
	bool& Hit,
	TArray<FTraceResult>& TraceResults,
	UNeighborGridComponent* NeighborGridComponent,
	int32 KeepCount,
	UPARAM(ref) const FVector& TraceOrigin,
	float Radius,
	bool bCheckObstacle,
	UPARAM(ref) const FVector& CheckOrigin,
	float CheckRadius,
	ESortMode SortMode,
	UPARAM(ref) const FVector& SortOrigin,
	UPARAM(ref) const FSubjectArray& IgnoreSubjects,
	UPARAM(ref) const FBFFilter& Filter,
	UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig
)
{
	TraceResults.Reset();

	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
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

	NeighborGridComponent->SphereTraceForSubjects(KeepCount, TraceOrigin, Radius, bCheckObstacle, CheckOrigin, CheckRadius, SortMode, SortOrigin, IgnoreSubjects, Filter, DrawDebugConfig, Hit, TraceResults);
}

void UBattleFrameFunctionLibraryRT::SphereSweepForSubjects
(
	bool& Hit,
	TArray<FTraceResult>& TraceResults,
	UNeighborGridComponent* NeighborGridComponent,
	int32 KeepCount,
	UPARAM(ref) const FVector& TraceStart,
	UPARAM(ref) const FVector& TraceEnd,
	float Radius,
	bool bCheckObstacle,
	UPARAM(ref) const FVector& CheckOrigin,
	float CheckRadius,
	ESortMode SortMode,
	UPARAM(ref) const FVector& SortOrigin,
	UPARAM(ref) const FSubjectArray& IgnoreSubjects,
	UPARAM(ref) const FBFFilter& Filter,
	UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig
)
{
	TraceResults.Reset();

	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
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
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("CallSphereSweepForSubjects");
	NeighborGridComponent->SphereSweepForSubjects(KeepCount, TraceStart, TraceEnd, Radius, bCheckObstacle, CheckOrigin, CheckRadius, SortMode, SortOrigin, IgnoreSubjects, Filter, DrawDebugConfig, Hit, TraceResults);
}

void UBattleFrameFunctionLibraryRT::SectorTraceForSubjects
(
	bool& Hit,
	TArray<FTraceResult>& TraceResults,
	UNeighborGridComponent* NeighborGridComponent,
	int32 KeepCount,
	UPARAM(ref) const FVector& TraceOrigin,
	float Radius,
	float Height,
	UPARAM(ref) const FVector& ForwardVector,
	float Angle,
	bool bCheckObstacle,
	UPARAM(ref) const FVector& CheckOrigin,
	float CheckRadius,
	ESortMode SortMode,
	UPARAM(ref) const FVector& SortOrigin,
	UPARAM(ref) const FSubjectArray& IgnoreSubjects,
	UPARAM(ref) const FBFFilter& Filter,
	UPARAM(ref) const FTraceDrawDebugConfig& DrawDebugConfig
)
{
	TraceResults.Reset();

	if (!IsValid(NeighborGridComponent))
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
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

	NeighborGridComponent->SectorTraceForSubjects(KeepCount, TraceOrigin, Radius, Height, ForwardVector, Angle, bCheckObstacle, CheckOrigin, CheckRadius, SortMode, SortOrigin, IgnoreSubjects, Filter, DrawDebugConfig, Hit, TraceResults);
}


//-------------------------------Async Trace-------------------------------

USphereSweepForSubjectsAsyncAction* USphereSweepForSubjectsAsyncAction::SphereSweepForSubjectsAsync
(
	const UObject* WorldContextObject,
	UNeighborGridComponent* NeighborGridComponent,
	int32 KeepCount,
	const FVector TraceStart,
	const FVector TraceEnd,
	float Radius,
	bool bCheckObstacle,
	const FVector CheckOrigin,
	float CheckRadius,
	ESortMode SortMode,
	const FVector SortOrigin,
	const FSubjectArray IgnoreSubjects,
	const FBFFilter DmgFilter,
	const FTraceDrawDebugConfig DrawDebugConfig
)
{
	if (!WorldContextObject) return nullptr;

	bool bHasValidNeighborGrid = false;

	if (!IsValid(NeighborGridComponent))
	{
		for (TActorIterator<ANeighborGridActor> It(WorldContextObject->GetWorld()); It; ++It)
		{
			if (*It)
			{
				ANeighborGridActor* NeighborGridActor = *It;
				NeighborGridComponent = NeighborGridActor->GetComponentByClass<UNeighborGridComponent>();

				if (NeighborGridComponent)
				{
					bHasValidNeighborGrid = true;
					break;
				}
			}
		}

		if (!IsValid(NeighborGridComponent)) return nullptr;
	}

	USphereSweepForSubjectsAsyncAction* AsyncAction = NewObject<USphereSweepForSubjectsAsyncAction>();

	AsyncAction->CurrentWorld = WorldContextObject->GetWorld();
	AsyncAction->NeighborGrid = NeighborGridComponent;
	AsyncAction->KeepCount = KeepCount;
	AsyncAction->Start = TraceStart;
	AsyncAction->End = TraceEnd;
	AsyncAction->Radius = Radius;
	AsyncAction->bCheckObstacle = bCheckObstacle;
	AsyncAction->CheckOrigin = CheckOrigin;
	AsyncAction->CheckRadius = CheckRadius;
	AsyncAction->SortMode = SortMode;
	AsyncAction->SortOrigin = SortOrigin;
	AsyncAction->IgnoreSubjects = IgnoreSubjects;
	AsyncAction->Filter = DmgFilter;
	AsyncAction->DrawDebugConfig = DrawDebugConfig;

	AsyncAction->RegisterWithGameInstance(AsyncAction->CurrentWorld);

	return AsyncAction;
}

void USphereSweepForSubjectsAsyncAction::Activate()
{
	if (!IsValid(NeighborGrid.Get()))
	{
		Hit = false;
		Completed.Broadcast(Hit, Results);
		SetReadyToDestroy();
	}
	else
	{
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
			{
				TArray<FIntVector> CellCoords = NeighborGrid->SphereSweepForCells(Start, End, Radius);

				for (const FIntVector& CellCoord : CellCoords)
				{
					ValidCells.Add(NeighborGrid->GetCellAt(NeighborGrid->SubjectCells, CellCoord));
				}

				const FVector TraceDir = (End - Start).GetSafeNormal();
				const float TraceLength = FVector::Distance(Start, End);

				// 创建忽略列表的哈希集合以便快速查找
				TSet<FSubjectHandle> IgnoreSet;

				for (const FSubjectHandle Subject : IgnoreSubjects.Subjects)
				{
					IgnoreSet.Add(Subject);
				}

				SubjectFilter.Include(Filter.IncludeTraits);
				SubjectFilter.Exclude(Filter.ExcludeTraits);

				// 检查每个单元中的subject
				for (const auto& CageCell : ValidCells)
				{
					if (!CageCell.Fingerprint.Matches(Filter.IncludeTraits)) continue;

					for (const FGridData& Data : CageCell.Subjects)
					{
						const FSubjectHandle Subject = Data.SubjectHandle;

						// 检查是否在忽略列表中
						if (IgnoreSet.Contains(Subject)) continue;

						const FVector SubjectPos = FVector(Data.Location);
						float SubjectRadius = Data.Radius;

						// 距离计算
						const FVector ToSubject = SubjectPos - Start;
						const float ProjOnTrace = FVector::DotProduct(ToSubject, TraceDir);

						// 初步筛选
						const float ProjThreshold = SubjectRadius + Radius;
						if (ProjOnTrace < -ProjThreshold || ProjOnTrace > TraceLength + ProjThreshold) continue;

						// 精确距离检查
						const float ClampedProj = FMath::Clamp(ProjOnTrace, 0.0f, TraceLength);
						const FVector NearestPoint = Start + ClampedProj * TraceDir;
						const float CombinedRadSq = FMath::Square(Radius + SubjectRadius);

						if (FVector::DistSquared(NearestPoint, SubjectPos) >= CombinedRadSq) continue;

						// 障碍物检查
						const FVector CheckOriginToSubjectDir = (SubjectPos - CheckOrigin).GetSafeNormal();
						const FVector CheckOriginToSubjectSurfacePoint = SubjectPos - (CheckOriginToSubjectDir * SubjectRadius);

						if (bCheckObstacle && !Filter.ObstacleObjectType.IsEmpty())
						{
							FHitResult VisibilityResult;
							bool bVisibilityHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
								CurrentWorld,
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

						const FVector TraceClosestPointOnLine = FMath::ClosestPointOnLine(Start, End, SubjectPos);
						const FVector TraceClosestPointToSubjectDir = (SubjectPos - TraceClosestPointOnLine).GetSafeNormal();
						const FVector TraceHitLocation = SubjectPos - (TraceClosestPointToSubjectDir * SubjectRadius);
						const FVector TraceShapeLocation = TraceHitLocation - (TraceClosestPointToSubjectDir * Radius);

						// 创建FTraceResult并添加到结果数组
						FTraceResult Result;
						Result.Subject = Subject;
						Result.SubjectLocation = SubjectPos;
						Result.HitLocation = TraceHitLocation;
						Result.ShapeLocation = TraceShapeLocation;
						Result.CachedDistSq = FVector::DistSquared(SortOrigin, SubjectPos);
						TempResults.Add(Result);
					}
				}

				// 排序逻辑
				if (SortMode != ESortMode::None)
				{
					TempResults.Sort([this](const FTraceResult& A, const FTraceResult& B)
						{
							if (SortMode == ESortMode::NearToFar)
							{
								return A.CachedDistSq < B.CachedDistSq;
							}
							else // FarToNear
							{
								return A.CachedDistSq > B.CachedDistSq;
							}
						});
				}

				AsyncTask(ENamedThreads::GameThread, [this]()
					{
						Results.Reset();

						int32 ValidCount = 0;
						const bool bRequireLimit = (KeepCount > 0);

						// 按预排序顺序遍历，遇到有效项立即收集
						for (const FTraceResult& TempResult : TempResults)
						{
							if (!TempResult.Subject.Matches(SubjectFilter)) continue;// this can only run on gamethread

							Results.Add(TempResult);
							ValidCount++;

							// 达到数量限制立即终止
							if (bRequireLimit && ValidCount >= KeepCount) break;
						}

						Hit = !Results.IsEmpty();

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

						Completed.Broadcast(Hit, Results);
						SetReadyToDestroy();
					});
			});
	}
}

//-----------------------------------Misc-------------------------------

void UBattleFrameFunctionLibraryRT::SortSubjectsByDistance(UPARAM(ref) TArray<FTraceResult>& TraceResults, const FVector& SortOrigin, ESortMode SortMode)
{
	// 1. 首先移除所有无效的Subject
	TraceResults.RemoveAll([](const FTraceResult& TraceResult)
		{
			return !TraceResult.Subject.IsValid();
		});

	// 2. 检查剩余元素数量
	if (TraceResults.Num() <= 1)
	{
		return; // 不需要排序
	}

	// 3. 预计算距离（可选优化）
	for (auto& Result : TraceResults)
	{
		Result.CachedDistSq = FVector::DistSquared(SortOrigin, Result.SubjectLocation);
	}

	// 4. 根据模式排序
	TraceResults.Sort([SortMode](const FTraceResult& A, const FTraceResult& B)
		{
			if (SortMode == ESortMode::NearToFar)
			{
				return A.CachedDistSq < B.CachedDistSq;  // 从近到远
			}
			else
			{
				return A.CachedDistSq > B.CachedDistSq;  // 从远到近
			}
		});
}

void UBattleFrameFunctionLibraryRT::CalculateThreadsCountAndBatchSize(int32 IterableNum, int32 MaxThreadsAllowed, int32 MinBatchSizeAllowed, int32& ThreadsCount, int32& BatchSize)
{
	// 计算最大可能线程数（考虑最小批次限制）
	const int32 MaxPossibleThreads = FMath::Clamp(IterableNum / FMath::Max(1, MinBatchSizeAllowed), 1, MaxThreadsAllowed);

	// 最终确定使用的线程数
	ThreadsCount = FMath::Clamp(MaxPossibleThreads, 1, MaxThreadsAllowed);

	// 计算批次大小（使用向上取整算法解决余数问题）
	BatchSize = IterableNum / ThreadsCount;

	if (IterableNum % ThreadsCount != 0)
	{
		BatchSize += 1;
	}

	// 最终限制批次大小范围
	BatchSize = FMath::Clamp(BatchSize, 1, FLT_MAX);
}


//-------------------------------Connector Nodes-------------------------------

TArray<FSubjectHandle> UBattleFrameFunctionLibraryRT::ConvertDmgResultsToSubjectHandles(const TArray<FDmgResult>& DmgResults)
{
	TArray<FSubjectHandle> SubjectHandles;
	SubjectHandles.Reserve(DmgResults.Num() * 2); // Reserve space for both Damaged and Instigator subjects

	for (const FDmgResult& Result : DmgResults)
	{
		// Add both DamagedSubject and InstigatorSubject to the array
		SubjectHandles.Add(Result.DamagedSubject);

		// Only add Instigator if it's valid (not empty)
		if (Result.InstigatorSubject.IsValid())
		{
			SubjectHandles.Add(Result.InstigatorSubject);
		}
	}

	return SubjectHandles;
}

TArray<FSubjectHandle> UBattleFrameFunctionLibraryRT::ConvertTraceResultsToSubjectHandles(const TArray<FTraceResult>& TraceResults)
{
	TArray<FSubjectHandle> SubjectHandles;
	SubjectHandles.Reserve(TraceResults.Num());

	for (const FTraceResult& Result : TraceResults)
	{
		SubjectHandles.Add(Result.Subject);
	}

	return SubjectHandles;
}

FSubjectArray UBattleFrameFunctionLibraryRT::ConvertDmgResultsToSubjectArray(const TArray<FDmgResult>& DmgResults)
{
	FSubjectArray SubjectArray;
	SubjectArray.Subjects.Reserve(DmgResults.Num());

	for (const FDmgResult& Result : DmgResults)
	{
		SubjectArray.Subjects.Add(Result.DamagedSubject);
	}

	return SubjectArray;
}

FSubjectArray UBattleFrameFunctionLibraryRT::ConvertTraceResultsToSubjectArray(const TArray<FTraceResult>& TraceResults)
{
	FSubjectArray SubjectArray;
	SubjectArray.Subjects.Reserve(TraceResults.Num());

	for (const FTraceResult& Result : TraceResults)
	{
		SubjectArray.Subjects.Add(Result.Subject);
	}

	return SubjectArray;
}

FSubjectArray UBattleFrameFunctionLibraryRT::ConvertSubjectHandlesToSubjectArray(const TArray<FSubjectHandle>& SubjectHandles)
{
	FSubjectArray SubjectArray;
	SubjectArray.Subjects = SubjectHandles;

	return SubjectArray;
}


//-------------------------------Trait Setters-------------------------------

void UBattleFrameFunctionLibraryRT::SetRecordSubTypeTraitByIndex(int32 Index, FSubjectRecord& SubjectRecord)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("SetRecordSubTypeTraitByIndex");
	switch (Index)
	{
	default:
		SubjectRecord.SetTrait(FSubType0());
		break;
	case 0:
		SubjectRecord.SetTrait(FSubType0());
		break;
	case 1:
		SubjectRecord.SetTrait(FSubType1());
		break;
	case 2:
		SubjectRecord.SetTrait(FSubType2());
		break;
	case 3:
		SubjectRecord.SetTrait(FSubType3());
		break;
	case 4:
		SubjectRecord.SetTrait(FSubType4());
		break;
	case 5:
		SubjectRecord.SetTrait(FSubType5());
		break;
	case 6:
		SubjectRecord.SetTrait(FSubType6());
		break;
	case 7:
		SubjectRecord.SetTrait(FSubType7());
		break;
	case 8:
		SubjectRecord.SetTrait(FSubType8());
		break;
	case 9:
		SubjectRecord.SetTrait(FSubType9());
		break;
	case 10:
		SubjectRecord.SetTrait(FSubType10());
		break;
	case 11:
		SubjectRecord.SetTrait(FSubType11());
		break;
	case 12:
		SubjectRecord.SetTrait(FSubType12());
		break;
	case 13:
		SubjectRecord.SetTrait(FSubType13());
		break;
	case 14:
		SubjectRecord.SetTrait(FSubType14());
		break;
	case 15:
		SubjectRecord.SetTrait(FSubType15());
		break;
	case 16:
		SubjectRecord.SetTrait(FSubType16());
		break;
	case 17:
		SubjectRecord.SetTrait(FSubType17());
		break;
	case 18:
		SubjectRecord.SetTrait(FSubType18());
		break;
	case 19:
		SubjectRecord.SetTrait(FSubType19());
		break;
	case 20:
		SubjectRecord.SetTrait(FSubType20());
		break;
	case 21:
		SubjectRecord.SetTrait(FSubType21());
		break;
	case 22:
		SubjectRecord.SetTrait(FSubType22());
		break;
	case 23:
		SubjectRecord.SetTrait(FSubType23());
		break;
	case 24:
		SubjectRecord.SetTrait(FSubType24());
		break;
	case 25:
		SubjectRecord.SetTrait(FSubType25());
		break;
	case 26:
		SubjectRecord.SetTrait(FSubType26());
		break;
	case 27:
		SubjectRecord.SetTrait(FSubType27());
		break;
	case 28:
		SubjectRecord.SetTrait(FSubType28());
		break;
	case 29:
		SubjectRecord.SetTrait(FSubType29());
		break;
	case 30:
		SubjectRecord.SetTrait(FSubType30());
		break;
	case 31:
		SubjectRecord.SetTrait(FSubType31());
		break;
	case 32:
		SubjectRecord.SetTrait(FSubType32());
		break;
	case 33:
		SubjectRecord.SetTrait(FSubType33());
		break;
	case 34:
		SubjectRecord.SetTrait(FSubType34());
		break;
	case 35:
		SubjectRecord.SetTrait(FSubType35());
		break;
	case 36:
		SubjectRecord.SetTrait(FSubType36());
		break;
	case 37:
		SubjectRecord.SetTrait(FSubType37());
		break;
	case 38:
		SubjectRecord.SetTrait(FSubType38());
		break;
	case 39:
		SubjectRecord.SetTrait(FSubType39());
		break;

	case 40:
		SubjectRecord.SetTrait(FSubType40());
		break;

	case 41:
		SubjectRecord.SetTrait(FSubType41());
		break;

	case 42:
		SubjectRecord.SetTrait(FSubType42());
		break;

	case 43:
		SubjectRecord.SetTrait(FSubType43());
		break;

	case 44:
		SubjectRecord.SetTrait(FSubType44());
		break;

	case 45:
		SubjectRecord.SetTrait(FSubType45());
		break;

	case 46:
		SubjectRecord.SetTrait(FSubType46());
		break;

	case 47:
		SubjectRecord.SetTrait(FSubType47());
		break;

	case 48:
		SubjectRecord.SetTrait(FSubType48());
		break;

	case 49:
		SubjectRecord.SetTrait(FSubType49());
		break;

	case 50:
		SubjectRecord.SetTrait(FSubType50());
		break;

	case 51:
		SubjectRecord.SetTrait(FSubType51());
		break;

	case 52:
		SubjectRecord.SetTrait(FSubType52());
		break;

	case 53:
		SubjectRecord.SetTrait(FSubType53());
		break;

	case 54:
		SubjectRecord.SetTrait(FSubType54());
		break;

	case 55:
		SubjectRecord.SetTrait(FSubType55());
		break;

	case 56:
		SubjectRecord.SetTrait(FSubType56());
		break;

	case 57:
		SubjectRecord.SetTrait(FSubType57());
		break;

	case 58:
		SubjectRecord.SetTrait(FSubType58());
		break;

	case 59:
		SubjectRecord.SetTrait(FSubType59());
		break;

	case 60:
		SubjectRecord.SetTrait(FSubType60());
		break;

	case 61:
		SubjectRecord.SetTrait(FSubType61());
		break;

	case 62:
		SubjectRecord.SetTrait(FSubType62());
		break;

	case 63:
		SubjectRecord.SetTrait(FSubType63());
		break;

	case 64:
		SubjectRecord.SetTrait(FSubType64());
		break;

	case 65:
		SubjectRecord.SetTrait(FSubType65());
		break;

	case 66:
		SubjectRecord.SetTrait(FSubType66());
		break;

	case 67:
		SubjectRecord.SetTrait(FSubType67());
		break;

	case 68:
		SubjectRecord.SetTrait(FSubType68());
		break;

	case 69:
		SubjectRecord.SetTrait(FSubType69());
		break;

	case 70:
		SubjectRecord.SetTrait(FSubType70());
		break;

	case 71:
		SubjectRecord.SetTrait(FSubType71());
		break;

	case 72:
		SubjectRecord.SetTrait(FSubType72());
		break;

	case 73:
		SubjectRecord.SetTrait(FSubType73());
		break;

	case 74:
		SubjectRecord.SetTrait(FSubType74());
		break;

	case 75:
		SubjectRecord.SetTrait(FSubType75());
		break;

	case 76:
		SubjectRecord.SetTrait(FSubType76());
		break;

	case 77:
		SubjectRecord.SetTrait(FSubType77());
		break;

	case 78:
		SubjectRecord.SetTrait(FSubType78());
		break;

	case 79:
		SubjectRecord.SetTrait(FSubType79());
		break;

	case 80:
		SubjectRecord.SetTrait(FSubType80());
		break;

	case 81:
		SubjectRecord.SetTrait(FSubType81());
		break;
	}
}

void UBattleFrameFunctionLibraryRT::SetRecordSubTypeTraitByEnum(EESubType SubType, FSubjectRecord& SubjectRecord)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("SetRecordSubTypeTraitByEnum");
	switch (SubType)
	{
	default:
		break;

	case EESubType::None:
		break;

	case EESubType::SubType0:
		SubjectRecord.SetTrait(FSubType0());
		break;
	case EESubType::SubType1:
		SubjectRecord.SetTrait(FSubType1());
		break;
	case EESubType::SubType2:
		SubjectRecord.SetTrait(FSubType2());
		break;
	case EESubType::SubType3:
		SubjectRecord.SetTrait(FSubType3());
		break;
	case EESubType::SubType4:
		SubjectRecord.SetTrait(FSubType4());
		break;
	case EESubType::SubType5:
		SubjectRecord.SetTrait(FSubType5());
		break;
	case EESubType::SubType6:
		SubjectRecord.SetTrait(FSubType6());
		break;
	case EESubType::SubType7:
		SubjectRecord.SetTrait(FSubType7());
		break;
	case EESubType::SubType8:
		SubjectRecord.SetTrait(FSubType8());
		break;
	case EESubType::SubType9:
		SubjectRecord.SetTrait(FSubType9());
		break;
	case EESubType::SubType10:
		SubjectRecord.SetTrait(FSubType10());
		break;
	case EESubType::SubType11:
		SubjectRecord.SetTrait(FSubType11());
		break;
	case EESubType::SubType12:
		SubjectRecord.SetTrait(FSubType12());
		break;
	case EESubType::SubType13:
		SubjectRecord.SetTrait(FSubType13());
		break;
	case EESubType::SubType14:
		SubjectRecord.SetTrait(FSubType14());
		break;
	case EESubType::SubType15:
		SubjectRecord.SetTrait(FSubType15());
		break;
	case EESubType::SubType16:
		SubjectRecord.SetTrait(FSubType16());
		break;
	case EESubType::SubType17:
		SubjectRecord.SetTrait(FSubType17());
		break;
	case EESubType::SubType18:
		SubjectRecord.SetTrait(FSubType18());
		break;
	case EESubType::SubType19:
		SubjectRecord.SetTrait(FSubType19());
		break;
	case EESubType::SubType20:
		SubjectRecord.SetTrait(FSubType20());
		break;
	case EESubType::SubType21:
		SubjectRecord.SetTrait(FSubType21());
		break;
	case EESubType::SubType22:
		SubjectRecord.SetTrait(FSubType22());
		break;
	case EESubType::SubType23:
		SubjectRecord.SetTrait(FSubType23());
		break;
	case EESubType::SubType24:
		SubjectRecord.SetTrait(FSubType24());
		break;
	case EESubType::SubType25:
		SubjectRecord.SetTrait(FSubType25());
		break;
	case EESubType::SubType26:
		SubjectRecord.SetTrait(FSubType26());
		break;
	case EESubType::SubType27:
		SubjectRecord.SetTrait(FSubType27());
		break;
	case EESubType::SubType28:
		SubjectRecord.SetTrait(FSubType28());
		break;
	case EESubType::SubType29:
		SubjectRecord.SetTrait(FSubType29());
		break;
	case EESubType::SubType30:
		SubjectRecord.SetTrait(FSubType30());
		break;
	case EESubType::SubType31:
		SubjectRecord.SetTrait(FSubType31());
		break;
	case EESubType::SubType32:
		SubjectRecord.SetTrait(FSubType32());
		break;
	case EESubType::SubType33:
		SubjectRecord.SetTrait(FSubType33());
		break;
	case EESubType::SubType34:
		SubjectRecord.SetTrait(FSubType34());
		break;
	case EESubType::SubType35:
		SubjectRecord.SetTrait(FSubType35());
		break;
	case EESubType::SubType36:
		SubjectRecord.SetTrait(FSubType36());
		break;
	case EESubType::SubType37:
		SubjectRecord.SetTrait(FSubType37());
		break;
	case EESubType::SubType38:
		SubjectRecord.SetTrait(FSubType38());
		break;
	case EESubType::SubType39:
		SubjectRecord.SetTrait(FSubType39());
		break;
	case EESubType::SubType40:
		SubjectRecord.SetTrait(FSubType40());
		break;

	case EESubType::SubType41:
		SubjectRecord.SetTrait(FSubType41());
		break;

	case EESubType::SubType42:
		SubjectRecord.SetTrait(FSubType42());
		break;

	case EESubType::SubType43:
		SubjectRecord.SetTrait(FSubType43());
		break;

	case EESubType::SubType44:
		SubjectRecord.SetTrait(FSubType44());
		break;

	case EESubType::SubType45:
		SubjectRecord.SetTrait(FSubType45());
		break;

	case EESubType::SubType46:
		SubjectRecord.SetTrait(FSubType46());
		break;

	case EESubType::SubType47:
		SubjectRecord.SetTrait(FSubType47());
		break;

	case EESubType::SubType48:
		SubjectRecord.SetTrait(FSubType48());
		break;

	case EESubType::SubType49:
		SubjectRecord.SetTrait(FSubType49());
		break;

	case EESubType::SubType50:
		SubjectRecord.SetTrait(FSubType50());
		break;

	case EESubType::SubType51:
		SubjectRecord.SetTrait(FSubType51());
		break;

	case EESubType::SubType52:
		SubjectRecord.SetTrait(FSubType52());
		break;

	case EESubType::SubType53:
		SubjectRecord.SetTrait(FSubType53());
		break;

	case EESubType::SubType54:
		SubjectRecord.SetTrait(FSubType54());
		break;

	case EESubType::SubType55:
		SubjectRecord.SetTrait(FSubType55());
		break;

	case EESubType::SubType56:
		SubjectRecord.SetTrait(FSubType56());
		break;

	case EESubType::SubType57:
		SubjectRecord.SetTrait(FSubType57());
		break;

	case EESubType::SubType58:
		SubjectRecord.SetTrait(FSubType58());
		break;

	case EESubType::SubType59:
		SubjectRecord.SetTrait(FSubType59());
		break;

	case EESubType::SubType60:
		SubjectRecord.SetTrait(FSubType60());
		break;

	case EESubType::SubType61:
		SubjectRecord.SetTrait(FSubType61());
		break;

	case EESubType::SubType62:
		SubjectRecord.SetTrait(FSubType62());
		break;

	case EESubType::SubType63:
		SubjectRecord.SetTrait(FSubType63());
		break;

	case EESubType::SubType64:
		SubjectRecord.SetTrait(FSubType64());
		break;

	case EESubType::SubType65:
		SubjectRecord.SetTrait(FSubType65());
		break;

	case EESubType::SubType66:
		SubjectRecord.SetTrait(FSubType66());
		break;

	case EESubType::SubType67:
		SubjectRecord.SetTrait(FSubType67());
		break;

	case EESubType::SubType68:
		SubjectRecord.SetTrait(FSubType68());
		break;

	case EESubType::SubType69:
		SubjectRecord.SetTrait(FSubType69());
		break;

	case EESubType::SubType70:
		SubjectRecord.SetTrait(FSubType70());
		break;

	case EESubType::SubType71:
		SubjectRecord.SetTrait(FSubType71());
		break;

	case EESubType::SubType72:
		SubjectRecord.SetTrait(FSubType72());
		break;

	case EESubType::SubType73:
		SubjectRecord.SetTrait(FSubType73());
		break;

	case EESubType::SubType74:
		SubjectRecord.SetTrait(FSubType74());
		break;

	case EESubType::SubType75:
		SubjectRecord.SetTrait(FSubType75());
		break;

	case EESubType::SubType76:
		SubjectRecord.SetTrait(FSubType76());
		break;

	case EESubType::SubType77:
		SubjectRecord.SetTrait(FSubType77());
		break;

	case EESubType::SubType78:
		SubjectRecord.SetTrait(FSubType78());
		break;

	case EESubType::SubType79:
		SubjectRecord.SetTrait(FSubType79());
		break;

	case EESubType::SubType80:
		SubjectRecord.SetTrait(FSubType80());
		break;

	case EESubType::SubType81:
		SubjectRecord.SetTrait(FSubType81());
		break;
	}
}

void UBattleFrameFunctionLibraryRT::RemoveSubjectSubTypeTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("SetRecordSubTypeTraitByIndex");
	switch (Index)
	{
	default:
		SubjectHandle.RemoveTrait<FSubType0>();
		break;

	case 0:
		SubjectHandle.RemoveTrait<FSubType0>();
		break;

	case 1:
		SubjectHandle.RemoveTrait<FSubType1>();
		break;

	case 2:
		SubjectHandle.RemoveTrait<FSubType2>();
		break;

	case 3:
		SubjectHandle.RemoveTrait<FSubType3>();
		break;

	case 4:
		SubjectHandle.RemoveTrait<FSubType4>();
		break;

	case 5:
		SubjectHandle.RemoveTrait<FSubType5>();
		break;

	case 6:
		SubjectHandle.RemoveTrait<FSubType6>();
		break;

	case 7:
		SubjectHandle.RemoveTrait<FSubType7>();
		break;

	case 8:
		SubjectHandle.RemoveTrait<FSubType8>();
		break;

	case 9:
		SubjectHandle.RemoveTrait<FSubType9>();
		break;

	case 10:
		SubjectHandle.RemoveTrait<FSubType10>();
		break;

	case 11:
		SubjectHandle.RemoveTrait<FSubType11>();
		break;

	case 12:
		SubjectHandle.RemoveTrait<FSubType12>();
		break;

	case 13:
		SubjectHandle.RemoveTrait<FSubType13>();
		break;

	case 14:
		SubjectHandle.RemoveTrait<FSubType14>();
		break;

	case 15:
		SubjectHandle.RemoveTrait<FSubType15>();
		break;

	case 16:
		SubjectHandle.RemoveTrait<FSubType16>();
		break;

	case 17:
		SubjectHandle.RemoveTrait<FSubType17>();
		break;

	case 18:
		SubjectHandle.RemoveTrait<FSubType18>();
		break;

	case 19:
		SubjectHandle.RemoveTrait<FSubType19>();
		break;

	case 20:
		SubjectHandle.RemoveTrait<FSubType20>();
		break;

	case 21:
		SubjectHandle.RemoveTrait<FSubType21>();
		break;

	case 22:
		SubjectHandle.RemoveTrait<FSubType22>();
		break;

	case 23:
		SubjectHandle.RemoveTrait<FSubType23>();
		break;

	case 24:
		SubjectHandle.RemoveTrait<FSubType24>();
		break;

	case 25:
		SubjectHandle.RemoveTrait<FSubType25>();
		break;

	case 26:
		SubjectHandle.RemoveTrait<FSubType26>();
		break;

	case 27:
		SubjectHandle.RemoveTrait<FSubType27>();
		break;

	case 28:
		SubjectHandle.RemoveTrait<FSubType28>();
		break;

	case 29:
		SubjectHandle.RemoveTrait<FSubType29>();
		break;

	case 30:
		SubjectHandle.RemoveTrait<FSubType30>();
		break;

	case 31:
		SubjectHandle.RemoveTrait<FSubType31>();
		break;

	case 32:
		SubjectHandle.RemoveTrait<FSubType32>();
		break;

	case 33:
		SubjectHandle.RemoveTrait<FSubType33>();
		break;

	case 34:
		SubjectHandle.RemoveTrait<FSubType34>();
		break;

	case 35:
		SubjectHandle.RemoveTrait<FSubType35>();
		break;

	case 36:
		SubjectHandle.RemoveTrait<FSubType36>();
		break;

	case 37:
		SubjectHandle.RemoveTrait<FSubType37>();
		break;

	case 38:
		SubjectHandle.RemoveTrait<FSubType38>();
		break;

	case 39:
		SubjectHandle.RemoveTrait<FSubType39>();
		break;

	case 40:
		SubjectHandle.RemoveTrait<FSubType40>();
		break;

	case 41:
		SubjectHandle.RemoveTrait<FSubType41>();
		break;

	case 42:
		SubjectHandle.RemoveTrait<FSubType42>();
		break;

	case 43:
		SubjectHandle.RemoveTrait<FSubType43>();
		break;

	case 44:
		SubjectHandle.RemoveTrait<FSubType44>();
		break;

	case 45:
		SubjectHandle.RemoveTrait<FSubType45>();
		break;

	case 46:
		SubjectHandle.RemoveTrait<FSubType46>();
		break;

	case 47:
		SubjectHandle.RemoveTrait<FSubType47>();
		break;

	case 48:
		SubjectHandle.RemoveTrait<FSubType48>();
		break;

	case 49:
		SubjectHandle.RemoveTrait<FSubType49>();
		break;

	case 50:
		SubjectHandle.RemoveTrait<FSubType50>();
		break;

	case 51:
		SubjectHandle.RemoveTrait<FSubType51>();
		break;

	case 52:
		SubjectHandle.RemoveTrait<FSubType52>();
		break;

	case 53:
		SubjectHandle.RemoveTrait<FSubType53>();
		break;

	case 54:
		SubjectHandle.RemoveTrait<FSubType54>();
		break;

	case 55:
		SubjectHandle.RemoveTrait<FSubType55>();
		break;

	case 56:
		SubjectHandle.RemoveTrait<FSubType56>();
		break;

	case 57:
		SubjectHandle.RemoveTrait<FSubType57>();
		break;

	case 58:
		SubjectHandle.RemoveTrait<FSubType58>();
		break;

	case 59:
		SubjectHandle.RemoveTrait<FSubType59>();
		break;

	case 60:
		SubjectHandle.RemoveTrait<FSubType60>();
		break;

	case 61:
		SubjectHandle.RemoveTrait<FSubType61>();
		break;

	case 62:
		SubjectHandle.RemoveTrait<FSubType62>();
		break;

	case 63:
		SubjectHandle.RemoveTrait<FSubType63>();
		break;

	case 64:
		SubjectHandle.RemoveTrait<FSubType64>();
		break;

	case 65:
		SubjectHandle.RemoveTrait<FSubType65>();
		break;

	case 66:
		SubjectHandle.RemoveTrait<FSubType66>();
		break;

	case 67:
		SubjectHandle.RemoveTrait<FSubType67>();
		break;

	case 68:
		SubjectHandle.RemoveTrait<FSubType68>();
		break;

	case 69:
		SubjectHandle.RemoveTrait<FSubType69>();
		break;

	case 70:
		SubjectHandle.RemoveTrait<FSubType70>();
		break;

	case 71:
		SubjectHandle.RemoveTrait<FSubType71>();
		break;

	case 72:
		SubjectHandle.RemoveTrait<FSubType72>();
		break;

	case 73:
		SubjectHandle.RemoveTrait<FSubType73>();
		break;

	case 74:
		SubjectHandle.RemoveTrait<FSubType74>();
		break;

	case 75:
		SubjectHandle.RemoveTrait<FSubType75>();
		break;

	case 76:
		SubjectHandle.RemoveTrait<FSubType76>();
		break;

	case 77:
		SubjectHandle.RemoveTrait<FSubType77>();
		break;

	case 78:
		SubjectHandle.RemoveTrait<FSubType78>();
		break;

	case 79:
		SubjectHandle.RemoveTrait<FSubType79>();
		break;

	case 80:
		SubjectHandle.RemoveTrait<FSubType80>();
		break;

	case 81:
		SubjectHandle.RemoveTrait<FSubType81>();
		break;
	}
}

void UBattleFrameFunctionLibraryRT::SetSubjectSubTypeTraitByIndex(int32 Index, FSubjectHandle SubjectHandle)
{
	//TRACE_CPUPROFILER_EVENT_SCOPE_STR("SetRecordSubTypeTraitByIndex");
	switch (Index)
	{
	default:
		SubjectHandle.SetTrait(FSubType0());
		break;

	case 0:
		SubjectHandle.SetTrait(FSubType0());
		break;

	case 1:
		SubjectHandle.SetTrait(FSubType1());
		break;

	case 2:
		SubjectHandle.SetTrait(FSubType2());
		break;

	case 3:
		SubjectHandle.SetTrait(FSubType3());
		break;

	case 4:
		SubjectHandle.SetTrait(FSubType4());
		break;

	case 5:
		SubjectHandle.SetTrait(FSubType5());
		break;

	case 6:
		SubjectHandle.SetTrait(FSubType6());
		break;

	case 7:
		SubjectHandle.SetTrait(FSubType7());
		break;

	case 8:
		SubjectHandle.SetTrait(FSubType8());
		break;

	case 9:
		SubjectHandle.SetTrait(FSubType9());
		break;

	case 10:
		SubjectHandle.SetTrait(FSubType10());
		break;

	case 11:
		SubjectHandle.SetTrait(FSubType11());
		break;

	case 12:
		SubjectHandle.SetTrait(FSubType12());
		break;

	case 13:
		SubjectHandle.SetTrait(FSubType13());
		break;

	case 14:
		SubjectHandle.SetTrait(FSubType14());
		break;

	case 15:
		SubjectHandle.SetTrait(FSubType15());
		break;

	case 16:
		SubjectHandle.SetTrait(FSubType16());
		break;

	case 17:
		SubjectHandle.SetTrait(FSubType17());
		break;

	case 18:
		SubjectHandle.SetTrait(FSubType18());
		break;

	case 19:
		SubjectHandle.SetTrait(FSubType19());
		break;

	case 20:
		SubjectHandle.SetTrait(FSubType20());
		break;

	case 21:
		SubjectHandle.SetTrait(FSubType21());
		break;

	case 22:
		SubjectHandle.SetTrait(FSubType22());
		break;

	case 23:
		SubjectHandle.SetTrait(FSubType23());
		break;

	case 24:
		SubjectHandle.SetTrait(FSubType24());
		break;

	case 25:
		SubjectHandle.SetTrait(FSubType25());
		break;

	case 26:
		SubjectHandle.SetTrait(FSubType26());
		break;

	case 27:
		SubjectHandle.SetTrait(FSubType27());
		break;

	case 28:
		SubjectHandle.SetTrait(FSubType28());
		break;

	case 29:
		SubjectHandle.SetTrait(FSubType29());
		break;

	case 30:
		SubjectHandle.SetTrait(FSubType30());
		break;

	case 31:
		SubjectHandle.SetTrait(FSubType31());
		break;

	case 32:
		SubjectHandle.SetTrait(FSubType32());
		break;

	case 33:
		SubjectHandle.SetTrait(FSubType33());
		break;

	case 34:
		SubjectHandle.SetTrait(FSubType34());
		break;

	case 35:
		SubjectHandle.SetTrait(FSubType35());
		break;

	case 36:
		SubjectHandle.SetTrait(FSubType36());
		break;

	case 37:
		SubjectHandle.SetTrait(FSubType37());
		break;

	case 38:
		SubjectHandle.SetTrait(FSubType38());
		break;

	case 39:
		SubjectHandle.SetTrait(FSubType39());
		break;

	case 40:
		SubjectHandle.SetTrait(FSubType40());
		break;

	case 41:
		SubjectHandle.SetTrait(FSubType41());
		break;

	case 42:
		SubjectHandle.SetTrait(FSubType42());
		break;

	case 43:
		SubjectHandle.SetTrait(FSubType43());
		break;

	case 44:
		SubjectHandle.SetTrait(FSubType44());
		break;

	case 45:
		SubjectHandle.SetTrait(FSubType45());
		break;

	case 46:
		SubjectHandle.SetTrait(FSubType46());
		break;

	case 47:
		SubjectHandle.SetTrait(FSubType47());
		break;

	case 48:
		SubjectHandle.SetTrait(FSubType48());
		break;

	case 49:
		SubjectHandle.SetTrait(FSubType49());
		break;

	case 50:
		SubjectHandle.SetTrait(FSubType50());
		break;

	case 51:
		SubjectHandle.SetTrait(FSubType51());
		break;

	case 52:
		SubjectHandle.SetTrait(FSubType52());
		break;

	case 53:
		SubjectHandle.SetTrait(FSubType53());
		break;

	case 54:
		SubjectHandle.SetTrait(FSubType54());
		break;

	case 55:
		SubjectHandle.SetTrait(FSubType55());
		break;

	case 56:
		SubjectHandle.SetTrait(FSubType56());
		break;

	case 57:
		SubjectHandle.SetTrait(FSubType57());
		break;

	case 58:
		SubjectHandle.SetTrait(FSubType58());
		break;

	case 59:
		SubjectHandle.SetTrait(FSubType59());
		break;

	case 60:
		SubjectHandle.SetTrait(FSubType60());
		break;

	case 61:
		SubjectHandle.SetTrait(FSubType61());
		break;

	case 62:
		SubjectHandle.SetTrait(FSubType62());
		break;

	case 63:
		SubjectHandle.SetTrait(FSubType63());
		break;

	case 64:
		SubjectHandle.SetTrait(FSubType64());
		break;

	case 65:
		SubjectHandle.SetTrait(FSubType65());
		break;

	case 66:
		SubjectHandle.SetTrait(FSubType66());
		break;

	case 67:
		SubjectHandle.SetTrait(FSubType67());
		break;

	case 68:
		SubjectHandle.SetTrait(FSubType68());
		break;

	case 69:
		SubjectHandle.SetTrait(FSubType69());
		break;

	case 70:
		SubjectHandle.SetTrait(FSubType70());
		break;

	case 71:
		SubjectHandle.SetTrait(FSubType71());
		break;

	case 72:
		SubjectHandle.SetTrait(FSubType72());
		break;

	case 73:
		SubjectHandle.SetTrait(FSubType73());
		break;

	case 74:
		SubjectHandle.SetTrait(FSubType74());
		break;

	case 75:
		SubjectHandle.SetTrait(FSubType75());
		break;

	case 76:
		SubjectHandle.SetTrait(FSubType76());
		break;

	case 77:
		SubjectHandle.SetTrait(FSubType77());
		break;

	case 78:
		SubjectHandle.SetTrait(FSubType78());
		break;

	case 79:
		SubjectHandle.SetTrait(FSubType79());
		break;

	case 80:
		SubjectHandle.SetTrait(FSubType80());
		break;

	case 81:
		SubjectHandle.SetTrait(FSubType81());
		break;
	}
}

void UBattleFrameFunctionLibraryRT::IncludeSubTypeTraitByIndex(int32 Index, FFilter& Filter)
{
	switch (Index)
	{
	case 0:
		Filter.Include<FSubType0>();
		break;

	case 1:
		Filter.Include<FSubType1>();
		break;

	case 2:
		Filter.Include<FSubType2>();
		break;

	case 3:
		Filter.Include<FSubType3>();
		break;

	case 4:
		Filter.Include<FSubType4>();
		break;

	case 5:
		Filter.Include<FSubType5>();
		break;

	case 6:
		Filter.Include<FSubType6>();
		break;

	case 7:
		Filter.Include<FSubType7>();
		break;

	case 8:
		Filter.Include<FSubType8>();
		break;

	case 9:
		Filter.Include<FSubType9>();
		break;

	case 10:
		Filter.Include<FSubType10>();
		break;

	case 11:
		Filter.Include<FSubType11>();
		break;

	case 12:
		Filter.Include<FSubType12>();
		break;

	case 13:
		Filter.Include<FSubType13>();
		break;

	case 14:
		Filter.Include<FSubType14>();
		break;

	case 15:
		Filter.Include<FSubType15>();
		break;

	case 16:
		Filter.Include<FSubType16>();
		break;

	case 17:
		Filter.Include<FSubType17>();
		break;

	case 18:
		Filter.Include<FSubType18>();
		break;

	case 19:
		Filter.Include<FSubType19>();
		break;

	case 20:
		Filter.Include<FSubType20>();
		break;

	case 21:
		Filter.Include<FSubType21>();
		break;

	case 22:
		Filter.Include<FSubType22>();
		break;

	case 23:
		Filter.Include<FSubType23>();
		break;

	case 24:
		Filter.Include<FSubType24>();
		break;

	case 25:
		Filter.Include<FSubType25>();
		break;

	case 26:
		Filter.Include<FSubType26>();
		break;

	case 27:
		Filter.Include<FSubType27>();
		break;

	case 28:
		Filter.Include<FSubType28>();
		break;

	case 29:
		Filter.Include<FSubType29>();
		break;

	case 30:
		Filter.Include<FSubType30>();
		break;

	case 31:
		Filter.Include<FSubType31>();
		break;

	case 32:
		Filter.Include<FSubType32>();
		break;

	case 33:
		Filter.Include<FSubType33>();
		break;

	case 34:
		Filter.Include<FSubType34>();
		break;

	case 35:
		Filter.Include<FSubType35>();
		break;

	case 36:
		Filter.Include<FSubType36>();
		break;

	case 37:
		Filter.Include<FSubType37>();
		break;

	case 38:
		Filter.Include<FSubType38>();
		break;

	case 39:
		Filter.Include<FSubType39>();
		break;

	case 40:
		Filter.Include<FSubType40>();
		break;

	case 41:
		Filter.Include<FSubType41>();
		break;

	case 42:
		Filter.Include<FSubType42>();
		break;

	case 43:
		Filter.Include<FSubType43>();
		break;

	case 44:
		Filter.Include<FSubType44>();
		break;

	case 45:
		Filter.Include<FSubType45>();
		break;

	case 46:
		Filter.Include<FSubType46>();
		break;

	case 47:
		Filter.Include<FSubType47>();
		break;

	case 48:
		Filter.Include<FSubType48>();
		break;

	case 49:
		Filter.Include<FSubType49>();
		break;

	case 50:
		Filter.Include<FSubType50>();
		break;

	case 51:
		Filter.Include<FSubType51>();
		break;

	case 52:
		Filter.Include<FSubType52>();
		break;

	case 53:
		Filter.Include<FSubType53>();
		break;

	case 54:
		Filter.Include<FSubType54>();
		break;

	case 55:
		Filter.Include<FSubType55>();
		break;

	case 56:
		Filter.Include<FSubType56>();
		break;

	case 57:
		Filter.Include<FSubType57>();
		break;

	case 58:
		Filter.Include<FSubType58>();
		break;

	case 59:
		Filter.Include<FSubType59>();
		break;

	case 60:
		Filter.Include<FSubType60>();
		break;

	case 61:
		Filter.Include<FSubType61>();
		break;

	case 62:
		Filter.Include<FSubType62>();
		break;

	case 63:
		Filter.Include<FSubType63>();
		break;

	case 64:
		Filter.Include<FSubType64>();
		break;

	case 65:
		Filter.Include<FSubType65>();
		break;

	case 66:
		Filter.Include<FSubType66>();
		break;

	case 67:
		Filter.Include<FSubType67>();
		break;

	case 68:
		Filter.Include<FSubType68>();
		break;

	case 69:
		Filter.Include<FSubType69>();
		break;

	case 70:
		Filter.Include<FSubType70>();
		break;

	case 71:
		Filter.Include<FSubType71>();
		break;

	case 72:
		Filter.Include<FSubType72>();
		break;

	case 73:
		Filter.Include<FSubType73>();
		break;

	case 74:
		Filter.Include<FSubType74>();
		break;

	case 75:
		Filter.Include<FSubType75>();
		break;

	case 76:
		Filter.Include<FSubType76>();
		break;

	case 77:
		Filter.Include<FSubType77>();
		break;

	case 78:
		Filter.Include<FSubType78>();
		break;

	case 79:
		Filter.Include<FSubType79>();
		break;

	case 80:
		Filter.Include<FSubType80>();
		break;

	case 81:
		Filter.Include<FSubType81>();
		break;

	default:
		Filter.Include<FSubType0>();
		break;
	}
}