# BattleFrame Agent 状态机

本文档梳理 `Plugins/BattleFrame/Source/BattleFrame/Private/BattleFrameBattleControl.cpp` 中 Agent (entity) 的所有状态与转换。Agent 同时拥有 **多套并行状态系统**——它们正交叠加，而非单一状态机。

---

## 1 · Agent 拥有的状态总览

按"作用层"划分为四套子状态系统：

### 1.1 生命周期 (Lifecycle, tag trait)

| Trait | 含义 | 进入 | 退出 |
|---|---|---|---|
| `FAppearing` | 出生剧本进行中 | `AgentSpawner.cpp:292` 若 `Appear.bEnable` | `cpp:247` `Appearing.Time ≥ Appear.Delay + Duration` |
| *（无标记）* | 正常存活期 | `FAppearing` 移除后 | 进入 `FDying` 或被 Despawn |
| `FDying` | 死亡剧本进行中 | `cpp:102` (LifeSpan 超时) / `cpp:3064` (Health ≤ 0) | Death region 结束后 `Subject.Despawn()` |
| `FActivated` | 已激活、参与所有系统 | `AgentSpawner.cpp:346` 出生收尾 | 无（终身持有） |

### 1.2 行为模式 (Behavior tag trait — 决定 MoveState 走哪个分支)

| Trait | 进入条件 | 退出条件 |
|---|---|---|
| `FSleeping` | Spawn 时 `Sleep.bEnable` (`AgentSpawner.cpp:303`) | `!Sleep.bEnable` 或 `Tracing.TraceResult.IsValid()` (`cpp:270-280`) |
| `FPatrolling` | Spawn 时 `Patrol.bEnable` (`AgentSpawner.cpp:312`)；或索敌失败 + `Patrol.OnLostTarget == Patrol` (`cpp:1992-1998`) | `!Patrol.bEnable` 或 `Tracing.TraceResult.IsValid()` (`cpp:310-320`) |
| `FAttacking` | Attack Trigger 命中可攻击目标 (`cpp:2061`) | 目标失效 / 超出射程 / 攻击完成 (`cpp:2117, 2165, 2472`) |
| `FBeingHit` | 被击中时由 Hit 系统设置 (`cpp:5256-5270`) | 全部受击子动效完成 (`cpp:3236-3240`) |

### 1.3 移动状态 `EMoveState` (`FMoving::MoveState`)

枚举位置：`BattleFrameEnums.h:57`。共 8 个值（1 个无效 + 7 个运行态）：

| MoveState | Tooltip | 进入条件（在 `cpp:515-635` 状态机中） |
|---|---|---|
| `Dirty` | 无效数据 | 初始默认值 |
| `Sleep_Sleeping` | 休眠中 | `bIsSleeping` (有 `FSleeping`) |
| `Patrol_Patrolling` | 巡逻中 | `bIsPatrolling` 且 `Dist > Patrol.AcceptanceRadius` |
| `Patrol_Waiting` | 巡逻点等待 | `bIsPatrolling` 且 `Dist ≤ Patrol.AcceptanceRadius` |
| `Chase_Chasing` | 追逐中 | `Chase.bEnable && bHasValidTraceResult` 且超过 `Chase.AcceptanceRadius` |
| `Chase_Reached` | 已追到 | `Chase.bEnable && bHasValidTraceResult` 且在 `Chase.AcceptanceRadius` 内 |
| `Approach_Approaching` | 前往目标 | 默认分支，未到 `Move.XY.AcceptanceRadius` |
| `Approach_Arrived` | 已抵达 | 默认分支，已到 `Move.XY.AcceptanceRadius` |

> 行为模式 trait 的优先级写死在 `if/else if` 顺序里：`Sleeping ▶ Patrolling ▶ Chasing ▶ Approach`。

### 1.4 攻击子状态 `EAttackState` (`FAttacking::State`)

枚举位置：`BattleFrameEnums.h:45`。攻击进行时的内部阶段：

| State | 含义 | 进入 (cpp 行号) |
|---|---|---|
| `Aim_FirstExec` / `Aim` | 瞄准（首帧/后续） | `FAttacking` 设置时初始为 Aim (`2061`) |
| `PreCast_FirstExec` / `PreCast` | 前摇 | `2183, 2190` |
| `PostCast` | 后摇（命中判定） | `2353` 当 `ATKTime ≥ Attack.TimeOfHit` |
| `Cooling` | 冷却 | `2453` 当 `ATKTime == Attack.DurationPerRound` |
| `Completed` | 完成 | `2470` 冷却到时 → 同步 `RemoveTrait<FAttacking>` |

### 1.5 其它布尔状态位 (Flag)

| Flag | 作用 |
|---|---|
| `AppearAnimFlag` / `AppearDissolveFlag` | 出生动画 / 溶入效果开关 (`cpp:198, 204`) |
| `HitAnimFlag` | 受击动画进行中（短路 Attack Trigger）(`cpp:2028, 3225`) |
| `HitJiggleFlag` | 受击形变进行中 (`cpp:3209`) |

---

## 2 · 状态转换流程图

### 2.1 生命周期主流程

```mermaid
stateDiagram-v2
    [*] --> Spawning : AgentSpawner
    Spawning --> Appearing : "FAppear.bEnable == true"
    Spawning --> Alive : "FAppear.bEnable == false"
    Appearing --> Alive : "Appearing.Time ≥ Delay + Duration<br/>(cpp:247)"
    Alive --> Dying : "Health.Current ≤ 0<br/>(cpp:3064)"
    Alive --> Dying : "Stats.TotalTime > Death.LifeSpan<br/>(cpp:100-102)"
    Alive --> [*] : "Fall.KillZ 触发<br/>DespawnDeferred (cpp:482)"
    Dying --> [*] : "Death 剧本播完<br/>Subject.Despawn() (Death region)"
```

### 2.2 移动状态机 `EMoveState`

```mermaid
stateDiagram-v2
    [*] --> Dirty : 默认值

    state "Sleep_Sleeping" as Sleep
    state "Patrol_Patrolling" as PatP
    state "Patrol_Waiting" as PatW
    state "Chase_Chasing" as ChaC
    state "Chase_Reached" as ChaR
    state "Approach_Approaching" as AppA
    state "Approach_Arrived" as AppD

    Dirty --> Sleep : bIsSleeping

    Sleep --> PatP : "!FSleeping<br/>(被 TraceResult 唤醒)"
    Sleep --> AppA : "!FSleeping 且无 Patrol"

    PatP --> PatW : "Dist ≤ Patrol.AcceptanceRadius"
    PatW --> PatP : "重新选点后 Dist > AcceptanceRadius"
    PatP --> ChaC : "索敌命中<br/>(FPatrolling 被移除, Chase.bEnable)"
    PatW --> ChaC : 索敌命中
    PatP --> Sleep : "Sleep.bEnable 重新生效 (罕见)"

    ChaC --> ChaR : "Dist - Radii ≤ Chase.AcceptanceRadius"
    ChaR --> ChaC : "目标远离, Dist 超出 AcceptanceRadius"
    ChaC --> PatP : "索敌失败 + OnLostTarget==Patrol<br/>(cpp:1992-1998)"
    ChaC --> AppA : "索敌失败 + OnLostTarget==Move"
    ChaR --> AppA : 同上

    AppA --> AppD : "Dist ≤ Move.XY.AcceptanceRadius"
    AppD --> AppA : "目标变更, Dist 超出 AcceptanceRadius"
    AppA --> ChaC : 索敌命中
    AppD --> ChaC : 索敌命中

    note right of Sleep
        分支判定 (cpp:534-635):
        Sleeping ▶ Patrolling ▶
        Chasing ▶ Approach
    end note
```

**转换的判定层 (`cpp:515-635`)**：状态机每帧重算，依赖三个输入

```
bIsSleeping  = HasTrait<FSleeping>()
bIsPatrolling = HasTrait<FPatrolling>()
bIsChasing   = Chase.bEnable && Tracing.TraceResult.IsValid()
```

按 `if/else if` 顺序选大类，再用 `Dist vs AcceptanceRadius` 选 `_ing/_ed` 子状态。

### 2.3 行为模式 trait 流转（决定 MoveState 走哪条分支）

```mermaid
stateDiagram-v2
    [*] --> Sleeping : Spawn 时 Sleep.bEnable
    [*] --> Patrolling : Spawn 时 Patrol.bEnable
    [*] --> Idle : 否则

    state "FSleeping" as Sleeping
    state "FPatrolling" as Patrolling
    state "无行为 trait" as Idle

    Sleeping --> Idle : "TraceResult 命中<br/>(cpp:276-280)"
    Sleeping --> Idle : "!Sleep.bEnable"

    Patrolling --> Idle : "TraceResult 命中<br/>(cpp:316-320)"
    Patrolling --> Idle : "!Patrol.bEnable"

    Idle --> Patrolling : "索敌失败 + OnLostTarget==Patrol<br/>(cpp:1992-1998 SetTraitDeferred)"

    note right of Idle
        Idle 状态下，移动状态机
        进入 Chase (有 TraceResult)
        或 Approach (无)
    end note
```

### 2.4 攻击子状态机 `EAttackState`

```mermaid
stateDiagram-v2
    [*] --> NotAttacking : 默认 (无 FAttacking)

    state "无 FAttacking" as NotAttacking
    state "Aim" as Aim
    state "PreCast" as PreCast
    state "PostCast" as PostCast
    state "Cooling" as Cooling

    NotAttacking --> Aim : "Attack Trigger 命中<br/>SetTraitDeferred(FAttacking) (cpp:2061)"
    Aim --> NotAttacking : "目标失效<br/>RemoveTrait<FAttacking> (cpp:2117)"
    Aim --> NotAttacking : "超出射程 (cpp:2165)"
    Aim --> PreCast : "瞄准完成 (cpp:2183-2190)"
    PreCast --> PostCast : "ATKTime ≥ Attack.TimeOfHit (cpp:2353)"
    PostCast --> Cooling : "ATKTime == Attack.DurationPerRound (cpp:2453)"
    Cooling --> NotAttacking : "CoolTime == Attack.CoolDown<br/>RemoveTrait<FAttacking> (cpp:2470-2472)"
```

### 2.5 受击状态 `FBeingHit`

```mermaid
stateDiagram-v2
    [*] --> Normal
    Normal --> BeingHit : "Hit 系统命中 (cpp:5256-5270)<br/>SetFlag(HitAnimFlag), SetFlag(HitJiggleFlag)"
    BeingHit --> Normal : "Jiggle + Anim 全部播完<br/>RemoveTrait<FBeingHit> (cpp:3237-3239)"
    BeingHit --> Dying : "Health.Current ≤ 0<br/>SetTraitDeferred(FDying) (cpp:3064)"
```

---

## 3 · 四套状态系统的正交关系

同一时刻 Agent 同时持有：

```
[ Lifecycle ]    Appearing | Alive | Dying
       ×
[ Behavior  ]    Sleeping | Patrolling | (none)
       ×
[ MoveState ]    Sleep_Sleeping | Patrol_* | Chase_* | Approach_*
       ×
[ AttackState ]  (none) | Aim | PreCast | PostCast | Cooling
       ×
[ Hit/Anim  ]    Normal | BeingHit (+ flags)
```

约束（写在 Filter 的 `Exclude` 中，见 `cpp:4832-4843`）：

- `Appearing` / `Dying` 排除几乎所有行为系统（Sleep / Patrol / Trace / Attack）
- `Sleeping` / `Patrolling` 排除 Attack 相关系统
- Attack Trigger 还看 `HitAnimFlag` (`cpp:2028`)

---

## 4 · 关键源码位置索引

| 主题 | 文件 : 行 |
|---|---|
| `EMoveState` 定义 | `Public/BattleFrameEnums.h:57` |
| `EAttackState` 定义 | `Public/BattleFrameEnums.h:45` |
| MoveState 状态机主体 | `BattleFrameBattleControl.cpp:515-635` |
| Sleep 退出 | `cpp:270-280` |
| Patrol 退出 | `cpp:310-320` |
| Trace 失败回退到 Patrol | `cpp:1992-1998` |
| Attack 触发 | `cpp:2044-2072` |
| Attack 子状态转换 | `cpp:2109-2472` |
| Dying 进入 (LifeSpan) | `cpp:96-113` |
| Dying 进入 (Health ≤ 0) | `cpp:3064` |
| Appearing 移除 | `cpp:241-248` |
| BeingHit 退出 | `cpp:3236-3240` |
| Filter 定义 | `cpp:4832-4843` |
