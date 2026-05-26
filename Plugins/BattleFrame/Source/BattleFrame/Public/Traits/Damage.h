#pragma once

#include "CoreMinimal.h"
#include "Traits/Trace.h"
#include "Damage.generated.h"

UENUM(BlueprintType)
enum class EDmgType : uint8
{
	Normal UMETA(DisplayName = "Normal Damage", Tooltip = "普通伤"),
	Fire UMETA(DisplayName = "Fire Damage", Tooltip = "火伤"),
	Ice UMETA(DisplayName = "Ice Damage", Tooltip = "冰伤"),
	Poison UMETA(DisplayName = "Poison Damage", Tooltip = "毒伤")
	//Heal UMETA(DisplayName = "Heal", Tooltip = "治疗")
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDamage
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害类型"))
	EDmgType DmgType = EDmgType::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "普通伤"))
	float Damage = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "百分比伤"))
	float PercentDmg = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击倍数"))
	float CritDmgMult = 2.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击概率"))
	float CritProbability = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害半径，0为单体伤害"))
	float DmgRadius = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用距离衰减，目前为线性衰减"))
	bool bUseFalloff = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否排除障碍物后的目标"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "伤害过滤器"))
	FBFFilter Filter = FBFFilter();

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDamage_Point
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害类型"))
	EDmgType DmgType = EDmgType::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "普通伤"))
	float Damage = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "百分比伤"))
	float PercentDmg = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击倍数"))
	float CritDmgMult = 2.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击概率"))
	float CritProbability = 0.1f;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "伤害过滤器"))
	//FBFFilter Filter = FBFFilter();

	// 默认构造函数
	FDamage_Point() = default;

	// 从FDamage构造
	FDamage_Point(const FDamage& InDamage)
	{
		DmgType = InDamage.DmgType;
		Damage = InDamage.Damage;
		PercentDmg = InDamage.PercentDmg;
		CritDmgMult = InDamage.CritDmgMult;
		CritProbability = InDamage.CritProbability;
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDamage_Radial
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害类型"))
	EDmgType DmgType = EDmgType::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "普通伤"))
	float Damage = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "百分比伤"))
	float PercentDmg = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击倍数"))
	float CritDmgMult = 2.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击概率"))
	float CritProbability = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害半径，0为单体伤害"))
	float DmgRadius = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用范围伤害衰减"))
	bool bUseFalloff = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否排除障碍物后的目标"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "范围伤害过滤器"))
	FBFFilter Filter = FBFFilter();

	// 默认构造函数
	FDamage_Radial() = default;

	// 从FDamage构造
	FDamage_Radial(const FDamage& InDamage)
	{
		DmgType = InDamage.DmgType;
		Damage = InDamage.Damage;
		PercentDmg = InDamage.PercentDmg;
		CritDmgMult = InDamage.CritDmgMult;
		CritProbability = InDamage.CritProbability;
		DmgRadius = InDamage.DmgRadius;
		bUseFalloff = InDamage.bUseFalloff;
		bCheckObstacle = InDamage.bCheckObstacle;
		Filter = InDamage.Filter;
	}
};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FDamage_Beam
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害类型"))
	EDmgType DmgType = EDmgType::Normal;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "普通伤"))
	float Damage = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "百分比伤"))
	float PercentDmg = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击倍数"))
	float CritDmgMult = 2.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "暴击概率"))
	float CritProbability = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害半径，0为单体伤害"))
	float DmgRadius = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "伤害方向乘以距离"))
	FVector DmgDirectionAndDistance = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否启用范围伤害衰减"))
	bool bUseFalloff = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ToolTip = "是否排除障碍物后的目标"))
	bool bCheckObstacle = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip = "范围伤害过滤器"))
	FBFFilter Filter = FBFFilter();


	// 默认构造函数
	FDamage_Beam() = default;

	// 从FDamage构造
	FDamage_Beam(const FDamage& InDamage)
	{
		DmgType = InDamage.DmgType;
		Damage = InDamage.Damage;
		PercentDmg = InDamage.PercentDmg;
		CritDmgMult = InDamage.CritDmgMult;
		CritProbability = InDamage.CritProbability;
		DmgRadius = InDamage.DmgRadius;
		bUseFalloff = InDamage.bUseFalloff;
		bCheckObstacle = InDamage.bCheckObstacle;
		Filter = InDamage.Filter;
	}
};
