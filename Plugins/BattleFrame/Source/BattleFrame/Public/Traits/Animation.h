#pragma once
 
#include "CoreMinimal.h"
#include "AnimToTextureDataAsset.h"
#include "BattleFrameEnums.h"
#include "Animation.generated.h"
 

class ANiagaraSubjectRenderer;

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAnimation
{
	GENERATED_BODY()

    //-----------------------------------------------------------

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftObjectPtr <UAnimToTextureDataAsset> AnimToTextureDataAsset;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftClassPtr<ANiagaraSubjectRenderer> RendererClass;

    //-------------------------------------------------------------

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "动画过渡速度"))
    float LerpSpeed = 4;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "待机动画随机时间偏移"))
    FVector2D IdleRandomTimeOffset = FVector2D(0, 0);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "移动动画随机时间偏移"))
    FVector2D MoveRandomTimeOffset = FVector2D(0, 0);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "使用待机动画的移速，使用移动动画的移速", DisplayName = "BlendSpace_Idle-Move"))
    FVector2D BS_IdleMove = FVector2D(0, 300);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "待机动画播放速度"))
    float IdlePlayRate = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "移动动画播放速度"))
    float MovePlayRate = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "坠落动画播放速度"))
    float FallPlayRate = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "跳跃动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfIdleAnim = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "跳跃动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfMoveAnim = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "出生动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfAppearAnim = 2;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "攻击动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfAttackAnim = 3;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "受击动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfHitAnim = 4;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "死亡动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfDeathAnim = 5;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "坠落动画的索引值,AnimToTextureDataAsset里可查到"))
    int32 IndexOfFallAnim = 6;

    //UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = "跳跃动画的索引值"))
    //int32 IndexOfJumpAnim = 7;

};

USTRUCT(BlueprintType)
struct BATTLEFRAME_API FAnimating
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

    //-----------------------------------------------------------

    TObjectPtr<UAnimToTextureDataAsset> AnimToTextureData = nullptr;

    //-----------------------------------------------------------

    float AnimIndex0 = 0;
    float AnimPlayRate0 = 0.f;
    float AnimCurrentTime0 = 0.f;
    float AnimOffsetTime0 = 0.f;
    float AnimPauseFrame0 = 0.f;

    float AnimIndex1 = 0;
    float AnimPlayRate1 = 0.f;
    float AnimCurrentTime1 = 0.f;
    float AnimOffsetTime1 = 0.f;
    float AnimPauseFrame1 = 0.f;

    float AnimIndex2 = 0;
    float AnimPlayRate2 = 0.f;
    float AnimCurrentTime2 = 0.f;
    float AnimOffsetTime2 = 0.f;
    float AnimPauseFrame2 = 0.f;

    float AnimLerp0 = 0;
    float AnimLerp1 = 0;

    int32 CurrentMontageSlot = 2;
    int32 SampleRate = 30;

    //-----------------------------------------------------------

    TArray<int32> AnimPauseFrameArray;

    //-----------------------------------------------------------

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float Team = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float HitGlow = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float Dissolve = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float IceFx = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float FireFx = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    float PoisonFx = 0;

    float IceFxInterped = 0;
    float FireFxInterped = 0;
    float PoisonFxInterped = 0;


    //-----------------------------------------------------------

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    EAnimState AnimState = EAnimState::BS_IdleMove;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    EAnimState PreviousAnimState = EAnimState::Dirty;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Tooltip = ""))
    bool bUpdateAnimState = true;

    //-----------------------------------------------------------

    FAnimating() {};

    FAnimating(const FAnimating& Anim)
    {
        LockFlag.store(Anim.LockFlag.load());

        AnimToTextureData = Anim.AnimToTextureData;

        Team = Anim.Team;
        HitGlow = Anim.HitGlow;
        Dissolve = Anim.Dissolve;

        IceFx = Anim.IceFx;
        FireFx = Anim.FireFx;
        PoisonFx = Anim.PoisonFx;

        IceFxInterped = Anim.IceFxInterped;
        FireFxInterped = Anim.FireFxInterped;
        PoisonFxInterped = Anim.PoisonFxInterped;

        AnimIndex0 = Anim.AnimIndex0;
        AnimPlayRate0 = Anim.AnimPlayRate0;
        AnimCurrentTime0 = Anim.AnimCurrentTime0;
        AnimOffsetTime0 = Anim.AnimOffsetTime0;
        AnimPauseFrame0 = Anim.AnimPauseFrame0;

        AnimIndex1 = Anim.AnimIndex1;
        AnimPlayRate1 = Anim.AnimPlayRate1;
        AnimCurrentTime1 = Anim.AnimCurrentTime1;
        AnimOffsetTime1 = Anim.AnimOffsetTime1;
        AnimPauseFrame1 = Anim.AnimPauseFrame1;

        AnimIndex2 = Anim.AnimIndex2;
        AnimPlayRate2 = Anim.AnimPlayRate2;
        AnimCurrentTime2 = Anim.AnimCurrentTime2;
        AnimOffsetTime2 = Anim.AnimOffsetTime2;
        AnimPauseFrame2 = Anim.AnimPauseFrame2;

        AnimLerp0 = Anim.AnimLerp0;
        AnimLerp1 = Anim.AnimLerp1;

        CurrentMontageSlot = Anim.CurrentMontageSlot;
        SampleRate = Anim.SampleRate;

        AnimPauseFrameArray = Anim.AnimPauseFrameArray;

        AnimState = Anim.AnimState;
        PreviousAnimState = Anim.PreviousAnimState;

        bUpdateAnimState = Anim.bUpdateAnimState;
    }

    FAnimating& operator=(const FAnimating& Anim)
    {
        LockFlag.store(Anim.LockFlag.load());

        AnimToTextureData = Anim.AnimToTextureData;

        Team = Anim.Team;
        HitGlow = Anim.HitGlow;
        Dissolve = Anim.Dissolve;

        IceFx = Anim.IceFx;
        FireFx = Anim.FireFx;
        PoisonFx = Anim.PoisonFx;

        IceFxInterped = Anim.IceFxInterped;
        FireFxInterped = Anim.FireFxInterped;
        PoisonFxInterped = Anim.PoisonFxInterped;

        AnimIndex0 = Anim.AnimIndex0;
        AnimPlayRate0 = Anim.AnimPlayRate0;
        AnimCurrentTime0 = Anim.AnimCurrentTime0;
        AnimOffsetTime0 = Anim.AnimOffsetTime0;
        AnimPauseFrame0 = Anim.AnimPauseFrame0;

        AnimIndex1 = Anim.AnimIndex1;
        AnimPlayRate1 = Anim.AnimPlayRate1;
        AnimCurrentTime1 = Anim.AnimCurrentTime1;
        AnimOffsetTime1 = Anim.AnimOffsetTime1;
        AnimPauseFrame1 = Anim.AnimPauseFrame1;

        AnimIndex2 = Anim.AnimIndex2;
        AnimPlayRate2 = Anim.AnimPlayRate2;
        AnimCurrentTime2 = Anim.AnimCurrentTime2;
        AnimOffsetTime2 = Anim.AnimOffsetTime2;
        AnimPauseFrame2 = Anim.AnimPauseFrame2;

        AnimLerp0 = Anim.AnimLerp0;
        AnimLerp1 = Anim.AnimLerp1;

        CurrentMontageSlot = Anim.CurrentMontageSlot;
        SampleRate = Anim.SampleRate;

        AnimPauseFrameArray = Anim.AnimPauseFrameArray;

        AnimState = Anim.AnimState;
        PreviousAnimState = Anim.PreviousAnimState;

        bUpdateAnimState = Anim.bUpdateAnimState;

        return *this;
    }
};
