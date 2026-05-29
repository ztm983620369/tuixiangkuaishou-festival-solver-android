# 推箱快手 Festival 求解融合版

这是一个将 BoxMan 9.89 Android 源码与 Festival3OS 推箱子求解器融合的工程。

## 当前状态

- `app`：推箱快手 Android 游戏源码。
- `festival-solver`：独立 Android library 模块，封装 Festival native 求解器。
- 游戏界面底部新增“提示”按钮。
- 点击“提示”会导出当前正推局面；没有缓存时弹出 Festival 求解面板，可调时间、算法、线程、额外内存等参数。
- 求解面板会显示当前状态、耗时、输入关卡和 Festival 原生 console/solution 输出，失败也会保留原始信息用于排查。
- 时间限制支持 15 秒到 3600 秒；复杂关卡短时间没有 LURD 路径通常表示未在限定时间内解出，不是解析失败。
- 线程和额外内存会按手机运行时 CPU/内存能力动态开放；Festival 原生只支持 1/2/4/8 线程，APP 会自动选择不超过本机能力的最大支持值。
- 求解成功后缓存完整校验路径，后续每次“提示”只按缓存执行下一步；局面变化会主动丢弃缓存并要求重新求解。
- 求解器输入会把 BoxMan 的墙外 `_` 转为墙，避免 Festival 将墙外区域误判为可走地板。
- Festival 返回的路径不会被直接信任；游戏会在后台用当前棋盘完整模拟整条路径，确认每一步合法且最终通关后才执行第一步。
- 只有 Festival 原生输出明确包含 `SOLVED!` 的路径才会被当作提示候选；开启“保存 best”时输出的 best/中间路径只用于诊断，不会进入缓存或自动执行。
- 求解候选会按多个 Festival 算法策略尝试，只有通过本地校验的候选路径才会进入游戏动作系统。
- 求解动作复用原游戏的 `formatPath + UpData1` 移动系统，保留动画、计步、undo/redo 等原逻辑。

## 构建

本工程使用 Android Gradle Plugin 8.2.2、Gradle wrapper、CMake 和 NDK。

```bash
./gradlew --no-daemon --max-workers=1 :app:assembleDebug
```

默认只构建 `arm64-v8a`，生成 APK：

```text
app/build/outputs/apk/debug/app-debug.apk
```

## 目录说明

- `app/src/main/java/my/boxman/myGameView.java`：提示按钮接入与游戏动作执行。
- `festival-solver/src/main/java/my/boxman/solver/FestivalSolver.java`：Java 求解器门面。
- `festival-solver/src/main/cpp/`：Festival native 源码和 JNI 桥接。

## 注意

首次启动数据目录使用 app 专属外部目录，避免现代 Android 存储权限导致启动失败。
