# BattleFrame 插件调研报告

> 调研对象：`BattleFrame/`（UE5 群体战斗插件）
> 基于 Apparatus ECS（Subject/Trait/Mechanism）+ FlowFieldCanvas + Niagara + AnimToTexture
> 报告中所有功能描述均带 `file:line` 代码引用

---

## 1. 插件元数据与依赖

- 清单：`BattleFrame/BattleFrame.uplugin`
  - `FriendlyName: "BattleFrame"`，作者 `LeroyWorks`，仅 `Win64`
  - 两个模块：
    - `BattleFrame`（Runtime，`LoadingPhase: PreDefault`）
    - `BattleFrameEditor`（Editor，`LoadingPhase: PostEngineInit`）
  - 强依赖插件：`Apparatus`、`FlowFieldCanvas`、`Niagara`、`AnimToTexture`（`BattleFrame.uplugin:18-43`）
- 运行时模块入口：`BattleFrame/Source/BattleFrame/Public/BattleFrame.h:11` 与 `Private/BattleFrame.cpp`（仅 `StartupModule/ShutdownModule` 桩）
- 编辑器模块构建：`BattleFrameEditor.Build.cs`（依赖 `UnrealEd`、`AssetTools`、`BlueprintGraph`、`Niagara`、`NiagaraCore`、`BattleFrame`）
- 图标资源：`BattleFrame/Resources/Icon128.png`
- 单例工具模板：`Public/ApparatusSingleton.h:28` `GetSingletonSubject<T,Ts...>`、`:54` `GetSingletonTrait<T,Ts...>`

---

## 2. ECS Trait 一览（按用途分类，共 44 个 Trait 头文件）

所有 Trait 位于 `BattleFrame/Source/BattleFrame/Public/Traits/`。下面按业务域分类，每条列出关键字段与代码位置。

### 2.1 基础数据 / 变换
- `FLocated`（`Transform.h:7`）：`Location/PreLocation/InitialLocation`
- `FRotated`（`Transform.h:25`）：`Rotation`（FQuat）
- `FDirected`（`Transform.h:37`）：`Direction/DesiredDirection`，朝向与目标朝向
- `FScaled`（`Transform.h:52`）：`Scale/RenderScale/JiggleMultiplier`（实际尺寸 = `RenderScale * JiggleMultiplier`）
- `FAge / FAging`（`Age.h:7,16`）：生存时间累计
- `FActivated`（`Activated.h:6`）：激活标记（空 tag）
- `FIsSubjective`（`IsSubjective.h:6`）：标记 Subject↔Actor 链接的标识 tag
- `FMayDie`（`MayDie.h:8`）：延迟死亡定时器 `TimeLeft = 3`

### 2.2 主类型 / 子类型 / 队伍（用于 Filter 索引）
- 主类型 tag（`PrimaryType.h`）：`FAgent`（含 `Score`）、`FHero`、`FProjectile`、`FProp`、`FTower`
- 子类型：
  - 主结构 `FSubType { int32 Index; int32 PreviousIndex; }`（`SubType.h:6`）
  - 81 个空 tag `FSubType0`…`FSubType81`（`SubType.h:17-...`，文件 595 行）
  - 枚举 `EESubType { SubType0…SubType81 }`（蓝图侧选择）
- 队伍：
  - 主结构 `FTeam { int32 index; int32 PreviousIndex; }`（`Team.h:5`）
  - 10 个空 tag `FTeam0`…`FTeam9`（`Team.h:17-75`）
- 避障组（Avoidance Group）：10 个空 tag `FAvoGroup0`…`FAvoGroup9`（`AvoGroup.h:7-63`）
- 自定义业务标签：10 个空 tag `FBFTag1`…`FBFTag10`（`BFTag.h:7-64`）

子类型/Team/AvoGroup 的"以索引切换 tag"动作由 `BattleFrameFunctionLibraryRT.h` 中一组 `Set/Remove/IncludeXxxByIndex(int32 Index, …)` 内联实现（`BattleFrameFunctionLibraryRT.h:361-697`）。

### 2.3 感知 / 索敌
- `FTrace`（`Trace.h:114`）：启用、debug 绘制、`FSectorTraceShape`、模式 `ETraceMode`、`FBFFilter`
  - 索敌形状参数：`FSectorTraceParams`（`Trace.h:12`）含 `TraceRadius/TraceAngle/TraceHeight/LocationOffset/YawOffset/CoolDown` 与 `bCheckObstacle`
  - `FSectorTraceShape`（`Trace.h:74`）：通用 Common + Sleep/Patrol/Chase 三种专用覆盖
  - `FBFFilter`（`Trace.h:96`）：`IncludeTraits/ExcludeTraits/ObstacleObjectType`
  - 模式枚举 `ETraceMode`（`BattleFrameEnums.h:87`）：`TargetIsPlayer_0` / `SectorTraceByTraits`
- `FTracing`（`Trace.h:138`）：含 `TraceResult/NeighborGrid/TimeLeft` 与 atomic lock

### 2.4 移动 / 转向 / 坠落 / 寻路
- `FMove`（`Move.h:108`）+ 子结构：
  - `FYawMovement`（`Move.h:9`）：`TurnSpeed/TurnAcceleration/EOrientMode` 转向模式
  - `FXYMovement`（`Move.h:32`）：`MoveSpeed/Accel/Decel`、按角度的速度乘数 `MoveSpeedRangeMapByAngle`、按距离 `MoveSpeedRangeMapByDist`、弹跳衰减 `MoveBounceVelocityDecay`、`AcceptanceRadius`
  - `EOrientMode`（`BattleFrameEnums.h:70`）：`ToPath / ToMovement / ToMovementForwardAndBackward / ToCustom`
- `FFall`（`Move.h:65`）：可飞行 `bCanFly`、随机飞行高度区间 `FlyHeight`、`Gravity`、`KillZ`、地面检测模式 `EGroundTraceMode`、`SphereTraceAngleThreshold`、`GroundObjectType`
  - `EGroundTraceMode`（`BattleFrameEnums.h:79`）：FlowField/SphereTrace/混合
- `FMoving`（`Move.h:132`）：运行时速度状态（`CurrentVelocity/DesiredVelocity/Goal/MoveState/LaunchVelSum/PushBackSpeedOverride/FlyingHeight`）、5 帧速度环形缓冲（`HistoryVelocity` + `UpdateVelocityHistory` `Move.h:204`）
- `FNavigation`（`Navigation.h:9`）：选 FlowField、`bReloadFlowField`、`bUseAStar`、`AStarCoolDown`
- `FNavigating`（`Navigation.h:33`）：路径点 `PathPoints`、刷新计时 `TimeLeft`、缓存 `FlowField*`、`PreviousNavMode`
- `ENavMode`（`BattleFrameEnums.h:94`）：`None / AStar / FlowField / ApproachDirectly`
- `FBindFlowField`（`BindFlowField.h:8`）：绑定流场资产
- `FSleep / FSleeping`（`Sleep.h:8,25`）：休眠、`bCanTrace/bWakeOnHit`
- `FPatrol / FPatrolling`（`Patrol.h:24,66`）：巡逻半径区间、`MaxMovingTime`、停留 `CoolDown`、原点模式 `EPatrolOriginMode`、丢失目标行为 `EPatrolRecoverMode`、`MoveSpeedMult`
- `FChase`（`Chase.h:7`）：`bCanTrace`、`AcceptanceRadius`、`MoveSpeedMult`
- 移动状态枚举：`EMoveState`（`BattleFrameEnums.h:57`）：`Sleep_Sleeping / Patrol_Patrolling/Waiting / Chase_Chasing/Reached / Approach_Approaching/Arrived`

### 2.5 避障
- `FAvoidance`（`Avoidance.h:17`）：模式 `EAvoidMode { RVO2, PBD }`（`Avoidance.h:11`）、`Group/IgnoreGroups`、`TraceDist`、`AvoidDistMult`、`MaxNeighbors`、`RVO_TimeHorizon_Agent/Obstacle`，缓存 `OrcaLines/DesiredVelocity/AvoidingVelocity`
- `FAvoiding`（`Avoidance.h:62`）：缓存友好数据 `Radius/CurrentVelocity/Position`，供邻居查询
- `FSphereObstacle`（`SphereObstacle.h:10`）：含 atomic lock、覆盖目标速度（`bOverrideSpeedLimit/NewSpeedLimit`）、`OverridingAgents`、`bStatic/bRegistered/bExcluded`
- `FBoxObstacle`（`BoxObstacle.h:9`）：RVO 边链表 `prevObstacle_/nextObstacle_`、4 个 `RVO::Vector2` 顶点、`unitDir_/height_`、凸性 `isConvex_`

### 2.6 生命 / 受击 / 死亡
- `FHealth`（`Health.h:9`）：`Current/Maximum/bLockHealth`，并发伤害队列（MPSC：`DamageToTake/DamageInstigator/HitDirection`）
- `FHealthBar`（`HealthBar.h:7`）：显示策略 `HideOnFullHealth/HideOnEmptyHealth`、`UseInterpolation/InterpSpeed`、运行时 `TargetRatio/CurrentRatio/Opacity`
- `FHit`（`Hit.h:11`）：发光 `bCanGlow`、受击动画 `bPlayAnim/AnimLength`、形变强度 `JiggleStr`、+ `SpawnActor/SpawnFx/PlaySound` 配置数组
- `FBeingHit`（`Hit.h:41`）：`GlowTime/JiggleTime/AnimTime` 与 reset 助手
- `FDeath`（`Death.h:11`）：`bEnable`、`DespawnDelay/FadeOutDelay/AnimLength`、`bCanFadeout/bCanPlayAnim`、`LifeSpan`（负值无限）+ `SpawnActor/SpawnFx/PlaySound`
- `FDying`（`Death.h:52`）：运行时时长、Instigator、命中方向

### 2.7 攻击 / 伤害 / 减益 / 抗性
- `FAttack`（`Attack.h:24`）：每轮持续 `DurationPerRound`、`MinAimTime/TimeOfHit/CoolDown`、`Range/AngleToleranceATK`、命中容差 `RangeToleranceHit/AngleToleranceHit`、击中时刻行为 `EAttackMode`（`Attack.h:15`：`None/ApplyDMG/SuicideATK/Despawn`）、`bCanPlayAnim`，并挂载 `SpawnProjectile/SpawnActor/SpawnFx/PlaySound` 数组
- `FAttacking`（`Attack.h:83`）：状态机 `EAttackState`（`BattleFrameEnums.h:45`：`Aim_FirstExec/Aim/PreCast/.../Cooling/Completed`）
- `FDamage`（`Damage.h:18`）：类型 `EDmgType`（`Damage.h:8`：`Normal/Fire/Ice/Poison`）、`Damage/PercentDmg`、`CritDmgMult/CritProbability`、`DmgRadius`、`bUseFalloff/bCheckObstacle`、`FBFFilter`
- 三种伤害形态变体：`FDamage_Point`（`Damage.h:54`）、`FDamage_Radial`（`Damage.h:93`）、`FDamage_Beam`（`Damage.h:145`，含 `DmgDirectionAndDistance`）
- `FDebuff`（`Debuff.h:62`）+ 子组件：
  - 击退 `FLaunchParams`（`Debuff.h:7`）：`bCanLaunch/LaunchSpeed`
  - 延时伤害 `FTemporalDmgParams`（`Debuff.h:22`）：`TemporalDmg/TemporalDmgSegment/TemporalDmgInterval`
  - 减速 `FSlowParams`（`Debuff.h:43`）：`SlowTime/SlowStrength`
- 三种 Debuff 变体：`FDebuff_Point/_Radial/_Beam`（`Debuff.h:84,116,152`）
- `FTemporalDamage`（`TemporalDamage.h:10`）+ `FTemporalDamaging`（`TemporalDamage.h:35`）：分段持续伤害状态（按 Instigator 聚合，含 atomic lock）
- `FSlow / FSlowing`（`Slow.h:9,24`）：减速来源集合 + 合成乘数 `CombinedSlowMult`
- `FDefence`（`Defence.h:7`）：分类型免疫 `Normal/Fire/Ice/Poison/Percent`、击飞免疫存活态/死亡态、`LaunchMaxImpulse`、`SlowImmune`、`bCanSlowATKSpeed`

### 2.8 出生 / 唤起
- `FAppear`（`Appear.h:11`）：`Delay/Duration`、`bCanDissolveIn/bCanPlayAnim` + Spawn 配置数组
- `FAppearing`（`Appear.h:44`）：`bInitialized/bStarted/Time/AnimTime/DissolveTime`

### 2.9 文本飘字 / 血条
- `FTextPopUp`（`TextPopUp.h:7`）：阈值色 `WhiteTextBelowPercent/OrangeTextAbovePercent`、`TextScale`
- `FPoppingText`（`TextPopUp.h:26`）：批量队列 `TextLocationArray` 与 `Value_Style_Scale_Offset_Array`（含 atomic lock）
- `FTextPopConfig`（`TextPopConfig.h:8`）：构造时填入 Owner/Value/Style/Scale/Radius/Location

### 2.10 配置型 Trait（生成 Actor/Fx/Sound/Projectile）
- `FActorSpawnConfig`（`ActorSpawnConfig.h:10`）/ `_Attack`（含 `ESpawnOrigin`）/ `_Final`（运行时态，含 `Owner/AttachTo/SpawnTransform/SpawnedActors`）
- `FSoundConfig`（`SoundConfig.h:12`）/ `_Attack`（含 `EPlaySoundOrigin_Attack`）/ `_Final`（含 SpawnedSounds 与转换构造）
- `FFxConfig`（`FxConfig.h:14`）/ `_Attack`/ `_Final`：双轨支持——合批（`EESubType` 指向 batched 渲染器）与非合批（`SoftNiagaraAsset/SoftCascadeAsset`），可附着
- `FSpawningProjectile`（`SpawningProjectile.h:10`）：主投射物 tag，存放 `ProjectileClass`
- `FSpawningFx`（`SpawningFx.h:11`）：附着 Niagara/Cascade 句柄、生命周期标记
- 投射物配置（`ProjectileConfig.h`）：
  - `FProjectileParams`（`:42`）：`Radius/LifeSpan/Health`、`FBFFilter`、一系列移除条件 `bRemoveOn*`、`bTraceOnlyOnArrival/bHurtTargetOnlyOnce`
  - `FProjectileParamsRT`（`:90`）：`Instigator/IgnoreSubjects/NeighborGridComponent`
  - 移动模式四种：`FProjectileMove_Static/Interped/Ballistic/Tracking`（`:111/120/203/254`）+ 配对运行时态 `FProjectileMoving_*`
  - 抛物线含 `EProjectileSolveMode` 仰角解/初速解 + 高/平抛选项
  - 解算精度 `Iterations` 用于运动预测
  - `FProjectileMultipliers_*` 4 种缩放/速度/重力乘数
  - `FProjectileConfig` / `_Final` 顶层配置（数量、附着、原点、起止点、目标）
- `FCurves`（`Curves.h:8`）：四条曲线 `DissolveIn/Out`、`HitEmission`、`HitJiggle`，使用 RuntimeFloatCurve

### 2.11 渲染与批量数据
- `FAnimation`（`Animation.h:12`）：`AnimToTextureDataAsset`（VAT 资产）、`RendererClass`（`ANiagaraSubjectRenderer` 子类）、Idle/Move 之间的 BlendSpace `BS_IdleMove`、各动画索引 `IndexOfIdle/Move/Appear/Attack/Hit/Death/Fall`、随机时间偏移
- `FAnimating`（`Animation.h:74`）：三槽蒙太奇（slot 0/1/2 的 `AnimIndex/PlayRate/CurrentTime/OffsetTime/PauseFrame`）+ Lerp 两路 + Particle Color 通道（`Team/HitGlow/Dissolve/IceFx/FireFx/PoisonFx` + 插值副本）+ `EAnimState` 状态机（`BattleFrameEnums.h:160`：`BS_IdleMove/Appearing/Sleeping/Attacking/BeingHit/Dying/Falling/Jumping`）
- `FRendering`（`Rendering.h:10`）：`InstanceId` + 关联 Renderer Subject
- `FAgentRenderBatchData`（`RenderBatchData.h:15`）：合批 Niagara 渲染所需的全部 SoA 数组：`LocationArray/OrientationArray/ScaleArray/AnimIndex_PauseFrame_Playrate_MatFx_Array/AnimTimeStamp_Array/AnimLerp0_AnimLerp1_Team_Dissolve_Array/HealthBar_Opacity_CurrentRatio_TargetRatio_Array/Text_Location_Array/Text_Value_Style_Scale_Offset_Array`，含池化 `Transforms/ValidTransforms(FBitMask)/FreeTransforms`
- `FFxRenderBatchData`（`RenderBatchData.h:128`）：合批特效用 SoA，分 Attached / Burst 两类数据轨道

### 2.12 关系 / 网格
- `FOwnerSubject`（`OwnerSubject.h:8`）：`Owner / Host` 双向引用（关联 Spawner Agent 与寄主 Actor）
- `FGridData`（`GridData.h:12`）：32 字节缓存对齐结构，记录 `SubjectHash/Location/Radius/SubjectHandle/DistSqr`，支持以 Hash 作为 `GetTypeHash`
- `FStatistics`（`Statistics.h:8`）：`TotalDamage/TotalHeal/TotalKills/TotalScore/TotalTime`，atomic lock

### 2.13 碰撞 / Collider
- `FCollider`（`Collider.h:8`）：`Radius`、`bHightQuality`（巨型单位注册到全部相交格）、`bDrawDebugShape`

---

## 3. 核心战斗控制系统 `ABattleFrameBattleControl`

声明：`Public/BattleFrameBattleControl.h:77`；实现：`Private/BattleFrameBattleControl.cpp`（8197 行）。

### 3.1 顶层结构
- 单例：`Instance` 静态 + `GetInstance()` 蓝图静态（`BattleFrameBattleControl.h:98, 207`）
- 多线程配置：`MaxThreadsAllowed/MinBatchSizeAllowed/ThreadsCount/BatchSize`（`:84-90`）
- `bGamePaused`、`AgentCount`（`:93,96`）
- 注册的邻居网格数组 `NeighborGrids`（`:101`）
- `FStreamableManager` 用于软资产异步加载（`:104`）
- 13 个 `EFlagmarkBit` 子状态位（`:107-121`）：`AppearDissolve/DeathDissolve/HitGlow/HitJiggle/HitPoppingText/HitDecideHealth/DeathDisableCollision/RegisterMultiple/AppearAnim/AttackAnim/HitAnim/DeathAnim/FallAnim`
- 4 类延迟生成队列（MPSC）：`ProjectileConfigQueue / ActorSpawnConfigQueue / FxConfigQueue / SoundConfigQueue`（`:124-127`）
- 6 类事件回调队列（MPSC）：`OnAppearQueue / OnTraceQueue / OnMoveQueue / OnAttackQueue / OnHitQueue / OnDeathQueue`（`:130-135`）
- 6 类 debug 图元队列：`DebugPointQueue/LineQueue/SphereQueue/CapsuleQueue/SectorQueue/CircleQueue`（`:138-143`）
- 30+ 预定义 `FFilter`（`:149-180`），在 `DefineFilters()` 一次性构造（`BattleFrameBattleControl.h:212`）

### 3.2 Tick 阶段（实现于 `BattleFrameBattleControl.cpp::Tick`，逐节注释清晰）

依次（行号即每阶段 `TRACE_CPUPROFILER_EVENT_SCOPE_STR` 起点）：

1. **数据统计**（`Tick` `:72-118`）：累计存活时长等
2. **出生 Appear**（`:119-253`）：延迟、淡入溶解、出生动画状态切换、SpawnActor/Fx/Sound 入队
3. **移动 Move** 大块：
   - Sleep（`:259`）：休眠状态切换
   - Patrol（`:291`）：巡逻状态机，目标点查找 `FindNewPatrolGoalLocation`（`BattleFrameBattleControl.h:214`）
   - SpeedLimitOverride（`:363`）：球形障碍物覆写 agent 速度上限
   - AgentMove 主体（`:448` 起）含子节：FlowField 取流速、Move State Machine、Desired Direction（导航）、Desired Speed（速度映射）、Desired Velocity、Launched（被击飞）、Final Velocity（避障 → ORCA）、Velocity Z（含 SphereTraceForGround `:1121`）、New Location、Orientation（`:1300`）
4. **NeighborGrid 更新**（`:1458`）：在 Move 后写入网格
5. **攻击 Attack** 大块：
   - AgentTrace（`:1475`）：索敌
   - AgentAttackTrigger（`:2011`）：发起攻击
   - AgentAttacking（`:2083`）：状态机推进（Aim→PreCast→PostCast→Cooling），击中时刻触发伤害/Debuff、SpawnProjectile/Actor/Fx/Sound
6. **投射物 Projectile**：
   - SpawnProjectile（`:2513`）
   - Projectile Move and Dmg（`:2628`）：四种运动模式更新位置 + 碰撞检测（按 `FProjectileParams.Filter`）+ 应用伤害
7. **受击 Hit**（`:3005`）：
   - 受击发光（`:3144`）
   - 受击形变（`:3180`）
   - 受击动画（`:3217`）
8. **减速生效**（AgentSlowed `:3249`）：聚合 `Slowing.Slows` → `CombinedSlowMult`
9. **延时伤害**（AgentTemporalDamaging `:3378`）：分段持续伤害推进
10. **死亡**（AgentDeath `:3586`）：寿命到/血量到/`KillZ`/`SuicideAttack` → 进入 `FDying`
11. **游戏线程逻辑**：从队列拉取
    - SpawnActors（`:3719`）
    - SpawnFx（`:3845`）
    - PlaySound（`:4018`）
    - EventInterface 派发（`:4149`）：把 `OnAppear/Trace/Move/Attack/Hit/Death` 派给实现了 `IBattleFrameInterface` 的 Subject Owner（接口定义 `BattleFrameInterface.h`）
12. **渲染**：
    - Agent Anim State Machine（`:4382`）
    - ClearValidTransforms（`:4603`）
    - AgentRender（`:4624`）：写入合批数组
    - WritePoolingInfo（`:4713`）
    - SendDataToNiagara（`:4738`）：通过 `UNiagaraDataInterfaceArrayFunctionLibrary` 把 SoA 推送给 Niagara

### 3.3 伤害应用 API（实现节 `Damager` `BattleFrameBattleControl.cpp:5219-7437`）

成对的"立即/延迟"接口（避免在并行 ECS 迭代中改 Subject）：
- 点伤害：`ApplyPointDamageAndDebuff` / `…Deferred`（`.h:356-358`）
- 球形：`ApplyRadialDamageAndDebuff` / `…Deferred`（`.h:360-362`）—— 通过 `UNeighborGridComponent` 球形检索，可 `KeepCount` 限定数量
- 球扫：`ApplyBeamDamageAndDebuff` / `…Deferred`（`.h:364-366`）

每个调用内部依次执行：抗性扣除（`:5277` 区）→ Debuff 入队（`:5379` 区）→ 击退/延时伤害/减速 → 视觉效果（弹字、HitGlow、Jiggle、Hit 动画 flag）→ 写入 Statistics

### 3.4 寻路（A*）
- `FindPathAStar(AFlowField*, StartLocation, GoalLocation, OutPath)`（`.h:429`，实现 `:7443`）
- 路径跟随 `GetSteeringDirection(...)`（`.h:431`，实现 `:7642`）：返回 `SteeringDirection`、`bHasPath/bIsCurrentNearPath/bIsGoalNearEnd`
- 辅助 `FindClosestPointOnSegment`（`.h:433`）
- 流场高度插值 `GetInterpedWorldLocation(AFlowField*, location, angleThreshold, outLoc)`（`.h:216`）

### 3.5 RVO2-2D 求解器（移植自 EastFoxStudio）
- 入口 `ComputeAvoidingVelocity(FAvoidance&, FAvoiding&, SubjectNeighbors, ObstacleNeighbors, TimeStep)`（`.h:438`，实现 `:7724`）
- 线性规划三层：`LinearProgram1/2/3`（`.h:440-444`，实现 `:8043,:8117,:8149`）
- 基础数学结构：`RVO::Vector2`（`Public/RVOVector2.h`）、`RVO::Line`（`Public/RVOSimulator.h:28`）、宏 `RVO_ERROR`（`RVOSimulator.h:23`）
- `RVODefinitions.h` 提供常量
- `EAvoidMode { RVO2, PBD }`（`Avoidance.h:11`）—— 同时为 PBD（Position-Based Dynamics）保留分支

### 3.6 内联工具函数（位于 `BattleFrameBattleControl.h` 头）
- `PlayAnimAsMontage(...)`（`:222`）：三槽动画蒙太奇切换 + 过渡 Lerp 计算
- `ProcessCritDamage(BaseDamage, mult, prob)`（`:285`）：暴击判定
- `LocalOffsetToWorld(...)`（`:304`）：局部 Transform→世界
- `QueueText(FTextPopConfig)`（`:316`）：飘字并发入队
- `ResetPatrol(...)`（`:334`）：巡逻原点切换
- Pack 数据为 float：`EncodeAnimationIndices/EncodePauseFrames/EncodePlayRates/EncodeStatusEffects`（`:372,386,400,413`）—— 用于压缩三槽动画参数到 Niagara Dynamic Param 通道
- 静态 debug 工具 `DrawDebugSector`（`:218`）
- `CopyPasteAnimData(Animating, From, To)`（`:220`）

---

## 4. 生成系统（`AAgentSpawner` + DataAsset）

### 4.1 `AAgentSpawner`
- 声明 `Public/AgentSpawner.h:23`，实现 420 行 `Private/AgentSpawner.cpp`
- 公开 API：
  - `SpawnAgentsByConfigRectangular(bAutoActivate, DataAsset, Quantity, Team, Origin, Region, LaunchVelocity, EInitialDirection, CustomDirection, FSpawnerMult)` 返回 `TArray<FSubjectHandle>`（`AgentSpawner.h:55-67`）—— 矩形区域随机生成
  - `ActivateAgent(FSubjectHandle)` （`:70`，蓝图）
  - `KillAllAgents()`（`:73`）
  - `KillAgentsBySubtype(int32 Index)`（`:76`）
- 初始方向枚举 `EInitialDirection`（`BattleFrameEnums.h:37`）：`FacePlayer / FaceForward / CustomDirection`
- 数量/速度/血量/尺寸的整体乘数：`FSpawnerMult { HealthMult/MoveSpeedMult/DamageMult/ScaleMult }`（`BattleFrameStructs.h:166`）
- 共用 EFlagmarkBit 子状态位与 BattleControl 对齐（`AgentSpawner.h:39-53`）

### 4.2 `UAgentConfigDataAsset`（主类型 Agent 模板）
位置：`Public/AgentConfigDataAsset.h:42`。把 24 类 Trait 打包为蓝图可编辑资产：
`Agent/SubType/Scale/Collider/Health/Animation/Appear/Trace/Sleep/Patrol/Chase/Move/Fall/Navigation/Avoidance/Attack/Damage/Debuff/Hit/HealthBar/TextPop/Defence/Death/Curves/Statistics` + 任意 `ExtraTraits: FSubjectRecord`（`:50-125`）。

### 4.3 `UProjectileConfigDataAsset`
位置：`Public/ProjectileConfigDataAsset.h:25`。
- 通用：`FProjectile / FSubType / FProjectileParams`（`:31-38`）
- 运动模式枚举 `EProjectileMoveMode { Static, Interped, Ballistic, Tracking }`（`BattleFrameEnums.h:174`） + 四个 EditCondition 互斥参数块（`:46-56`）
- 伤害模式枚举 `EProjectileDamageMode { Point, Radial, Beam }`（`BattleFrameEnums.h:183`） + 三组 Damage/Debuff 参数块（`:64-81`）
- 运行时缓存（不暴露给编辑器）：`FProjectileParamsRT/Moving_*/Located/Directed/Scaled`（`:84-92`）

---

## 5. 空间查询系统（NeighborGrid）

### 5.1 `UNeighborGridComponent`
继承 `UMechanicalActorComponent`，声明 `Public/NeighborGridComponent.h:48`，实现 1487 行。

配置：
- `CellSize`（默认 `300x300x300`）、`GridSize`（默认 `20x20x1`）（`:69-72`）
- `MaxThreadsAllowed/MinBatchSizeAllowed`（`:54-58`）
- `bDebugDrawCageCells`（编辑器，`:64`）

三组缓存：
- `SubjectCells / ObstacleCells / StaticObstacleCells`（`:77-79`）
- 每线程 MPSC 占用队列 `OccupiedCellsQueues`（`:82`）
- `InvCellSizeCache`

公开 Trace API（实现 0–1185 行段）：
- `SphereTraceForSubjects`（`:142`）—— 点为圆心
- `SphereSweepForSubjects`（`:159`）—— 起止点 + 半径
- `SectorTraceForSubjects`（`:177`）—— 圆心 + 扇形高度/角度/朝向
- 全部接受 `KeepCount / bCheckObstacle / CheckOrigin/CheckRadius / ESortMode / SortOrigin / IgnoreSubjects / FBFFilter / FTraceDrawDebugConfig` 公共参数

更新与初始化：
- `Update()`（`:207`，实现 `NeighborGridComponent.cpp:1186`）三段：`RegisterSubject`（`:1211`，过滤 `FLocated/FScaled/FCollider/FGridData/FActivated`，排除 `FSphereObstacle`）、`RegisterSphereObstacles`（`:1313`）、`RegisterBoxObstacles`（`:1395`）
- `DoInitializeCells()`（`:121`）按 `GridSize` 预分配；`DefineFilters()`（`NeighborGridComponent.cpp:42`）
- 共 11 个预定义 FFilter（`:102-112`）

工具内联：
- 球扫格 `SphereSweepForCells(Start, End, Radius)` 使用 3D Bresenham + 按距离排序（`:240-330`）
- `GetNeighborCells(Center, Range3D)`（`:214`）AABB → 格集合
- `AddSphereCells(...)`（`:332`）分层椭球生成
- 坐标转换：`CoordToLocation/LocationToCoord/CoordToIndex/IndexToCoord/LocationToIndex/GetCellAt`（`:383-444`）
- `IsInside(Coord/Location)`（`:370-378`）
- 障碍物子状态位与 BattleControl 一致（`:85-99`）

### 5.2 `ANeighborGridActor`
位置 `Public/NeighborGridActor.h:30`。仅作为容器 Actor 内嵌一个 `UNeighborGridComponent`，对外暴露 `GetComponent()`（`:63`）。

### 5.3 `FNeighborGridCell`
`Public/NeighborGridCell.h:26`。每个格的存储：
- `FFingerprint`（指纹，用于快速类型过滤）
- 内联分配 `TArray<FGridData, TInlineAllocator<16>> Subjects`
- `bRegistered` 标志位
- atomic lock，避免多线程写冲突

---

## 6. 避障系统（RVO/ORCA + 障碍物）

### 6.1 算法
位于 `BattleFrameBattleControl.cpp:7722-8200`，移植自 EastFoxStudio 的 RVO2-2D。Trait 侧 `FAvoidance` 缓存 `OrcaLines` 与 `Desired/AvoidingVelocity`，由 `ComputeAvoidingVelocity` 内 `LinearProgram1/2/3` 求解半平面交。

### 6.2 障碍物 Actor
- `ARVOSphereObstacle`（`Public/RVOSphereObstacle.h:20`）：包 `USphereComponent` + `SubjectHandle`；可动态 `bIsDynamicObstacle`、可覆盖被推单位的速度上限 `bOverrideSpeedLimit/NewSpeedLimit`。实现 `Private/RVOSphereObstacle.cpp`（77 行）每 Tick 把 actor 位置/半径写回 `FSphereObstacle`
- `ARVOSquareObstacle`（`Public/RVOSquareObstacle.h:20`）：四边形障碍，每条边对应一个 `FSubjectHandle`（`Obstacle1…4`）；支持 `bInsideOut`（凹包反向）。实现 252 行

---

## 7. Niagara 批量渲染系统

### 7.1 Agent 渲染：`ANiagaraSubjectRenderer`
- 声明 `Public/NiagaraSubjectRenderer.h:37`，实现 248 行
- 配置：`FAgent` + `FSubType{Index}` 双 Trait 过滤、`RenderBatchSize`（默认 1000）、`Scale/OffsetLocation/OffsetRotation`、`NiagaraSystemAsset/StaticMeshAsset`（`:59-83`）
- 方法：
  - `Register()`（`:50`）从 BattleControl 注册自己
  - `IdleCheck()`（`:52`）—— 池化清理
  - `AddRenderBatch()/RemoveRenderBatch(handle)`（`:54-56`）—— 子 batch（Subject）作为渲染单元
- 缓存 `Mechanism/BattleControl/CurrentWorld` 与已派发的 `SpawnedRenderBatches`

### 7.2 Fx 渲染：`ANiagaraFXRenderer`
- 声明 `Public/NiagaraFXRenderer.h:28`，实现 313 行
- 配置：`NiagaraSystemAsset`、`TraitType/SubType`（双 UScriptStruct 过滤，匹配任意带这两个 trait 的 fx）、`PoollingCoolDown`、`RenderBatchSize`（默认 500）、并发参数（`:48-70`）
- 工作模式枚举 `EFxMode { InPlace, Attached }`（`:17-21`，源码里默认 InPlace）
- 通过 BattleControl Tick 的"SpawnFx"阶段送入合批

### 7.3 工作机制
BattleControl::Tick `SendDataToNiagara`（`BattleFrameBattleControl.cpp:4738`）把所有合批 SoA 数组（位置、朝向、缩放、动画三槽 PackFloat、Particle Color 通道、血条三元、TextPopUp 文本与样式）通过 `UNiagaraDataInterfaceArrayFunctionLibrary` 推送到 Niagara System；Niagara 侧脚本据此驱动 VAT（顶点动画贴图）模型。
- Pack 通道对照（编码函数见 §3.6）：
  - DynamicParam0 = AnimIndex/PauseFrame/PlayRate × 3 槽
  - DynamicParam1 = AnimTimeStamp × 3 槽
  - ParticleColor = AnimLerp0/AnimLerp1/Team/Dissolve
  - MatFx 通道压 4 个状态效果（HitGlow/Frozen/Burning/Poisoned）

---

## 8. Actor ↔ Subject 桥接组件

### 8.1 `UBFSubjectiveActorComponent`
- 位置 `Public/BFSubjectiveActorComponent.h:10`，实现 94 行
- 继承自 Apparatus 的 `USubjectiveActorComponent`
- 关键流程：`InitializeSubjectTraits(AActor*)`（`:18`）、Tick 内 `SyncTransformActorToSubject(AActor*)`（`:25`，私有）将 Actor 位移同步到 Subject 的 `FLocated`

### 8.2 `UBFSubjectiveAgentComponent`
- 位置 `Public/BFSubjectiveAgentComponent.h:11`，实现 327 行
- 提供"以 DataAsset 初始化 Agent 全部 Trait"的能力（蓝图）：
  - `InitializeSubjectTraits(bAutoActivation, OwnerActor)`（`:19`，自动加载 `AgentConfigAsset` 后写入 Subject）
  - `ActivateAgent(FSubjectHandle)`（`:22`）
  - `SyncTransformSubjectToActor(AActor*)`（`:25`，与 actor 形态绑定）
- 蓝图字段：`AgentConfigAsset / TeamIndex / LaunchVelocity / Multipliers / bAutoInitWithDataAsset / bSyncTransformSubjectToActor`（`:28-44`）
- 同样持有完整 EFlagmarkBit 集合

---

## 9. 蓝图函数库 `UBattleFrameFunctionLibraryRT`

声明：`Public/BattleFrameFunctionLibraryRT.h:31`，实现 3187 行。

### 9.1 生成
- `SpawnAgentsByConfigRectangular(AAgentSpawner*, …)`（`:40`）—— 包装 AgentSpawner 的同名方法

### 9.2 伤害与减益（通过 BattleControl）
- `ApplyPointDamageAndDebuff`（`:58`）
- `ApplyRadialDamageAndDebuff`（`:72`）
- `ApplyBeamDamageAndDebuff`（`:88`）
- 输出 `TArray<FDmgResult>`（`BattleFrameStructs.h:45`：`DamagedSubject/InstigatorSubject/CauserSubject/IsCritical/IsKill/DmgDealt`）

### 9.3 杂项
- `SortSubjectsByDistance(TraceResults, SortOrigin, ESortMode)`（`:107`）
- `CalculateThreadsCountAndBatchSize(IterableNum, MaxThreadsAllowed, MinBatchSizeAllowed, OutThreads, OutBatch)`（`:109`）

### 9.4 导航辅助
- `WorldToIndexByRadius(AFlowField*, Location, Radius)`（`:115`，内联实现）—— 返回与圆相交的格子 index 数组
- `WorldToGirdByRadius(...)`（`:173`，内联）—— 返回 2D 网格坐标

### 9.5 投射物
- 弹道解算：
  - `SolveProjectileVelocityFromPitch(...)`（`:234`）
  - `SolveProjectileVelocityFromSpeed(...)`（`:236`）
  - 带目标速度预测：`...FromPitchWithPrediction` / `...FromSpeedWithPrediction`（`:240,243`）
- 子弹生成：
  - `SpawnProjectileByConfig(...)`（`:245`，C++ 内部）
  - 四种独立蓝图入口对应四种 MoveMode：`SpawnProjectile_Static / _Interped / _Ballistic / _Tracking`（`:248-257`）
- 子弹位置更新（C++ 内部）：`GetProjectilePositionAtTime_Interped / _Ballistic / _Tracking`（`:259,261,263`）

### 9.6 同步 Trace
- `SphereTraceForSubjects(...)`（`:269`）
- `SphereSweepForSubjects(...)`（`:288`）
- `SectorTraceForSubjects(...)`（`:308`）
- 全部支持 `KeepCount/CheckObstacle/CheckRadius/ESortMode/IgnoreSubjects/FBFFilter/FTraceDrawDebugConfig`

### 9.7 适配器（自动 Cast）
- `ConvertDmgResultsToSubjectHandles / TraceResultsToSubjectHandles`（`:344,347`）
- `ConvertDmgResultsToSubjectArray / TraceResultsToSubjectArray / SubjectHandlesToSubjectArray`（`:350,353,356`）

### 9.8 Trait Setter（按 Index 切 tag，避免冗长 switch）
位于头文件 `:361-697`，覆盖 `SubType / AvoGroup / Team`：
- `SetRecordSubTypeTraitByIndex/Enum`、`RemoveSubjectSubTypeTraitByIndex/Set...`、`IncludeSubTypeTraitByIndex`
- `RemoveSubjectAvoGroupTraitByIndex / Set...`、`Set/IncludeRecord...`、`Include/ExcludeAvoGroupTraitByIndex`
- `RemoveSubjectTeamTraitByIndex / Set...`、`SetRecordTeamTraitByIndex`

### 9.9 异步 Trace
- `USphereSweepForSubjectsAsyncAction`（`:706`，`UBlueprintAsyncActionBase`）—— `SphereSweepForSubjectsAsync(...)` 蓝图节点
- 完成委托：`FAsyncTraceOutput(bool Hit, TArray<FTraceResult>& Results)`（`:703`）

---

## 10. 事件接口 `IBattleFrameInterface`

`Public/BattleFrameInterface.h:15`。蓝图原生事件，由 BattleControl 的 EventInterface 阶段派发到 Subject 关联的 Owner Actor：
- `OnAppear(FAppearData)`（`:22`） / `OnTrace(FTraceData)`（`:26`） / `OnMove(FMoveData)`（`:29`） / `OnAttack(FAttackData)`（`:32`） / `OnHit(FHitData)`（`:35`） / `OnDeath(FDeathData)`（`:38`）
- 事件数据结构定义于 `BattleFrameStructs.h:231-336`，含 SelfSubject、状态枚举（`ETraceEventState/EMoveEventState/EAttackEventState/EHitEventState/EDeathEventState`，见 `BattleFrameEnums.h:113-157`）、DmgResults

---

## 11. 编辑器模块 `BattleFrameEditor`

- 模块入口：`Private/BattleFrameEditor.cpp`（含 `IMPLEMENT_MODULE(FBattleFrameEditorModule, BattleFrameEditor)`）
- 功能 1：**注册 NeighborGrid 可视化器** `FNeighborGridComponentVisualizer`（`Public/NeighborGridComponentVisualizer.h`、`Private/.cpp`）—— 重写 `DrawVisualization` 在编辑器中绘制网格盒
- 功能 2：**资产蓝图函数库** `UBattleFrameFunctionLibrary`（`Public/BattleFrameFunctionLibrary.h`）：
  - `DuplicateClassAsset(WorldContextObject, SourceClass, NewClassName, PackagePath)`：编辑器内蓝图类复制
  - `SetClassDefaultProperties(WorldContextObject, TSubclassOf<ANiagaraSubjectRenderer>, NewMesh, NewNiagaraSystem, int32 SubType)`：批量改 Renderer 默认值
- 依赖：`UnrealEd, AssetTools, BlueprintGraph`

---

## 12. 内置内容资产分类

目录 `BattleFrame/Content/`：

- `Content/BattleFrameTools.uasset`（顶层工具资产）
- `Content/Core/`
  - `AgentRenderer/`
    - `HealthBar/`、`TextPopUp/`、`RingDecal/`、`NS_Modules/`（共用 Niagara 模块）
    - `VAT/`（VAT_Master.uasset + `MF/`、`Tex/`，提供顶点动画贴图工作流的母材质与示例贴图）
  - `FxRenderer/`（特效合批渲染器示例）
  - `NeighborGrid/`（邻居网格示例资产）
- `Content/Demo/`
  - `Agent/`
    - `ActorToSpawn/AppearDecal`、`AttackDecal`、`DragonFlame`（生成型 actor 示例）
    - `AgentAsset/ChestMonster`、`Dragon`、`TurtleShell`（完整示例：每个含 `AgentConfig_xxxx.uasset` DataAsset、`Renderer_xxxx`、`SM_xxxx` Mesh、`DA_xxxx` AnimToTexture 数据、`VAT_xxxx_VertPositionTexture/VertNormalTexture`、若干动画与材质，示例见 `Demo/Agent/AgentAsset/Dragon/`）
    - `DeathBones/Assets`（死亡骨骼示例）
  - `Player/Asset`：`CameraShake / Input / Mesh / Skill / UI`
  - `Projectile/Renderer`（投射物示例）
  - `Prop/`（道具示例）

---

## 13. 关键概念总结（对外可用面）

1. **核心运行时类**（在场景中各放一个）：`ABattleFrameBattleControl`、`ANeighborGridActor`、`AAgentSpawner`
2. **Actor 端集成**：在玩家或寄主 Actor 上挂 `UBFSubjectiveActorComponent` 或 `UBFSubjectiveAgentComponent`
3. **配置端**：制作 `UAgentConfigDataAsset`、`UProjectileConfigDataAsset` 资产，绑定 VAT `UAnimToTextureDataAsset` 与对应 `ANiagaraSubjectRenderer` 子类
4. **运行时调用**：通过 `UBattleFrameFunctionLibraryRT` 暴露的蓝图节点完成生成 / 索敌 / 伤害 / 子弹发射 / 异步 sweep
5. **回调**：在 Subject Owner Actor 上实现 `IBattleFrameInterface` 的 6 个事件
6. **障碍物**：放置 `ARVOSphereObstacle` / `ARVOSquareObstacle` 即可自动参与避障
7. **性能开关**：每个 Tick 子模块均使用 `TRACE_CPUPROFILER_EVENT_SCOPE_STR`（Unreal Insights 可见），并行批量大小由 `MaxThreadsAllowed/MinBatchSizeAllowed/CalculateThreadsCountAndBatchSize` 控制
