# 評価関数と学習される重み — Group1

Group1 プレイヤーの手選択は、候補手を点数化する**評価関数 `scoreMove()`** と、
self-play 進化で**チューニングされる 11 個の重み `Weights`** で構成される。
本書はその構造と、学習対象／非学習対象の切り分けをまとめる。

実装: `sample/sampleProgram/groupplayer.cpp` / `groupplayer.h`

---

## 1. 全体像

```
follow()
  ├─ enumerateMoves()      合法手を全列挙
  ├─ 終盤ルート（手札が少ない場合に優先）
  │    ├─ mcPlayoutSelect()    MCプレイアウトで平均順位を最小化
  │    └─ endgameLookahead()   出した後の必要手数で評価
  └─ 通常ルート
       └─ scoreMove()           ← 評価関数。各候補手を点数化し最高得点を選ぶ
```

評価関数の本体は `scoreMove(move, gstat)`。1 つの候補手を入力に 1 つの実数スコアを返す。
中身は **重み × 特徴量 の重み付き線形和**：

```
score = Σ ( wᵢ × 特徴量ᵢ )
```

---

## 2. 評価関数 `scoreMove()` の項

| # | 項 | 式 | 意味 |
|---|---|---|---|
| 1 | 通る確率 | `passWeight * pass` | この手で場が流れる確率（0〜1）。`pass` は `analyticalPassProb` / `mcPassProb` で算出 |
| 2 | 弱い手優先 | `(15 - strength) * w.weak` | 弱いカードほど高得点。弱い札から処分 |
| 3 | ジョーカー温存 | `- jokerPenalty`（使用時） | ジョーカーを使う手を減点 |
| 4 | ペア温存 | `- pairBroken * pairWeight` | 同ランクの組を崩すと減点 |
| 5 | ライバル接近 | `dangerCount * w.danger * pass` | 残り 3 枚以下の相手がいると止め強化 |
| 6 | リーチ警戒 | `+ w.minOpp`（`minOpp<=1`） | 誰かリーチなら出すこと自体に価値 |
| 7 | 終盤ボーナス | `+ w.endgame` / `+ w.endgameDeep` | 手札が少ないとき積極的に出させる |
| 8 | リーダー時 | `(15-strength)*w.leaderWeak` ＋ `w.multiLead` | 場が空のとき：弱い札・複数枚リードを推奨 |
| 9 | 上がり判定 | `+ 100.0`（上がり）/ `+ w.oneMore`（あと 1 手） | 終局に近い手を強く優遇 |

### 終盤での動的な重み変化（手札枚数 `hsize` に依存）

評価関数は手札が減るにつれて挙動を変える。**これらの倍率は固定値であり学習対象ではない。**

| 対象 | 通常 | `hsize<=5` | `hsize<=4` | `hsize<=3` | `hsize<=2` | `hsize<=1` |
|---|---|---|---|---|---|---|
| `passWeight` | `w.pass` | `×2.0` | — | `×3.0` | — | `×4.0` |
| `jokerPenalty` | `w.joker` | — | `×0.5` | — | `0.0` | — |
| `pairWeight` | `w.pair` | — | `×0.5` | — | `×0.125` | — |

---

## 3. 学習される重み `Weights`（11 個）

`groupplayer.h` の `struct Weights`。インスタンスごとに保持し、self-play で
別重みの Group1 を同一プロセスに並べられる。環境変数（`W_PASS`, `A_W_PASS`,
`B_W_PASS` …）から `Weights::fromEnv()` で注入される。

| 重み | env キー | 既定値 | 探索範囲 (lo, hi) | 役割 |
|---|---|---|---|---|
| `pass` | `W_PASS` | 3.0 | 1.0 – 6.0 | 通る確率の係数 |
| `weak` | `W_WEAK` | 1.2 | 0.5 – 2.5 | 弱い手を出す価値 |
| `joker` | `W_JOKER` | 8.0 | 4.0 – 12.0 | ジョーカー温存ペナルティ |
| `pair` | `W_PAIR` | 4.0 | 1.0 – 7.0 | ペアを崩すペナルティ |
| `danger` | `W_DANGER` | 3.0 | 1.0 – 5.0 | ライバル接近時の止め強化 |
| `minOpp` | `W_MINOPP` | 4.0 | 1.0 – 8.0 | 相手リーチ時に出す価値 |
| `endgame` | `W_ENDGAME` | 3.0 | 0.0 – 6.0 | 終盤（手札≤5）ボーナス |
| `endgameDeep` | `W_ENDGAME_DEEP` | 3.0 | 0.0 – 6.0 | 深い終盤（手札≤3）追加ボーナス |
| `leaderWeak` | `W_LEADER_WEAK` | 0.5 | 0.0 – 1.5 | リーダー時に弱い札でリード |
| `multiLead` | `W_MULTI_LEAD` | 1.5 | 0.0 – 4.0 | 複数枚リードのボーナス |
| `oneMore` | `W_ONE_MORE` | 25.0 | 5.0 – 50.0 | あと 1 手で上がれる残り手札の価値 |

探索範囲は `scripts/evolve_weights.py` の `PARAM_SPACE` で定義。

---

## 4. 学習の仕組み（self-play 進化）

`scripts/evolve_weights.py` による世代型進化アルゴリズム：

1. **第 0 世代**：探索範囲内の一様乱数で初期化（手調整値を一切使わない）
2. **評価**：各個体を self-play 卓（Default + Simple×2 + Group1_A + Group1_B、
   TA1 不在）で対戦させ 1 位回数を集計。各個体は常に Group1A スロットで公平に評価
3. **選抜**：上位 `ELITE_K` を生存させる
4. **変異**：下位はエリートにガウシアン摂動を加えた子で置換
5. 2〜4 を `N_GENERATIONS` 世代繰り返す

途中経過は `scripts/evolved_weights.json` に逐次保存。最終ベストは
`scripts/validate_evolved.py` で公式卓（TA1 あり）と対戦させ汎化を検証する。

### 現状のベスト重み（`scripts/evolved_weights.json`）

第 0 世代で発見、3000 戦中 995 勝（約 33.2%）：

| 重み | 進化値 | 既定値 |
|---|---|---|
| `W_PASS` | 3.52 | 3.0 |
| `W_WEAK` | 1.06 | 1.2 |
| `W_JOKER` | 10.05 | 8.0 |
| `W_PAIR` | 4.71 | 4.0 |
| `W_DANGER` | 2.00 | 3.0 |
| `W_MINOPP` | 7.37 | 4.0 |
| `W_ENDGAME` | 5.90 | 3.0 |
| `W_ENDGAME_DEEP` | 4.86 | 3.0 |
| `W_LEADER_WEAK` | 1.35 | 0.5 |
| `W_MULTI_LEAD` | 1.24 | 1.5 |
| `W_ONE_MORE` | 37.84 | 25.0 |

---

## 5. 学習されないもの（固定の設計パラメータ）

評価関数のうち、**重みではない要素は進化の対象外**で人手設計のまま固定：

- **終盤の動的倍率**：`passWeight ×2/3/4`、`jokerPenalty ×0.5/0`、`pairWeight ×0.5/0.125`
- **マジックナンバー**：上がり `+100.0`、`endgameLookahead` の `×10.0 / ×2.0 / ×0.5 / +1000.0 / +50.0`
- **しきい値**：`endgameThreshold`、`mcPlayoutThreshold`、`mcSamples`、
  `mcPlayoutSamples`、`passBaseline`（`-10.0 / -3.0`）、`hsize<=5/3/1` の分岐点
- **アルゴリズム構造**：候補手の列挙ロジック、MC プレイアウトの貪欲方策
  （`greedyPickMove` = 最弱を出す）、ルックアヘッドの評価式

学習されるのは「評価関数の線形結合の **係数 11 個**」のみ。特徴量の作り方・
分岐構造・非線形な倍率・MC/ルックアヘッドの方策は固定である。

> 探索範囲を広げる場合、上記の固定倍率やしきい値を `Weights` に取り込み、
> `PARAM_SPACE` に追加するのが自然な次の一手。
