# Task Completion

- BorroWing のスクリプト/シーン変更後は BorroWing ルートで `cmake --build --preset win-x64-debug` を通す。
- 構成未生成なら先に `cmake --preset win-x64`。
- CMake やリリース実行構成を触った場合は `cmake --build --preset win-x64-release` も確認。
- CueEngine 側に変更を入れた場合は CueEngine ルートで `pwsh -NoProfile -File scripts/codex_build.ps1` を通す。
- 初回オンボーディングメモの参照整合性は BorroWing ルートで `serena memories check` を実行して確認できる。