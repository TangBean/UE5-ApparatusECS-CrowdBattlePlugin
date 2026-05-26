#include "BFSubjectiveAgentComponent.h"
#include "Traits/Health.h"
#include "Traits/GridData.h"
#include "Traits/Collider.h"
#include "Traits/BindFlowField.h"
#include "Traits/Activated.h"
#include "Traits/IsSubjective.h"
#include "Traits/Damage.h"
#include "Traits/SubType.h"
#include "Traits/Move.h"
#include "Traits/TextPopUp.h"
#include "Traits/Animation.h"
#include "Traits/Appear.h"
#include "Traits/Team.h"
#include "BattleFrameFunctionLibraryRT.h"
#include "NiagaraSubjectRenderer.h"

UBFSubjectiveAgentComponent::UBFSubjectiveAgentComponent()
{
    // Enable ticking
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UBFSubjectiveAgentComponent::BeginPlay()
{
    Super::BeginPlay();

    if(bAutoInitWithDataAsset) InitializeSubjectTraits(true, GetOwner());
}

void UBFSubjectiveAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bSyncTransformSubjectToActor) SyncTransformSubjectToActor(GetOwner());
}

void UBFSubjectiveAgentComponent::InitializeSubjectTraits(bool bAutoActivation, AActor* OwnerActor)
{
    if (!OwnerActor) return;

    if (!CurrentWorld)
    {
        CurrentWorld = GetWorld();

        if (!CurrentWorld) return;
    }

    if (!Mechanism)
    {
        Mechanism = UMachine::ObtainMechanism(CurrentWorld);

        if (!Mechanism) return;
    }

    if (!BattleControl)
    {
        BattleControl = Cast<ABattleFrameBattleControl>(UGameplayStatics::GetActorOfClass(CurrentWorld, ABattleFrameBattleControl::StaticClass()));

        if (!BattleControl) return;
    }

    UAgentConfigDataAsset* DataAsset = AgentConfigAsset.LoadSynchronous();

    if (!IsValid(DataAsset)) return;

    FSubjectRecord AgentConfig = DataAsset->ExtraTraits;

    AgentConfig.SetTrait(DataAsset->Agent);
    AgentConfig.SetTrait(DataAsset->SubType);
    AgentConfig.SetTrait(FTeam(TeamIndex));
    AgentConfig.SetTrait(DataAsset->Collider);
    AgentConfig.SetTrait(FLocated());
    AgentConfig.SetTrait(FDirected());
    AgentConfig.SetTrait(DataAsset->Scale);
    AgentConfig.SetTrait(DataAsset->Health);
    AgentConfig.SetTrait(DataAsset->Damage);
    AgentConfig.SetTrait(DataAsset->Debuff);
    AgentConfig.SetTrait(DataAsset->Defence);
    AgentConfig.SetTrait(DataAsset->Sleep);
    AgentConfig.SetTrait(DataAsset->Move);
    AgentConfig.SetTrait(DataAsset->Fall);
    AgentConfig.SetTrait(FMoving());
    AgentConfig.SetTrait(DataAsset->Patrol);
    AgentConfig.SetTrait(DataAsset->Navigation);
    AgentConfig.SetTrait(FNavigating());
    AgentConfig.SetTrait(DataAsset->Avoidance);
    AgentConfig.SetTrait(FAvoiding());
    AgentConfig.SetTrait(DataAsset->Appear);
    AgentConfig.SetTrait(DataAsset->Trace);
    AgentConfig.SetTrait(FTracing());
    AgentConfig.SetTrait(DataAsset->Chase);
    AgentConfig.SetTrait(DataAsset->Attack);
    AgentConfig.SetTrait(DataAsset->Hit);
    AgentConfig.SetTrait(DataAsset->Death);
    AgentConfig.SetTrait(DataAsset->Animation);
    AgentConfig.SetTrait(FAnimating());
    AgentConfig.SetTrait(DataAsset->HealthBar);
    AgentConfig.SetTrait(DataAsset->TextPop);
    AgentConfig.SetTrait(FPoppingText());
    AgentConfig.SetTrait(DataAsset->Curves);
    AgentConfig.SetTrait(FTemporalDamaging());
    AgentConfig.SetTrait(FSlowing());
    AgentConfig.SetTrait(DataAsset->Statistics);
    AgentConfig.SetTrait(FIsSubjective());

    AgentConfig.SetFlag(AppearDissolveFlag, false);
    AgentConfig.SetFlag(DeathDissolveFlag, false);

    AgentConfig.SetFlag(HitGlowFlag, false);
    AgentConfig.SetFlag(HitJiggleFlag, false);
    AgentConfig.SetFlag(HitPoppingTextFlag, false);
    AgentConfig.SetFlag(HitDecideHealthFlag, false);
    AgentConfig.SetFlag(DeathDisableCollisionFlag, false);
    AgentConfig.SetFlag(RegisterMultipleFlag, false);

    AgentConfig.SetFlag(AppearAnimFlag, false);
    AgentConfig.SetFlag(AttackAnimFlag, false);
    AgentConfig.SetFlag(HitAnimFlag, false);
    AgentConfig.SetFlag(DeathAnimFlag, false);
    AgentConfig.SetFlag(FallAnimFlag, false);

    // Apply Multipliers
    auto& Health = AgentConfig.GetTraitRef<FHealth>();
    Health.Current *= Multipliers.HealthMult;
    Health.Maximum *= Multipliers.HealthMult;

    auto& TextPopUp = AgentConfig.GetTraitRef<FTextPopUp>();
    TextPopUp.WhiteTextBelowPercent *= Multipliers.HealthMult;
    TextPopUp.OrangeTextAbovePercent *= Multipliers.HealthMult;

    auto& Damage = AgentConfig.GetTraitRef<FDamage>();
    Damage.Damage *= Multipliers.DamageMult;

    auto& Scaled = AgentConfig.GetTraitRef<FScaled>();
    Scaled.Scale *= Multipliers.ScaleMult;
    Scaled.RenderScale *= Multipliers.ScaleMult;

    auto& Move = AgentConfig.GetTraitRef<FMove>();
    Move.XY.MoveSpeed *= Multipliers.MoveSpeedMult;

    auto& Collider = AgentConfig.GetTraitRef<FCollider>();
    auto& Located = AgentConfig.GetTraitRef<FLocated>();
    auto& Directed = AgentConfig.GetTraitRef<FDirected>();
    auto& Fall = AgentConfig.GetTraitRef<FFall>();
    auto& Moving = AgentConfig.GetTraitRef<FMoving>();
    auto& Patrol = AgentConfig.GetTraitRef<FPatrol>();

    Directed.Direction = OwnerActor->GetActorRotation().Vector().GetSafeNormal2D();

    FVector SpawnPoint3D = OwnerActor->GetActorLocation();

    if (Fall.bCanFly)
    {
        Moving.FlyingHeight = FMath::RandRange(Fall.FlyHeight.X, Fall.FlyHeight.Y);
        SpawnPoint3D = FVector(SpawnPoint3D.X, SpawnPoint3D.Y, SpawnPoint3D.Z + Moving.FlyingHeight);
    }

    Patrol.Origin = SpawnPoint3D;
    Moving.Goal = SpawnPoint3D;

    Located.Location = SpawnPoint3D;
    Located.PreLocation = SpawnPoint3D;
    Located.InitialLocation = SpawnPoint3D;

    if (LaunchVelocity.Size() > 0)
    {
        Moving.LaunchVelSum = Directed.Direction * LaunchVelocity.X + Directed.Direction.UpVector * LaunchVelocity.Y;
        Moving.bLaunching = true;
    }

    this->GetHandle()->RemoveAllTraits();
    this->GetHandle()->SetTraits(AgentConfig);

    if (bAutoActivation) ActivateAgent(this->GetHandle());
}

void UBFSubjectiveAgentComponent::ActivateAgent(FSubjectHandle Agent)// strange apparatus bug : don't use get ref or the value may expire later when use
{
    TRACE_CPUPROFILER_EVENT_SCOPE_STR("ActivateAgent");

    auto Located = Agent.GetTrait<FLocated>();
    auto Scaled = Agent.GetTrait<FScaled>();
    auto Collider = Agent.GetTrait<FCollider>();
    auto Appear = Agent.GetTrait<FAppear>();
    auto Sleep = Agent.GetTrait<FSleep>();
    auto Patrol = Agent.GetTrait<FPatrol>();
    auto Animation = Agent.GetTrait<FAnimation>();
    auto Animating = Agent.GetTrait<FAnimating>();
    auto Team = Agent.GetTrait<FTeam>();
    auto SubType = Agent.GetTrait<FSubType>();
    auto Avoidance = Agent.GetTrait<FAvoidance>();

    Animating.AnimToTextureData = Animation.AnimToTextureDataAsset.LoadSynchronous(); // DataAsset Solid Pointer

    if (IsValid(Animating.AnimToTextureData))
    {
        Animating.AnimPauseFrameArray.Empty();

        for (FAnimToTextureAnimInfo CurrentAnim : Animating.AnimToTextureData->Animations)
        {
            Animating.AnimPauseFrameArray.Add(CurrentAnim.EndFrame - CurrentAnim.StartFrame);
        }

        Animating.SampleRate = Animating.AnimToTextureData->SampleRate;
        Animating.AnimIndex0 = Animating.AnimIndex1 = Animating.AnimIndex2 = Animation.IndexOfIdleAnim;
        Animating.CurrentMontageSlot = 2;
    }

    if (Appear.bEnable)
    {
        Animating.Dissolve = 1;
        Agent.SetTrait(FAppearing());
    }
    else
    {
        Agent.RemoveTrait<FAppearing>();
    }

    Agent.SetTrait(Animating);

    if (Sleep.bEnable)
    {
        Agent.SetTrait(FSleeping());
    }
    else
    {
        Agent.RemoveTrait<FSleeping>();
    }

    if (Patrol.bEnable)
    {
        Agent.SetTrait(FPatrolling());
    }
    else
    {
        Agent.RemoveTrait<FPatrolling>();
    }

    if (Collider.bHightQuality)
    {
        //Agent.SetTrait(FRegisterMultiple());
        Agent.SetFlag(RegisterMultipleFlag, true);
    }
    else
    {
        Agent.SetFlag(RegisterMultipleFlag, false);
    }

    UBattleFrameFunctionLibraryRT::RemoveSubjectSubTypeTraitByIndex(SubType.PreviousIndex, Agent);
    UBattleFrameFunctionLibraryRT::RemoveSubjectTeamTraitByIndex(FMath::Clamp(Team.PreviousIndex, 0, 9), Agent);
    UBattleFrameFunctionLibraryRT::RemoveSubjectAvoGroupTraitByIndex(FMath::Clamp(Avoidance.PreviousGroup, 0, 9), Agent);

    UBattleFrameFunctionLibraryRT::SetSubjectSubTypeTraitByIndex(SubType.Index, Agent);
    UBattleFrameFunctionLibraryRT::SetSubjectTeamTraitByIndex(FMath::Clamp(Team.index, 0, 9), Agent);
    UBattleFrameFunctionLibraryRT::SetSubjectAvoGroupTraitByIndex(FMath::Clamp(Avoidance.Group, 0, 9), Agent);

    Team.PreviousIndex = Team.index;
    SubType.PreviousIndex = SubType.Index;
    Avoidance.PreviousGroup = Avoidance.Group;

    Agent.SetTrait(Team);
    Agent.SetTrait(SubType);
    Agent.SetTrait(Avoidance);

    Agent.SetTrait(FGridData{ Agent.CalcHash(), FVector3f(Located.Location), Collider.Radius * Scaled.Scale, Agent });
    Agent.SetTrait(FActivated());

    // 如果场上没有，生成该怪物的渲染器
    if (CurrentWorld && BattleControl && !BattleControl->ExistingRenderers.Contains(SubType.Index))
    {
        TSubclassOf<ANiagaraSubjectRenderer> RendererClass = Animation.RendererClass.LoadSynchronous();

        if (IsValid(RendererClass))
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SpawnParams.bNoFail = true;

            // 直接生成目标类型
            ANiagaraSubjectRenderer* RendererActor = CurrentWorld->SpawnActor<ANiagaraSubjectRenderer>(RendererClass, FTransform::Identity, SpawnParams);

            // 设置SubType参数
            if (RendererActor)
            {
                BattleControl->ExistingRenderers.Add(SubType.Index);
                RendererActor->SubType.Index = SubType.Index;
                RendererActor->SetActorTickEnabled(true);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Animation.RendererClass is invalid | Animation.RendererClass无效"));
        }
    }
}

void UBFSubjectiveAgentComponent::SyncTransformSubjectToActor(AActor* OwnerActor)
{
    if (!OwnerActor) return;

    FTransform Transform = FTransform::Identity;

    auto Located = GetTraitPtr<FLocated, EParadigm::Unsafe>();

    if (Located)
    {
        Transform.SetLocation(Located->Location);
    }

    auto Directed = GetTraitPtr<FDirected, EParadigm::Unsafe>();

    if (Directed)
    {
        Transform.SetRotation(Directed->Direction.ToOrientationQuat());
    }

    auto Scaled = GetTraitPtr<FScaled, EParadigm::Unsafe>();

    if (Scaled)
    {
        Transform.SetScale3D(FVector(Scaled->Scale));
    }

    OwnerActor->SetActorTransform(Transform);
}
