# 项目规则

## 编译测试流程

每次修改代码后，必须使用项目指定的Qt编译工具进行编译测试，确保代码能够正常构建和运行，具体步骤如下：

### 编译命令

使用以下命令在项目根目录下执行编译：

```powershell
# 第一步：使用qmake生成Makefile
D:\TEXTProgramALL\installALL\QTinstall\5.14.2\mingw73_64\bin\qmake.exe main.pro

# 第二步：使用mingw32-make进行编译
D:\TEXTProgramALL\installALL\QTinstall\Tools\mingw730_64\bin\mingw32-make.exe
```

### 编译要求

1. **完整性检查**：编译过程必须完整执行，直至生成可执行文件 `main.exe`
2. **错误修复**：若出现编译错误，需逐一修复；修复时仅针对编译问题，**不得修改原有功能需求和业务逻辑**
3. **测试验证**：编译成功后，需运行生成的程序进行功能测试，确保修改未破坏现有功能
4. **持续迭代**：重复上述步骤，直至编译通过且程序运行正常

### 注意事项

- 编译工具路径固定为 `D:\TEXTProgramALL\installALL\QTinstall`，不得随意更改
- 优先使用 64-bit 编译工具（mingw73_64），避免 32-bit 编译时可能出现的内存不足问题
- 修复编译错误时，应聚焦于语法错误、依赖问题、类型不匹配等技术性问题，保持原有业务逻辑不变
