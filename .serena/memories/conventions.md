# Conventions

- 回答・コメントは日本語。外部 API 名、識別子、HLSL 識別子は英語のまま。
- CueEngine 規約に合わせる: 型 PascalCase、関数 snake_case、引数 `a_` + camelCase、変数 camelCase、class メンバ `m_` + camelCase、定数 `k_` + camelCase。
- C++ コメントは Why 中心。公開 API のみ Doxygen。
- ヘッダは `#pragma once`。ヘッダで `using namespace` 禁止。
- スクリプトは Marionette 形式を使う。例: `MARIONETTE_DECLARE_SCRIPT_TYPE`, `Marionette::Behaviour<T>`, `MARIONETTE_FIELDS`, `MARIONETTE_DEFINE_SCRIPT`。
- ゲーム仕様上の重要制約: 吸収対象は小型ミサイル・大型ミサイル・サルベージコアに限定。通常弾/レーザー/爆風/赤い特殊弾/敵本体は吸収不可。