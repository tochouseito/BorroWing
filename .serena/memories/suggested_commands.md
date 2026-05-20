# Suggested Commands

- 構成: `cmake --preset win-x64`
- Debug ビルド: `cmake --build --preset win-x64-debug`
- Release ビルド: `cmake --build --preset win-x64-release`
- PowerShell でファイル列挙: `rg --files`
- PowerShell で見出し確認: `rg -n "^(#|##|###|####)" *.md`
- エンジン側を変更した場合は CueEngine ルートで `pwsh -NoProfile -File scripts/codex_build.ps1`。BorroWing だけの変更なら上記 CMake build を優先。