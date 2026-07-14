# 项目规则

## 项目概述

本项目是基于 Qt 5.14.2 的植物大战僵尸游戏，集成 STM32F103 光敏传感器串口通信，实现现实环境光照驱动的昼夜模式切换。

- 硬件部分：STM32F103 通过 ADC 采集光敏传感器数据，经 USART 串口发送给 PC
- 软件部分：Qt Desktop 接收光感数据，通过迟滞阈值判断白天/黑夜，影响游戏背景与蘑菇植物睡眠状态

## 编译测试流程

每次修改代码后，必须使用项目指定的 Qt 编译工具进行编译测试，确保代码能够正常构建和运行，具体步骤如下：

### 编译命令

使用以下命令在项目根目录下执行编译：

```powershell
# 第一步：使用 qmake 生成 Makefile（64-bit debug）
D:\TEXTProgramALL\installALL\QTinstall\5.14.2\mingw73_64\bin\qmake.exe main.pro

# 第二步：使用 mingw32-make 进行编译
D:\TEXTProgramALL\installALL\QTinstall\Tools\mingw730_64\bin\mingw32-make.exe
```

### 完整清理重建（当遇到链接错误或路径变更时）

```powershell
# 清理旧编译产物（可自动重新生成，不影响源代码）
Remove-Item -Recurse -Force "out\obj" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "out\moc" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "debug" -ErrorAction SilentlyContinue

# 重新生成 Makefile 并编译
$env:PATH = "D:\TEXTProgramALL\installALL\QTinstall\Tools\mingw730_64\bin;" + $env:PATH
D:\TEXTProgramALL\installALL\QTinstall\5.14.2\mingw73_64\bin\qmake.exe main.pro
D:\TEXTProgramALL\installALL\QTinstall\Tools\mingw730_64\bin\mingw32-make.exe
```

### 编译要求

1. **完整性检查**：编译过程必须完整执行，直至生成可执行文件 `debug/main.exe`
2. **错误修复**：若出现编译错误，需逐一修复；修复时仅针对编译问题，**不得修改原有功能需求和业务逻辑**
3. **测试验证**：编译成功后，需运行生成的程序进行功能测试，确保修改未破坏现有功能
4. **持续迭代**：重复上述步骤，直至编译通过且程序运行正常

### 注意事项

- 编译工具路径固定为 `D:\TEXTProgramALL\installALL\QTinstall`，不得随意更改
- 必须使用 64-bit 编译工具（mingw73_64），避免 32-bit 编译时可能出现的内存不足问题
- 默认构建配置为 `debug`（main.pro 中 `CONFIG += qt debug`）
- 修复编译错误时，应聚焦于语法错误、依赖问题、类型不匹配等技术性问题，保持原有业务逻辑不变
- 若项目从其他路径迁移而来，旧的 `out/obj` 和 `out/moc` 中的 `.o` 文件可能残留旧路径信息，导致链接错误，需执行清理重建

## 关键模块说明

| 模块 | 文件 | 说明 |
|------|------|------|
| 串口通信 | SerialWorker.cpp/h | 独立线程运行，自动扫描 CH340/USB-TTL 设备 |
| 光感判断 | LightSensorReader.cpp/h | 迟滞阈值（ADC>1700 黑夜，<1500 白天），自动重连 |
| 场景管理 | MainView.cpp/h | 管理场景切换与昼夜模式 |
| 游戏核心 | GameScene.cpp/h | 60fps 游戏循环，触发器机制 |
| 植物系统 | Plant.cpp/h | 工厂模式 + 原型模式 |
| 僵尸系统 | Zombie.cpp/h | 继承层次结构，减速系统 |
| 图像缓存 | ImageManager.cpp/h | QPixmapCache 预加载 |
| 音频管理 | AudioManager.cpp/h | QMediaPlayer 背景音乐与音效 |
