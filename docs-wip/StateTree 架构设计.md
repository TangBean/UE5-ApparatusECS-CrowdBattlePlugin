## 心智模型
《思考，快与慢》中 Kahneman 提出人有两套认知系统（System 1 / System 2）。借用这个模型，我们将 Agent 的行为决策分为两类：
- **快反应 (Reactive)** — 对应 System 1
	- 对当前刺激的即时响应：快速、自动、低开销。依赖模式匹配和预设规则，不经过深思。
	- 例：受到攻击 → 反击；饥饿值过低 → 寻找食物。
	- With low effort。
- **慢决策 (Deliberative)** — 对应 System 2
	- 基于目标和已知信息的主动规划：较慢、需要计算资源、高开销。经过利害权衡后产出行动计划。
	- 例：根据"伐木工"身份 → 规划今天去哪片林地砍树。
	- With high effort。

映射到我们的系统里，按触发来源区分：
- **Deliberative Action** — 由 Plan 驱动。Agent 预先规划好一段时间内的行动序列（如 24h plan，时长可配置），当前正在执行的 action 来自这份计划。
- **Reactive Action** — 由事件驱动。外界突发刺激（受到攻击、环境威胁等）打断当前计划，Agent 即时切换到应激行为。

---

## 现有 Tick 中 Agent Action 的归类

以 `BattleFrameBattleControl::Tick` 为参照，先平铺列出各段 atomic action，再按 Reactive / Deliberative / 横切 三类归并。Statistics 与 Rendering 单独讨论。

### 1. Action 清单

| ID | 类别 | 子类别 | 描述 |
|-----|------|--------|------|
| A1 | 数据统计 | — | 累加 `Stats.TotalTime` |
| A2 | 数据统计 | — | 寿命到期 → `SetTraitDeferred(FDying)` + 入队 `Death(OutOfLifeSpan)` |
| A3 | 出生 | — | 首次出生：派生 Actor / Fx / Sound 的 spawn config（延后由 Game Thread Logic 真正生成） |
| A4 | 出生 | — | 入队 `Appear` 事件 |
| A5 | 出生 | — | 触发出生动画 flag、淡入 flag |
| A6 | 出生 | — | 推进出生动画时间 / 淡入曲线 |
| A7 | 出生 | — | 出生流程结束 → `RemoveTraitDeferred<FAppearing>` |
| A8 | 移动 | 休眠 | 若 `Tracing` 命中目标 → 退出 `FSleeping` |
| A9 | 移动 | 巡逻 | 到点 / 超时 → `FindNewPatrolGoalLocation` + 重置巡逻参数 |
| A10 | 移动 | 推动 (SphereObstacle) | 把范围内的 agents 标 `bPushedBack = true`，离开范围则清掉 |
| A11 | 移动 | 主体 | KillZ 检查 → `DespawnDeferred` + 入队 `Death(KillZ)` |
| A12 | 移动 | 主体 | MoveState 状态机切换（Sleep / Patrol_x / Chase_x / Approach_x）并入队 `Move` 事件 |
| A13 | 移动 | 主体 | 路径规划（A*）或直接朝向，或读 FlowField |
| A14 | 移动 | 主体 | 计算 DesiredSpeedMultiplier（巡逻/追击/减速/转向夹角/距离插值） |
| A15 | 移动 | 主体 | 处理 `LaunchVelSum`（外力冲击）→ `bLaunching` |
| A16 | 移动 | 主体 | RVO 邻居避障（agent + obstacle） |
| A17 | 移动 | 主体 | Z 轴：地面探测（FlowField / SphereTrace）→ 重力 / 飞行高度维持 / 着陆 |
| A18 | 移动 | 主体 | 写入 `Located.Location` |
| A19 | 移动 | 主体 | 朝向插值（ToPath / ToMovement / ToMovementForwardAndBackward / 攻击瞄准朝向目标） |
| A20 | 移动 | 更新邻居网格 | 调用所有 `NeighborGrid->Update()` |
| A21 | 攻击 | 索敌 | 选择该用哪套索敌参数（Sleep/Patrol/Chase/Common） |
| A22 | 攻击 | 索敌 | Player_0 距离/角度/可见性检测 或 SectorTrace 扇形索敌 |
| A23 | 攻击 | 索敌 | 入队 `Trace` Begin / Succeed / Fail |
| A24 | 攻击 | 索敌 | 丢失目标且 `OnLostTarget=Patrol` → 进入 `FPatrolling` |
| A25 | 攻击 | 攻击触发 | 进入攻击范围 → `SetTraitDeferred(FAttacking)` + 入队 `Attack(Aiming)` |
| A26 | 攻击 | 攻击过程 | Aim：校验目标/距离/角度，超时/丢失则中止 |
| A27 | 攻击 | 攻击过程 | PreCast_FirstExec：播放攻击动画 flag；派生 Projectile / Actor / Fx / Sound spawn config；入队 `Attack(Begin)` |
| A28 | 攻击 | 攻击过程 | PreCast→PostCast：到判定帧时，按 `Point` / `Radial` 结算伤害，入队 `Attack(Hit)`；`SuicideATK` / `Despawn` 直接 `DespawnDeferred` |
| A29 | 攻击 | 攻击过程 | PostCast→Cooling：清动画 flag，入队 `Attack(Cooling)` |
| A30 | 攻击 | 攻击过程 | Cooling 完成：`RemoveTraitDeferred<FAttacking>` + 入队 `Attack(Complete)` |
| A31 | 攻击 | 攻击过程 | 推进 Aim / ATK / Cool 计时器 |
| A32 | 投射物 | — | 生成投射物 subject（Static / Interped / Ballistic / Tracking） |
| A33 | 投射物 | — | 投射物运动一帧 |
| A34 | 投射物 | — | 命中检测 + Point / Radial / Beam 伤害结算 + 加入 ignore 列表 |
| A35 | 投射物 | — | 命中爆炸 Fx subject 生成 |
| A36 | 投射物 | — | 到期/命中 → DespawnDeferred |
| A37 | 受击 | 受击反馈 | 出队 `DamageToTake`，扣血；致命伤 → `SetTraitDeferred(FDying)`，Stats 累加 |
| A38 | 受击 | 受击反馈 | 血条 Ratio 插值 / Opacity 控制 |
| A39 | 受击 | 受击反馈 | 受击发光（HitGlow 曲线） |
| A40 | 受击 | 受击反馈 | 受击形变（JiggleMultiplier 曲线） |
| A41 | 受击 | 受击反馈 | 受击动画 flag 计时 |
| A42 | 受击 | 受击反馈 | 全部完成 → `RemoveTraitDeferred<FBeingHit>` |
| A43 | 受击 | 减速马甲 | 首次注册到目标 `Slowing.Slows`，开启材质 FX (Fire/Ice/Poison) |
| A44 | 受击 | 减速马甲 | 到期 → 注销、若无同类型残留则关闭材质 FX，自毁 |
| A45 | 受击 | 延时伤害马甲 | 首次注册到 `TemporalDamaging.TemporalDamages` + 开材质 FX |
| A46 | 受击 | 延时伤害马甲 | 计时到段 → 给目标入队伤害 + 文字气泡 |
| A47 | 受击 | 延时伤害马甲 | 段数耗尽 → 注销 + 关材质 FX + 自毁 |
| A48 | 受击 | 死亡 | 首次：派生死亡 Actor / Fx / Sound spawn config，触发死亡动画 flag 与消融 flag |
| A49 | 受击 | 死亡 | 消融曲线驱动 `Animating.Dissolve` |
| A50 | 受击 | 死亡 | `DespawnDelay` 到 → `DespawnDeferred` |
| A51 | 游戏线程逻辑 | — | Spawn Actor（真正在 GT 调用 `SpawnActor`，维护附着、生命周期） |
| A52 | 游戏线程逻辑 | — | Spawn Fx（Niagara/Cascade，附着，生命周期） |
| A53 | 游戏线程逻辑 | — | Play Sound（2D/3D，异步加载，附着） |
| A54 | 游戏线程逻辑 | — | Event Interface：把 OnAppear/Trace/Move/Attack/Hit/Death 事件分发到 BP 接口 |
| A55 | 游戏线程逻辑 | — | Draw Debug Shapes：消费各种 debug 队列 |
| A56 | 渲染 | — | 动画状态机（决定每个 Subject 当前播放的动画） |
| A57 | 渲染 | — | ClearValidTransforms（池槽初始化） |
| A58 | 渲染 | — | Gather Render Data（收集 Transform/VAT/HealthBar/PopText 等） |
| A59 | 渲染 | — | Write Pooling Info（写回池） |
| A60 | 渲染 | — | Send Data to Niagara（把数组打包发到 Niagara 渲染） |

### 二、按 Reactive / Deliberative 归类

> 关键判定：**触发源是「agent 内部目标/角色驱动」还是「外界刺激事件」**。

#### Deliberative Action（由目标/计划驱动；System 2）
当前代码里没有真正的 24h Plan，但这些 action 等价于"agent 当前的主动目标推进"：

- **巡逻执行序列**：A9（选下一个巡逻点）、A14 中 Patrol 段速度系数、A19 中 ToMovement 朝向
- **追击 / 接近目标**：A12 状态机里 Chase_x / Approach_x、A13 寻路、A14、A18
- **主动索敌**：A21–A23（按 MoveState 选参数，主动 SectorTrace 找目标）
- **完整攻击序列**：A25 → A26 Aim → A27 PreCast → A28 Hit 判定 → A29 Cooling → A30 完成、A31 计时（一旦决定攻击就按 plan 演完整段）
- **派生子任务的"意图"**：A27/A48 中由攻击/死亡发起的 Spawn Projectile / Actor / Fx / Sound 配置（A32, A51–A53 是 GT 落地，属横切）
- **丢失目标后回巡逻**：A24（基于 `OnLostTarget=Patrol` 这一角色配置的策略）

#### Reactive Action（外界刺激即时打断当前计划；System 1）
触发条件是事件 / 外力 / 环境读数：

- **生命周期类反应**
  - A2 寿命到 → Dying
  - A11 KillZ 跌出地图 → Despawn
  - A4–A7 出生（被 spawn 这一外部事件触发的一次性初始化 + 短时表现）
  - A50 死亡 Despawn 收尾
- **被外力 / 环境推动**
  - A8 被 Trace 命中而醒来（休眠中被刺激）
  - A10 被 SphereObstacle 推动
  - A15 LaunchVelSum 外力冲击
  - A17 重力 / 着陆 / 飞行高度维持（环境约束的被动反应）
  - A16 RVO 避障（对周围 agent/障碍的即时响应）
- **受击反应链**
  - A37 出队伤害扣血
  - A38–A41 血条 / 发光 / 形变 / 受击动画
  - A42 状态收尾
- **持续 debuff 的"被动表现 + 自驱"**
  - A43–A44 减速马甲生命周期 + 材质 FX
  - A45–A47 延时伤害马甲（每段伤害都是对宿主的反应注入）
- **死亡反应**
  - A48 派生死亡 spawn config（被"致命伤"这一事件触发）
  - A49 消融
- **投射物自身的反应链**（投射物作为独立 agent）
  - A33 运动、A34 命中伤害、A35 命中爆炸、A36 到期自毁

#### 横切 / 非"决策 Action"（基础设施层）
这些不是 agent 的决策，而是"执行 / 反馈 / 渲染"管线，不在 StateTree 的 Reactive/Deliberative 二分内：

- A20 NeighborGrid 更新（空间索引）
- A51–A53 Game Thread 真正生成 Actor/Fx/Sound（A27 等 Deliberative 决策的执行落地）
- A54 Event Interface 事件外发（让 BP 层旁观）
- A55 Debug Shapes 消费

### 三、Statistics 与 Rendering 单独讨论

#### Statistics
- A1 `TotalTime` 累加：是被动计时，谈不上"决策"，属横切的**计量基础设施**。
- A2 寿命到期 → Dying：虽然写在 Statistics 段，但语义是**Reactive**（被"寿命计时器超时"这一事件触发，进入死亡反应链）。
- A37 里对 `Stats.TotalDamage/Kills/Score` 的写入也是计量副作用，不算 action。

→ Statistics 段在 StateTree 视角下应被拆开：**计时累加是横切**，**寿命到 → Dying 是 Reactive**。它放在第一段只是因为它读写 `Stats`，并不构成一个"统计大类决策"。

#### Rendering（A56–A60）
全是**横切的表现层管线**，完全不参与 agent 的行为决策：
- 它只**读** agent 当前状态（Animating/Located/Health 等）。
- 输出到 Niagara 池。
- 不写回任何会改变下一帧决策的 trait。

→ 在 StateTree 设计中，Rendering 应该完全划到 **"非 Action / View 层"**，与 Deliberative/Reactive 正交。它只是"把 agent 当前状态可视化"，是 ECS 中典型的 read-only system。

### 四、迁移到 StateTree 时的归宿建议

| 段 | StateTree 中的归宿 |
|---|---|
| Patrol / Chase / Approach 状态切换 + 索敌 + 完整攻击序列 | **Deliberative 子树**（一个"行为计划"，可被打断） |
| Appear / 受击链 / Slow / TemporalDamage / Death / KillZ / Launch / PushedBack | **Reactive 子树**（事件驱动，可中断当前 Deliberative 分支） |
| RVO 避障、重力/着陆 | **Reactive 但偏物理子系统**，建议作为独立 movement 求解器，不入 StateTree |
| NeighborGrid 更新、Spawn Actor/Fx/Sound 执行、Event Interface、Debug | **Infrastructure systems**（StateTree 外） |
| Statistics 计时 / Rendering 全流程 | **View / Observer systems**（StateTree 外） |

---
继续 state tree design：
claude --resume 243c770f-72d7-43c0-8ab6-3eade7b83157
生成投射物部分代码理解：
claude --resume 90ecf9cd-c046-401d-a841-0a0b942b622b