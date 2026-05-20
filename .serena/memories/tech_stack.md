# Tech Stack

- C++20。CMake プロジェクト名は `BorroWingScripts`。
- Windows / Visual Studio generator 前提。`CMakePresets.json` は `Visual Studio 18 2026`, x64。
- Cue Engine 依存。`CUE_ENGINE_ROOT` は親探索、環境変数、最後に `C:/Users/sinse/source/repos/CueEngine` の順。
- DirectXTK12 を `find_package(directxtk12 CONFIG REQUIRED)` で使用。
- 出力ターゲット: `GameSources` OBJECT、開発用 `GameScript` SHARED、リリース用 `Game` STATIC、実行用 `CueApp` WIN32。
- エンジン生成ライブラリ参照先: `${CUE_ENGINE_ROOT}/generated/outputs/Sdk/Lib/$<CONFIG>`。
- vcpkg target root: `${CUE_ENGINE_ROOT}/out/build/win-x64/vcpkg_installed/x64-windows-static-md`。