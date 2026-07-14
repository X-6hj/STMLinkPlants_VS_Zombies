# 更新日志

## 2026-07-03 入水水花动画修复

### Bug 修复

#### 入水水花显示为多个水花团（splash.png 精灵图整图显示问题）

**文件**：`src/Zombie.cpp`（`ZombieInstance::triggerWaterEntry`）

**问题**：`splash.png` 是一张 776×88 像素的精灵图（sprite sheet），横向排列了 8 帧水花动画（每帧 97×88）。原代码使用 `QGraphicsPixmapItem` 直接加载整张图，导致 8 帧同时并排显示，玩家看到一个僵尸入水时出现"3 个水花团"的错误效果。同时 776 像素宽的整图覆盖了僵尸，导致玩家看不到入水后 GIF 从陆地行走切换到水中游泳姿态的变化。

**修复**：将整图显示改为逐帧动画播放：
1. 加载完整 `splash.png` 后，用 `QPixmap::copy()` 按帧切割（每帧 97×88）
2. 使用 `QTimer` 每 80ms 切换一帧，共播放 8 帧（总时长 640ms）
3. 动画结束后自动从场景移除并清理 `QGraphicsPixmapItem` 和 `QTimer`
4. 支持 `gPaused` 全局暂停：暂停时跳过帧切换，恢复后继续

**效果**：
- 入水时只显示一个水花（逐帧动画），不再出现多个水花团
- 水花尺寸从 776×88 缩小为 97×88，不再遮挡僵尸
- 玩家可清楚看到僵尸入水后从 `Zombie.gif`（陆地行走）切换为 `Walk1.gif`（水中游泳姿态，含鸭子游泳圈）

**正版依据**：正版植物大战僵尸中，僵尸入水时显示单帧水花动画，持续时间约 0.5-0.8 秒。

---

## 2026-07-03 功能整合与 Bug 修复

### 概述

将 `D:\TextSchool\unionSTMAndQT\STMLinkPlants_VS_Zombies\Qt-PlantsVsZombies-master`（STM32+Qt 集成版）的功能整合到当前项目，修复第四关水域僵尸死亡动画 Bug，补全缺失的僵尸资源注册，并完成 64 位 debug 编译验证。

---

### Bug 修复

#### 1. 第四关下水僵尸死亡动画错误

**文件**：`src/Zombie.cpp`

**问题**：第四关（泳池关卡）的两种水域僵尸使用了陆地普通僵尸的死亡动画，死亡时显示的是旱地僵尸倒地动画，与水域环境不符。

- `DuckyTubeZombie2`（路障鸭子僵尸）：`dieGif` 错误指向 `Zombies/Zombie/ZombieDie.gif`
- `DuckyTubeZombie3`（铁桶鸭子僵尸）：`dieGif` 错误指向 `Zombies/Zombie/ZombieDie.gif`

**修复**：将两者的 `dieGif` 改为水域风格的死亡动画 `Zombies/DuckyTubeZombie1/Die.gif`（复用已有的鸭子僵尸死亡动画资源）。

| 僵尸 | 修复前 | 修复后 |
|------|--------|--------|
| DuckyTubeZombie2（路障鸭子） | `Zombies/Zombie/ZombieDie.gif` | `Zombies/DuckyTubeZombie1/Die.gif` |
| DuckyTubeZombie3（铁桶鸭子） | `Zombies/Zombie/ZombieDie.gif` | `Zombies/DuckyTubeZombie1/Die.gif` |

**依据**：正版植物大战僵尸中，水域僵尸死亡时应播放水中下沉/倒下的动画，而非陆地僵尸的死亡动画。`DuckyTubeZombie1/Die.gif` 资源已存在且已在 `main.qrc` 中注册。

---

### 资源整合

#### 2. 补全缺失的僵尸资源注册（main.qrc）

**文件**：`main.qrc`

**问题**：对比源项目发现，当前项目 `main.qrc` 缺少 8 个僵尸相关资源的注册。这些资源文件在磁盘上已存在，但未注册到 Qt 资源系统，导致运行时无法加载（对应动画/卡片不显示）。

**新增注册的 8 个资源**：

| 资源路径 | 用途 | 影响的僵尸 |
|----------|------|------------|
| `images/Zombies/FootballZombie/OrnLost.gif` | 饰品损坏后行走动画 | 橄榄球僵尸 |
| `images/Zombies/FootballZombie/OrnLostAttack.gif` | 饰品损坏后攻击动画 | 橄榄球僵尸 |
| `images/Zombies/FootballZombie/LostHead.gif` | 掉头行走动画 | 橄榄球僵尸 |
| `images/Zombies/FootballZombie/LostHeadAttack.gif` | 掉头攻击动画 | 橄榄球僵尸 |
| `images/Zombies/FootballZombie/Die.gif` | 死亡动画 | 橄榄球僵尸 |
| `images/Zombies/DancingZombie/DancingZombie1.gif` | 舞王僵尸舞蹈变体 | 舞王僵尸 |
| `images/Card/Zombies/NewspaperZombie.png` | 读报僵尸卡片图 | 读报僵尸 |
| `images/Card/Zombies/PoleVaultingZombie.png` | 撑杆僵尸卡片图 | 撑杆僵尸 |

**影响**：修复后，橄榄球僵尸可正确显示饰品损坏、掉头、死亡动画；读报僵尸和撑杆僵尸的卡片图可正常加载。

---

### 功能改进

#### 3. 水域僵尸入水动画（DuckyTube1/2/3）

**文件**：`src/Zombie.h`、`src/Zombie.cpp`、`main.qrc`

**问题**：第四关水域僵尸（鸭子僵尸、路障鸭子僵尸、铁桶鸭子僵尸）从屏幕右侧出现时直接以水中漂浮姿态显示，缺少"接触水池 → 跳入水中"的过渡反馈，不符合正版植物大战僵尸的体验。

**修复方案**：新增入水触发机制，水域僵尸出生时使用陆地行走姿态（`Zombies/Zombie/Zombie.gif`），向左走到屏幕右侧水边（X≤820）时触发入水动作。

**入水动作包含**：
1. 播放 `audio/zombie_entering_water.mp3` 入水音效
2. 在僵尸脚下显示 `images/interface/splash.png` 水花图片（持续 800ms 后自动清除）
3. 切换为水中漂浮姿态（`DuckyTubeZombie1/Walk1.gif`）
4. 若 OrnZombie 饰品已掉落，自动切换到 `ornLostNormalGif`（水中受损姿态）

**新增资源注册**：

| 资源路径 | 用途 | 原状态 |
|----------|------|--------|
| `images/interface/splash.png` | 入水水花图片 | 磁盘存在但未注册到 qrc |
| `audio/zombie_entering_water.mp3` | 入水音效 | 磁盘存在但未注册到 qrc |

**代码变更**：

| 文件 | 变更内容 |
|------|---------|
| `Zombie.h` | ZombieInstance 基类新增 `enteredWater`、`landNormalGif`、`landAttackGif` 字段和 `triggerWaterEntry()` 方法；新增 `DuckyTubeZombie1Instance` 类；为 `DuckyTubeZombie2Instance`、`DuckyTubeZombie3Instance` 添加 `birth()` 和 `checkActs()` override |
| `Zombie.cpp` | 实现 `triggerWaterEntry()`（音效+水花+GIF 切换，含 OrnZombie 饰品掉落兼容）；实现 DuckyTube1/2/3 Instance 的 `birth()`（启用入水逻辑、初始陆地 GIF）和 `checkActs()`（X≤820 触发入水后调用基类）；在 `ZombieInstanceFactory` 中为 `oDuckyTubeZombie1` 创建 `DuckyTubeZombie1Instance` |
| `main.qrc` | 新增注册 `splash.png` 和 `zombie_entering_water.mp3` |

**设计要点**：
- 入水触发阈值 X≤820（屏幕右侧，僵尸出生后向左走约 3.5 秒后触发，玩家可短暂看到陆地行走姿态）
- `triggerWaterEntry()` 使用 `dynamic_cast<OrnZombieInstance1*>` 检测饰品状态，避免在基类引入 OrnZombie 依赖
- 水花图片 z-value 设为僵尸 z-value+2，确保显示在僵尸之上
- 入水音效复用共享音频播放器池 `getSharedAudioPlayer()`，避免为每个僵尸创建独立 QMediaPlayer

**性能影响**：
- 每个水域僵尸仅在入水瞬间触发一次水花+音效，无持续性能开销
- 水花图片 800ms 后自动清除（`delete`），不累积内存
- 共享音频播放器池避免音频对象爆炸
- `dynamic_cast` 仅在入水瞬间调用一次，无运行时开销

**正版依据**：正版植物大战僵尸中，水域僵尸从屏幕右侧陆地出现，走到水池边缘时跳入水中，伴随水花和音效，然后以水中漂浮姿态前进。本次修复复用了项目已有的 `splash.png` 和 `zombie_entering_water.mp3` 资源（原项目存在但未启用）。

---

### 僵尸行动逻辑优化（对标正版）

通过研究项目代码并对比正版植物大战僵尸的僵尸行为参数，对以下 6 类僵尸的行动逻辑进行优化，使玩家看到的动画与行为更贴近正版。

#### 4. 撑杆僵尸跳跃距离优化

**文件**：`src/Zombie.cpp`（PoleVaultingZombieInstance::checkActs）

**问题**：原跳跃距离 80 像素（1 格），正版约 1.5 格。

**修复**：跳跃距离 80 → 130 像素（约 1.5 格）。

**玩家可见效果**：撑杆僵尸跳过植物后落点更远，更接近正版"跳过 1.5 格"的体感。

---

#### 5. 海豚骑士僵尸跳跃距离优化

**文件**：`src/Zombie.cpp`（DolphinRiderZombieInstance::checkActs）

**问题**：原跳跃距离 80 像素（1 格），正版约 1.5 格。

**修复**：跳跃距离 80 → 130 像素（约 1.5 格）。

**玩家可见效果**：海豚骑士跃过植物后落点更远。

---

#### 6. 读报僵尸愤怒速度倍率优化

**文件**：`src/Zombie.cpp`（NewspaperZombieInstance::getHit）

**问题**：原愤怒倍率 2.0（速度 0.24 → 0.48），正版约 2.6 倍（速度 0.24 → 0.624，对应正版 4.7s/格 → 1.8s/格）。

**修复**：愤怒倍率 2.0 → 2.6。

**玩家可见效果**：报纸被打掉后，读报僵尸咆哮加速冲锋，速度明显比原来更快，威胁感更强。

---

#### 7. 橄榄球僵尸头盔掉落冲锋

**文件**：`src/Zombie.h`、`src/Zombie.cpp`（FootballZombieInstance::getHit 新增）

**问题**：原版橄榄球僵尸头盔掉落后会有"爆发冲刺"行为，本项目无任何加速。

**修复**：新增 `FootballZombieInstance::getHit` override，检测头盔掉落瞬间触发 `baseSpeed *= 1.4` 冲锋加速。新增 `helmetLost` 标志防止重复触发。

**玩家可见效果**：打掉橄榄球僵尸的头盔后，它会突然加速冲刺（速度 0.4 → 0.56），玩家需提前布置火力。

---

#### 8. 潜水僵尸免疫逻辑修复

**文件**：`src/Zombie.h`、`src/Zombie.cpp`（SnorkelZombieInstance）

**问题**：原 `getHit` override 在潜水状态下 `return` 免疫**所有**直接攻击（包括爆炸、秒杀、地刺），过强。正版只免疫豌豆直射，爆炸/秒杀应有效。

**修复**：移除 `SnorkelZombieInstance::getHit` override，仅保留 `getPea` override（豌豆直射免疫）。爆炸（樱桃、土豆地雷）、秒杀（窝瓜、小鬼机器人）等走 `getHit`/`boomDie`/`crushDie` 通道，现在能正常伤害潜水僵尸。

**玩家可见效果**：潜水僵尸在水下时仍免疫豌豆射手、寒冰射手、机枪射手等直射类植物，但樱桃炸弹、土豆地雷、窝瓜等爆炸/秒杀植物能直接击杀水下的潜水僵尸，符合正版策略。

---

#### 9. 小丑僵尸自爆倒计时上限优化

**文件**：`src/Zombie.cpp`（JackinTheBoxZombieInstance 构造函数）

**问题**：原倒计时 60~400 帧（1~6.7 秒），正版约 1~5 秒。

**修复**：倒计时上限 400 → 300 帧（`60 + qrand()%241`，即 1~5 秒）。

**玩家可见效果**：小丑僵尸出现后最多 5 秒就会自爆（原来最长 6.7 秒），玩家需要更快速反应。

---

### 优化汇总表

| 僵尸 | 优化项 | 修改前 | 修改后 | 正版依据 |
|------|--------|--------|--------|----------|
| 撑杆僵尸 | 跳跃距离 | 80px (1格) | 130px (1.5格) | 正版约 1.5 格 |
| 海豚骑士 | 跳跃距离 | 80px (1格) | 130px (1.5格) | 正版约 1.5 格 |
| 读报僵尸 | 愤怒倍率 | ×2.0 | ×2.6 | 正版 4.7s/格→1.8s/格 |
| 橄榄球僵尸 | 头盔掉落加速 | 无 | ×1.4 冲锋 | 正版头盔掉落爆发冲刺 |
| 潜水僵尸 | 免疫逻辑 | 免疫所有直接攻击 | 仅免疫豌豆直射 | 正版爆炸/秒杀可伤水下 |
| 小丑僵尸 | 自爆倒计时上限 | 6.7 秒 | 5 秒 | 正版 1~5 秒 |

---

### 跳过的优化项

| 僵尸 | 计划优化 | 跳过原因 |
|------|---------|----------|
| 铁网门僵尸 | 铁网门挡投掷物 | 项目无投掷类植物（Pult 系列），优化无实际意义 |
| 冰车僵尸 | 冰道功能性（植物无法种植/僵尸加速） | 涉及 Plant 系统改动，工作量大且影响范围广，暂不处理 |
| 旗帜僵尸 | 增加特殊行为 | 正版旗帜僵尸本身仅作为大波标志，无其他特殊行为 |
| 小鬼僵尸 | 由巨人僵尸投掷 | 项目无巨人僵尸，属于设计差异而非 bug |

---

### 保留的当前项目改进（未从源项目覆盖）

经逐文件对比（`git diff --no-index`），以下当前项目的改进优于源项目，予以保留：

| 文件 | 当前项目改进 | 源项目状态 |
|------|-------------|-----------|
| `Coordinate.cpp` | 泳池关卡行坐标优化（更均匀的 Y 坐标分布），注释更清晰 | 旧坐标值 |
| `MainView.cpp` | 场景切换时清理旧场景（`deleteLater`），重置 `gPaused` 标志，防止内存泄漏与定时器异常 | 缺少场景清理逻辑 |
| `SerialWorker.cpp/h` | 简化串口停止逻辑，移除冗余 `stopping` 标志 | 含 `stopping` 标志 |
| `LightSensorReader.cpp` | 重连计数由 `tryReconnect` 统一管理 | 计数逻辑分散 |
| `main.cpp` | `qRegisterMetaType` 调整到 `QApplication` 创建后 | 创建前注册 |
| `main.qrc`（植物部分） | 额外注册了咖啡豆、毁灭菇、火爆辣椒、大嘴花、魅惑菇、窝瓜、阳光菇、高坚果、三线射手、裂荚射手、火炬树桩、双子向日葵、荷叶、海蘑菇、缠咬海草、地刺、地刺王、南瓜头等植物资源及 background4/5/6 背景 | 缺少这些植物资源 |

---

### 性能整合说明

性能相关模块（`ImageManager`、`AudioManager`、`Timer`、`Animate`）在两个项目中**完全一致**，无需整合：

- **ImageManager**：使用 `QPixmapCache` 预加载和缓存图片资源，避免重复加载
- **AudioManager**：使用 `QMediaPlayer` 管理背景音乐和音效
- **Timer**：基于 `QTimeLine` 实现属性动画
- **Animate**：基于 `QTimer` 实现定时回调

本次新增的 8 个 `.qrc` 资源注册会略微增加二进制体积（约几百 KB），但对运行时性能无影响——Qt 资源在编译时嵌入可执行文件，运行时直接内存映射访问。

---

### 项目规则

#### 3. 新增 `project_rules.md`

在项目根目录创建 `project_rules.md`，包含：
- 项目概述（STM32+Qt 光感昼夜模式）
- 编译流程（qmake + mingw32-make，64 位 debug）
- 清理重建步骤（路径迁移后需清理旧 `.o` 文件）
- 关键模块说明表

---

### 编译验证

- **工具链**：Qt 5.14.2 (mingw73_64) + MinGW 7.3.0 64-bit
- **配置**：debug
- **结果**：编译成功，生成 `debug/main.exe`（MAKE_EXIT=0）
- **警告**：`Coordinate.cpp` 中 `qBinaryFind` 弃用警告（Qt 已标记为 deprecated，不影响功能，属既有代码）
- **编译过程**：因旧编译产物残留其他路径（E 盘）信息导致链接错误，执行清理重建后全量编译通过；入水动画修复后再次编译通过

---

### 变更文件清单

| 文件 | 变更类型 | 说明 |
|------|---------|------|
| `src/Zombie.cpp` | 修改 | 死亡动画路径修复（DuckyTube2/3）；新增 `triggerWaterEntry()` 辅助方法；新增 DuckyTube1/2/3 Instance 的 `birth()` 和 `checkActs()`；`ZombieInstanceFactory` 注册 DuckyTube1Instance；撑杆/海豚骑士跳跃距离 80→130；读报愤怒倍率 2.0→2.6；新增 FootballZombieInstance::getHit 头盔掉落冲锋；移除 SnorkelZombieInstance::getHit override；小丑倒计时上限 400→300 |
| `src/Zombie.h` | 修改 | ZombieInstance 基类新增 `enteredWater`、`landNormalGif`、`landAttackGif` 字段和 `triggerWaterEntry()` 方法；新增 `DuckyTubeZombie1Instance` 类；DuckyTube2/3 Instance 添加 `birth()` 和 `checkActs()` override；FootballZombieInstance 添加 `getHit` override 和 `helmetLost` 字段；移除 SnorkelZombieInstance::getHit override 声明 |
| `main.qrc` | 修改 | 新增 8 个僵尸资源注册 + `splash.png` + `zombie_entering_water.mp3` |
| `project_rules.md` | 新增 | 项目编译规则与模块说明 |
| `CHANGELOG.md` | 新增 | 本更新日志 |
