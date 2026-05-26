/*
* BattleFrame
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

#pragma once

#include "Engine/DataAsset.h"
#include "BattleFrameEnums.h"

// 移动相关 Traits
#include "Traits/PrimaryType.h"
#include "Traits/SubType.h"
#include "Traits/ProjectileConfig.h"
#include "Traits/Damage.h"
#include "Traits/Debuff.h"
#include "Traits/Trace.h"
#include "Traits/Transform.h"

#include "ProjectileConfigDataAsset.generated.h"


UCLASS(Blueprintable, BlueprintType)
class BATTLEFRAME_API UProjectileConfigDataAsset : public UPrimaryDataAsset
{
public:

    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Common", meta = (ToolTip = "主类型"))
    FProjectile Projectile;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Common", meta = (ToolTip = "子类型"))
    FSubType SubType;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Common", meta = (ToolTip = "通用参数"))
    FProjectileParams ProjectileParams;


    // 投射物运动模式
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement", meta = (ToolTip = "投射物运动模式"))
    EProjectileMoveMode MovementMode = EProjectileMoveMode::Ballistic;

    // 运动模式相关参数（根据MovementMode显示）
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement", meta = (DisplayName = "ProjectileMove", ToolTip = "抛物线运动参数", EditCondition = "MovementMode == EProjectileMoveMode::Ballistic", EditConditionHides))
    FProjectileMove_Ballistic ProjectileMove_Ballistic;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement", meta = (DisplayName = "ProjectileMove", ToolTip = "插值运动参数", EditCondition = "MovementMode == EProjectileMoveMode::Interped", EditConditionHides))
    FProjectileMove_Interped ProjectileMove_Interped;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement", meta = (DisplayName = "ProjectileMove", ToolTip = "跟踪运动参数", EditCondition = "MovementMode == EProjectileMoveMode::Tracking", EditConditionHides))
    FProjectileMove_Tracking ProjectileMove_Tracking;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement", meta = (DisplayName = "ProjectileMove", ToolTip = "静止运动参数", EditCondition = "MovementMode == EProjectileMoveMode::Static", EditConditionHides))
    FProjectileMove_Static ProjectileMove_Static;


    // 投射物伤害模式
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (ToolTip = "投射物伤害模式"))
    EProjectileDamageMode DamageMode = EProjectileDamageMode::Radial;

    // 伤害模式相关参数（根据DamageMode显示）
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Damage", ToolTip = "点伤害参数", EditCondition = "DamageMode == EProjectileDamageMode::Point", EditConditionHides))
    FDamage_Point Damage_Point;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Damage", ToolTip = "球形伤害参数", EditCondition = "DamageMode == EProjectileDamageMode::Radial", EditConditionHides))
    FDamage_Radial Damage_Radial;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Damage", ToolTip = "球扫伤害参数", EditCondition = "DamageMode == EProjectileDamageMode::Beam", EditConditionHides))
    FDamage_Beam Damage_Beam;

    // Debuff相关参数（根据DamageMode显示）
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Debuff", ToolTip = "点Debuff参数", EditCondition = "DamageMode == EProjectileDamageMode::Point", EditConditionHides))
    FDebuff_Point Debuff_Point;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Debuff", ToolTip = "球形Debuff参数", EditCondition = "DamageMode == EProjectileDamageMode::Radial", EditConditionHides))
    FDebuff_Radial Debuff_Radial;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DamageAndDebuff", meta = (DisplayName = "Debuff", ToolTip = "球扫Debuff参数", EditCondition = "DamageMode == EProjectileDamageMode::Beam", EditConditionHides))
    FDebuff_Beam Debuff_Beam;

    // Runtime Params
    FProjectileParamsRT ProjectileParamsRT;
    FProjectileMoving_Interped ProjectileMoving_Interped;
    FProjectileMoving_Ballistic ProjectileMoving_Ballistic;
    FProjectileMoving_Tracking ProjectileMoving_Tracking;

    FLocated Located;
    FDirected Directed;
    FScaled Scaled;

    UProjectileConfigDataAsset() {};
};
