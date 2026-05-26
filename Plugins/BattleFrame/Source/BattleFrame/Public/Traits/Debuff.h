#pragma once

#include "CoreMinimal.h"
#include "Debuff.generated.h" 

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FLaunchParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否可以将目标击退"))
	bool bCanLaunch = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击退的速度,分为XY与Z"))
	FVector2D LaunchSpeed = FVector2D(2000, 0);

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FTemporalDmgParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否可以造成延时伤害"))
	bool bDealTemporalDmg = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "总延时伤害"))
	float TemporalDmg = 60.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "分段数量"))
	int32 TemporalDmgSegment = 6;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "伤害间隔"))
	float TemporalDmgInterval = 0.5f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FSlowParams
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否可以使目标减速"))
	bool bCanSlow = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速的持续时间（秒）"))
	float SlowTime = 4.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速的强度"))
	float SlowStrength = 1.f;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebuff
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用减益"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击退"))
	FLaunchParams LaunchParams = FLaunchParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时伤害"))
	FTemporalDmgParams TemporalDmgParams = FTemporalDmgParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速"))
	FSlowParams SlowParams = FSlowParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用距离衰减，目前为线性衰减"))
	bool bUseFalloff = false;
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebuff_Point
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用减益"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击退"))
	FLaunchParams LaunchParams = FLaunchParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时伤害"))
	FTemporalDmgParams TemporalDmgParams = FTemporalDmgParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速"))
	FSlowParams SlowParams = FSlowParams();

	// 默认构造函数
	FDebuff_Point() = default;

	// 从FDebuff构造
	FDebuff_Point(const FDebuff& InDebuff)
	{
		bEnable = InDebuff.bEnable;
		LaunchParams = InDebuff.LaunchParams;
		TemporalDmgParams = InDebuff.TemporalDmgParams;
		SlowParams = InDebuff.SlowParams;
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebuff_Radial
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用减益"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击退"))
	FLaunchParams LaunchParams = FLaunchParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时伤害"))
	FTemporalDmgParams TemporalDmgParams = FTemporalDmgParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速"))
	FSlowParams SlowParams = FSlowParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用距离衰减，目前为线性衰减"))
	bool bUseFalloff = false;

	// 默认构造函数
	FDebuff_Radial() = default;

	// 从FDebuff构造
	FDebuff_Radial(const FDebuff& InDebuff)
	{
		bEnable = InDebuff.bEnable;
		LaunchParams = InDebuff.LaunchParams;
		TemporalDmgParams = InDebuff.TemporalDmgParams;
		SlowParams = InDebuff.SlowParams;
		bUseFalloff = InDebuff.bUseFalloff;  // 复制范围衰减属性
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDebuff_Beam
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "是否启用减益"))
	bool bEnable = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "击退"))
	FLaunchParams LaunchParams = FLaunchParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "延时伤害"))
	FTemporalDmgParams TemporalDmgParams = FTemporalDmgParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "减速"))
	FSlowParams SlowParams = FSlowParams();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用距离衰减，目前为线性衰减"))
	bool bUseFalloff = false;

	// 默认构造函数
	FDebuff_Beam() = default;

	// 从FDebuff构造
	FDebuff_Beam(const FDebuff& InDebuff)
	{
		bEnable = InDebuff.bEnable;
		LaunchParams = InDebuff.LaunchParams;
		TemporalDmgParams = InDebuff.TemporalDmgParams;
		SlowParams = InDebuff.SlowParams;
		bUseFalloff = InDebuff.bUseFalloff;  // 复制范围衰减属性
	}

	// 从FDebuff_Point构造
	FDebuff_Beam(const FDebuff_Point& InDebuff)
	{
		bEnable = InDebuff.bEnable;
		LaunchParams = InDebuff.LaunchParams;
		TemporalDmgParams = InDebuff.TemporalDmgParams;
		SlowParams = InDebuff.SlowParams;
	}

	// 从FDebuff_Radial构造
	FDebuff_Beam(const FDebuff_Radial& InDebuff)
	{
		bEnable = InDebuff.bEnable;
		LaunchParams = InDebuff.LaunchParams;
		TemporalDmgParams = InDebuff.TemporalDmgParams;
		SlowParams = InDebuff.SlowParams;
		bUseFalloff = InDebuff.bUseFalloff;  // 复制范围衰减属性
	}
};