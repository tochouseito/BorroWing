# Core

- Cue Engine 用ゲームプロジェクト: `BorroWing` / ウィンドウタイトル `BorroWing`。
- 入口設定: `cueproject.json`。`assetRoot=Assets`, `scriptRoot=.`, `startupScene=Assets/Scenes/Main.cuescene`。
- 初期シーン: `Assets/Scenes/Main.cuescene`。現状は `MainCamera` と `Cube` のみ。
- スクリプト登録の公開口: `EngineModule/ScriptRegistry.h`。生成ソースは `Intermediate/Generated/ScriptRegistry.gen.cpp`。
- ゲーム仕様: `BORROWED_WINGS_仕様書.md`。企画意図: `BORROWED_WINGS_企画書.md`。
- 実装時はスクリプト側を `Assets/Scripts/*Script.h/.cpp` に追加する。CMake が `*Script.cpp` / `*Script.h` を glob する。
- 関連メモ: 技術構成は `mem:tech_stack`、コマンドは `mem:suggested_commands`、規約は `mem:conventions`、完了条件は `mem:task_completion`。