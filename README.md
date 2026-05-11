# President

C++の大富豪サンプルを使ったAI実装プロジェクトです。提出用AIは
`sample/sampleProgramForMac/simpleplayer.cpp` の `SimplePlayer::follow` に実装しています。

## 使い方

```sh
cd sample/sampleProgramForMac
make
./daifugou
```

自動対戦で確認する場合は次のように実行します。

```sh
./daifugou -a -n 100
```

`-a` は自動進行、`-n` は試合数です。詳しいオプションは `./daifugou -h` で確認できます。

## sample

`sample/sampleProgramForMac/` がビルド・実行対象のサンプルです。主なファイルは以下です。

- `simpleplayer.cpp`: 改善したAI本体
- `simpleplayer.h`: `SimplePlayer` の宣言とカードソート比較
- `daifugou.cpp`: 対戦プログラム本体
- `dealer.cpp`: ルール判定とゲーム進行

## source

`source/` には授業資料・参考資料のPDFがあります。ルールやサンプルコードの意図を確認するときに参照してください。

## AI改善メモ

今回の改善内容は `ai-improvement/README.md` にまとめています。
