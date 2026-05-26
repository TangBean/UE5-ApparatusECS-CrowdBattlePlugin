#pragma once

#include "CoreMinimal.h"
#include "BattleFrameEnums.generated.h"

UENUM(BlueprintType)
enum class ESortMode : uint8
{
    None UMETA(DisplayName = "None", ToolTip = "不排序"),
    NearToFar UMETA(DisplayName = "Near to Far", ToolTip = "从近到远"),
    FarToNear UMETA(DisplayName = "Far to Near", ToolTip = "从远到近")
};

UENUM(BlueprintType)
enum class ESpawnOrigin : uint8
{
    AtSelf UMETA(DisplayName = "AtSelf", Tooltip = "在自身位置"),
    AtTarget UMETA(DisplayName = "AtTarget", Tooltip = "在攻击目标位置")
};

UENUM(BlueprintType)
enum class EPlaySoundOrigin : uint8
{
    PlaySound2D UMETA(DisplayName = "PlaySound2D", Tooltip = "不使用空间音效"),
    PlaySound3D UMETA(DisplayName = "PlaySound3D_AtSelf", Tooltip = "使用空间音效"),
};

UENUM(BlueprintType)
enum class EPlaySoundOrigin_Attack : uint8
{
    PlaySound2D UMETA(DisplayName = "PlaySound2D", Tooltip = "不使用空间音效"),
    PlaySound3D_AtSelf UMETA(DisplayName = "PlaySound3D_AtSelf", Tooltip = "在自身位置"),
    PlaySound3D_AtTarget UMETA(DisplayName = "PlaySound3D_AtTarget", Tooltip = "在攻击目标位置")
};

UENUM(BlueprintType)
enum class EInitialDirection : uint8
{
	FacePlayer UMETA(DisplayName = "FacingPlayer0",Tooltip = "生成时面朝玩家0的方向"),
	FaceForward UMETA(DisplayName = "SpawnerForwardVector",Tooltip = "使用生成器的前向向量作为朝向"),
	CustomDirection UMETA(DisplayName = "CustomDirection",Tooltip = "自定义朝向")
};

UENUM(BlueprintType)
enum class EAttackState : uint8
{
	Aim_FirstExec UMETA(DisplayName = "Aim_FirstExec", ToolTip = "瞄准"),
	Aim UMETA(DisplayName = "Aim", ToolTip = "瞄准"),
	PreCast_FirstExec UMETA(DisplayName = "PreCast_FirstExec", ToolTip = "前摇"),
	PreCast UMETA(DisplayName = "Begin", ToolTip = "前摇"),
	PostCast UMETA(DisplayName = "Hit", ToolTip = "后摇"),
	Cooling UMETA(DisplayName = "Cooling", ToolTip = "冷却"),
	Completed UMETA(DisplayName = "Completed", ToolTip = "完成")
};

UENUM(BlueprintType)
enum class EMoveState : uint8
{
	Dirty UMETA(DisplayName = "Dirty", ToolTip = "无效数据"),
	Sleep_Sleeping UMETA(DisplayName = "Sleep_Sleeping", ToolTip = "休眠中"),
	Patrol_Patrolling UMETA(DisplayName = "Patrol_Patrolling", ToolTip = "巡逻中"),
	Patrol_Waiting UMETA(DisplayName = "PatrolWaiting", ToolTip = "在巡逻点位等待"),
	Chase_Chasing UMETA(DisplayName = "Chase_Chasing", ToolTip = "正在追逐目标"),
	Chase_Reached UMETA(DisplayName = "Chase_Reached", ToolTip = "已追到目标"),
	Approach_Approaching UMETA(DisplayName = "Approach_Approaching", ToolTip = "移动到位置"),
	Approach_Arrived UMETA(DisplayName = "Approach_Arrived", ToolTip = "已抵达目标位置")
};

UENUM(BlueprintType)
enum class EOrientMode : uint8
{
	ToPath UMETA(DisplayName = "ToPath", Tooltip = "面朝移动路径"),
	ToMovement UMETA(DisplayName = "OrientToMovement", Tooltip = "面朝实际运动方向"),
	ToMovementForwardAndBackward UMETA(DisplayName = "AlignToMovement", Tooltip = "面朝或背对实际运动方向"),
	ToCustom UMETA(DisplayName = "ToCustom", Tooltip = "面朝自定义方向"),
};

UENUM(BlueprintType)
enum class EGroundTraceMode : uint8
{
	FlowFieldAndSphereTrace	UMETA(DisplayName = "FlowFieldAndSphereTrace", Tooltip = "在陡坡使用球形检测, 否则使用流场"),
	FlowField				UMETA(DisplayName = "FlowField", Tooltip = "仅使用流场高度，可能不够精确"),
	SphereTrace				UMETA(DisplayName = "SphereTrace", Tooltip = "仅使用球形扫描，很精确但性能消耗大")
};

UENUM(BlueprintType)
enum class ETraceMode : uint8
{
	TargetIsPlayer_0 UMETA(DisplayName = "IsPlayer_0", Tooltip = "索敌目标为玩家0"),
	SectorTraceByTraits UMETA(DisplayName = "ByTraits", Tooltip = "根据特征进行扇形索敌")
};

UENUM(BlueprintType)
enum class ENavMode : uint8
{
	None UMETA(DisplayName = "Dirty", Tooltip = ""),
	AStar UMETA(DisplayName = "AStar", Tooltip = ""),
	FlowField UMETA(DisplayName = "FlowField", Tooltip = ""),
	ApproachDirectly UMETA(DisplayName = "ApproachDirectly", Tooltip = "")
};


// Event State
UENUM(BlueprintType)
enum class EAppearEventState : uint8
{
	Delay UMETA(DisplayName = "Delay", ToolTip = "延时"),
	Begin UMETA(DisplayName = "Begin", ToolTip = "开始"),
	End UMETA(DisplayName = "End", ToolTip = "结束")
};

UENUM(BlueprintType)
enum class ETraceEventState : uint8
{
	Begin UMETA(DisplayName = "Begin", ToolTip = "开始"),
	End_Reason_Succeed UMETA(DisplayName = "End_Reason_Succeed", ToolTip = "成功"),
	End_Reason_Fail UMETA(DisplayName = "End_Reason_Fail", ToolTip = "失败")
};

UENUM(BlueprintType)
enum class EMoveEventState : uint8
{
	Sleep_Sleeping UMETA(DisplayName = "Sleep_Sleeping", ToolTip = "休眠中"),
	Patrol_Patrolling UMETA(DisplayName = "Patrol_Patrolling", ToolTip = "巡逻中"),
	Patrol_Waiting UMETA(DisplayName = "PatrolWaiting", ToolTip = "在巡逻点位等待"),
	Chase_Chasing UMETA(DisplayName = "Chase_Chasing", ToolTip = "正在追逐目标"),
	Chase_Reached UMETA(DisplayName = "Chase_Reached", ToolTip = "已追到目标"),
	Approach_Approaching UMETA(DisplayName = "Approach_Approaching", ToolTip = "移动到位置"),
	Approach_Arrived UMETA(DisplayName = "Approach_Arrived", ToolTip = "已抵达目标位置")
};

UENUM(BlueprintType)
enum class EAttackEventState : uint8
{
	Aiming UMETA(DisplayName = "Aiming", ToolTip = "瞄准"),
	Begin UMETA(DisplayName = "Begin", ToolTip = "开始"),
	Hit UMETA(DisplayName = "Hit", ToolTip = "击中"),
	Cooling UMETA(DisplayName = "Cooling", ToolTip = "冷却"),
	End_Reason_InvalidTarget UMETA(DisplayName = "End_Reason:InvalidTarget", ToolTip = "攻击目标失效"),
	End_Reason_NotInATKRange UMETA(DisplayName = "End_Reason:NotInATKRange", ToolTip = "不在攻击范围内"),
	End_Reason_Complete UMETA(DisplayName = "End_Reason:Complete", ToolTip = "完成")
};

UENUM(BlueprintType)
enum class EHitEventState : uint8
{
	Begin UMETA(DisplayName = "Begin", ToolTip = "开始")
};

UENUM(BlueprintType)
enum class EDeathEventState : uint8
{
	OutOfHealth UMETA(DisplayName = "OutOfHealth", ToolTip = "生命值归零"),
	OutOfLifeSpan UMETA(DisplayName = "OutOfLifeSpan", ToolTip = "寿命归零"),
	SuicideAttack UMETA(DisplayName = "SuicideAttack", ToolTip = "自杀攻击"),
	KillZ UMETA(DisplayName = "KillZ", ToolTip = "低于强制移除高度")
};

UENUM(BlueprintType)
enum class EAnimState : uint8
{
	Dirty UMETA(DisplayName = "Dirty"),
	BS_IdleMove UMETA(DisplayName = "Blendspace_Idle-Move"),
	Appearing UMETA(DisplayName = "Appearing"),
	Sleeping UMETA(DisplayName = "Sleeping"),
	Attacking UMETA(DisplayName = "Attacking"),
	BeingHit UMETA(DisplayName = "BeingHit"),
	Dying UMETA(DisplayName = "Dying"),
	Falling UMETA(DisplayName = "Falling"),
	Jumping UMETA(DisplayName = "Jumping")
};

UENUM(BlueprintType)
enum class EProjectileMoveMode : uint8
{
	Static UMETA(DisplayName = "Static", ToolTip = "静态"),
	Interped UMETA(DisplayName = "Interped", ToolTip = "插值"),
	Ballistic UMETA(DisplayName = "Ballistic", ToolTip = "抛物线"),
	Tracking UMETA(DisplayName = "Tracking", ToolTip = "跟踪")
};

UENUM(BlueprintType)
enum class EProjectileDamageMode : uint8
{
	Point UMETA(DisplayName = "Point", ToolTip = "点"),
	Radial UMETA(DisplayName = "Radial", ToolTip = "球形"),
	Beam UMETA(DisplayName = "Beam", ToolTip = "球扫")
};