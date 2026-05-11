# AI Tuning Lab

`sample/sampleProgramForMac` をコピーした実験用ディレクトリです。元の提出用コードはここでは変更しません。

## 中身

- `sampleProgramForMac/`: 現在の `SimplePlayer` 実装をコピーした対戦プログラム
- `macLib/`: macOS 用 ThinkTA1 オブジェクト
- `tune.py`: `simpleplayer.cpp` の評価関数定数を差し替えて自動対戦するチューナー
- `best-params.json`: 現時点のベスト定数
- `known-good-params.json`: 10000戦で強かった基準定数。短期探索では上書きしない
- `results.csv`: 実行すると生成される対戦ログ

## 使い方

まずコピー先だけでビルド確認します。

```sh
cd ai-tuning-lab/sampleProgramForMac
make
./daifugou -a -n 1000
```

パラメータ探索は `ai-tuning-lab` 直下で実行します。

```sh
cd ai-tuning-lab
python3 tune.py --mode baseline --games 1000 --repeats 2
python3 tune.py --mode coordinate --games 1000 --repeats 2 --passes 2
python3 tune.py --mode random --games 1000 --repeats 2 --trials 30 --seed 1
python3 tune.py --mode genetic --games 500 --repeats 1 --verify-games 3000 --verify-repeats 2 --population 10 --generations 5 --seed 1
```

`tune.py` は `Simple1 - ThinkTA1` の平均スコア差を最大化します。探索中はノイズが大きいので、良さそうな定数が出たら次のように長めに確認してください。

```sh
python3 tune.py --mode baseline --games 10000 --repeats 3
```

## 進化的アルゴリズム

`--mode genetic` は、評価関数の重みセットを1個体として扱います。

- 適応度: `Simple1 - ThinkTA1`
- 選択: トーナメント選択
- 交叉: 親2個体の重みをパラメータ単位で混ぜる
- 突然変異: 一部の重みを探索ステップ単位で増減
- エリート保存: 各世代の上位個体を次世代に残す
- MC定数: `kMcPlayouts`, `kMcThreads`, `kMcWinRateWeight` なども探索対象

このモードでは短期戦の結果だけでは採用しません。各世代の上位だけを `--verify-games` と `--verify-repeats` で再評価し、`known-good-params.json` の基準値を超えた場合だけ `best-params.json` と C++ 定数へ反映します。

最初は軽めに回してください。

```sh
python3 tune.py --mode genetic --games 500 --repeats 1 --verify-games 2000 --verify-repeats 2 --population 8 --generations 4 --seed 1
```

良い値が見えたら、ゲーム数とリピートを増やして確認します。

```sh
python3 tune.py --mode genetic --games 1000 --repeats 1 --verify-games 5000 --verify-repeats 2 --population 12 --generations 6 --seed 2
python3 tune.py --mode baseline --games 10000 --repeats 3
```

## 何を見ればいいか

画面に出る主な数字は以下です。

- `known-good-guard`: 既知の強い基準値を長めに測った結果
- `genX-iY`: 短期評価。ノイズが大きいのでこれだけで判断しない
- `genX-topY-verify`: 上位個体の再評価。こちらを重視する
- `kept_best`: 今回保存される候補のスコア差
- `best_margin`: 最終的に残った候補のスコア差

まず目標は `best_margin` が `known-good-guard` より高いことです。そのあと `baseline --games 10000 --repeats 3` で確認します。

## 操作手順

よく分からない場合は、この順番で実行してください。

```sh
cd /Users/kakiuchiakira/Code/class/cpp/President/ai-tuning-lab
python3 tune.py --restore-known-good
python3 tune.py --mode genetic --games 500 --repeats 1 --verify-games 3000 --verify-repeats 2 --population 10 --generations 5 --seed 1
python3 tune.py --mode baseline --games 10000 --repeats 3
```

結果が弱ければ seed を変えてもう一度探索します。

```sh
python3 tune.py --mode genetic --games 500 --repeats 1 --verify-games 3000 --verify-repeats 2 --population 10 --generations 5 --seed 2
python3 tune.py --mode baseline --games 10000 --repeats 3
```

保存済みの `best-params.json` からさらに探索を続けたい場合は `--from-best` を付けます。

```sh
python3 tune.py --mode genetic --from-best --games 500 --repeats 1 --verify-games 3000 --verify-repeats 2 --population 10 --generations 5 --seed 3
```

MCを入れた後は `--from-best` 推奨です。今の強い個体を保護基準にして、弱い候補で上書きされないようにします。

```sh
python3 tune.py --mode genetic --from-best --games 500 --repeats 1 --verify-games 3000 --verify-repeats 2 --population 8 --generations 4 --seed 10
python3 tune.py --mode baseline --from-best --games 10000 --repeats 2
```

## 終盤モンテカルロ

`SimplePlayer` は通常時は従来通り `scoreMove()` で選びます。自分の手札が6枚以下、または相手の最小手札枚数が3枚以下の終盤だけ、`scoreMove()` 上位候補に対して簡易プレイアウトを行います。

- 候補数: `kMcCandidateLimit`
- プレイアウト回数: `kMcPlayouts`
- 並列数: `kMcThreads`
- 最大手数: `kMcMaxPlies`
- 勝率重み: `kMcWinRateWeight`
- 上書き条件: `kMcOverrideWindow`, `kMcOverrideThreshold`

MCは補助判断です。`scoreMove()` の1位を基本にして、基礎点が近い候補だけをMCで比較します。

`best_margin` が目標に近づいたら、その出力を貼ってください。こちらで提出用 `sample/sampleProgramForMac/simpleplayer.cpp` に反映します。

最終的に採用する場合だけ、`best-params.json` の値を元の `sample/sampleProgramForMac/simpleplayer.cpp` に反映します。

## 注意

- このディレクトリは実験用です。提出前は元の `sample/sampleProgramForMac` 側で `make` と `./daifugou -a -n 10000` を確認してください。
- `ThinkTA1` への特別扱いや味方役のような名前依存ロジックは入れません。
- 小さいゲーム数の結果はブレます。1000戦で勝っても、10000戦で逆転することがあります。
