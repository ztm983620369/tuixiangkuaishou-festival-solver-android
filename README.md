# 推箱快手 Festival 求解融合版

这是一个将 BoxMan 9.89 Android 源码与 Festival3OS 推箱子求解器融合的工程。

## 当前状态

- `app`：推箱快手 Android 游戏源码。
- `festival-solver`：独立 Android library 模块，封装 Festival native 求解器。
- 游戏界面底部新增“提示”按钮。
- 点击“提示”会导出当前正推局面，调用 Festival 求解器，解析 LURD 路径，并让游戏自动执行第一步。
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
