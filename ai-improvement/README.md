# Daifugou AI Improvement Notes

## 変更対象

- `sample/sampleProgramForMac/simpleplayer.cpp`
- `sample/sampleProgramForMac/simpleplayer.h`
- `README.md`

## 実装方針

AIは次の順で判断します。

1. 現在の場に対して出せる合法手を列挙する
2. 各合法手とパスを評価関数でスコアリングする
3. 最もスコアが高い手を `CardSet` に入れて出す

このサンプルの `Dealer` は同ランク複数枚とJoker補完だけを合法として扱います。階段は `Dealer::checkRankUniqueness` で拒否されるため、AI側でも生成していません。

## 評価関数

主な評価項目は以下です。

- 上がれる手に非常に大きな加点
- 場が空なら、弱い複数枚・弱い単体の処理を加点
- 場に返すときは、勝てる中で弱い手を優先
- Joker、2、A、Kの使用に減点。ただし終盤・相手危険時は減点を軽くする
- ペアや3枚組を崩して弱い単体を残す手に減点
- 出した後の手札がペア・複数枚・強い単体・次に上がれる形なら加点
- 手札4枚以下は攻撃的に評価し、8枚以下でも上がり筋が近い場合は少し攻撃寄りにする
- 相手が1〜2枚なら、空場で弱い単体を出さず、複数枚や強い手を評価する
- PDFの条件に合わせ、名前依存の協調や支援役化はしない
- 参加プレイヤ名ではなく、場札・自分の手札・全相手の残り枚数だけで判断する
- 出した後の手札を軽量に分解し、あと何回の自由リードで出し切れるかを終盤評価に使う

重みは `simpleplayer.cpp` 冒頭の `k...` 定数で調整できます。

## 確認コマンド

```sh
cd sample/sampleProgramForMac
make
./daifugou -a -n 100
```

## 確認結果

- `make`: 成功
- `./daifugou -a -n 10000`: 成功
- `SimplePlayer` 1体での直近10000戦スコア: `Simple1=39139`, `ThinkTA1=38390`, `Default1=16223`, `Default2=16248`
- `SimplePlayer` 1体での直近10000戦順位: `Simple1` 1位、`ThinkTA1` 2位
