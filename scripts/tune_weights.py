#!/usr/bin/env python3
"""
Group1 の評価関数の重みを自動チューニングするスクリプト。

ランダムサーチ（または山登り）で重み空間を探索し、
1位回数を最大化する組合せを見つける。

使い方:
    cd sample/sampleProgram && make
    python3 ../../scripts/tune_weights.py

各イテレーションで:
- 現在のベストを少しランダム化した候補を生成
- 候補で N ゲーム回す
- 1位回数が改善したら採用
- best_weights.json に随時保存
"""

import json
import os
import random
import re
import subprocess
import sys
import time
from pathlib import Path

# 探索するパラメータ：(初期値, min, max, ステップ)
PARAM_SPACE = {
    'W_PASS':         (3.0, 1.0, 6.0),
    'W_WEAK':         (1.2, 0.5, 2.5),
    'W_JOKER':        (8.0, 4.0, 12.0),
    'W_PAIR':         (4.0, 1.0, 7.0),
    'W_DANGER':       (3.0, 1.0, 5.0),
    'W_MINOPP':       (4.0, 1.0, 8.0),
    'W_ENDGAME':      (3.0, 0.0, 6.0),
    'W_ENDGAME_DEEP': (3.0, 0.0, 6.0),
    'W_LEADER_WEAK':  (0.5, 0.0, 1.5),
    'W_MULTI_LEAD':   (1.5, 0.0, 4.0),
    'W_ONE_MORE':     (25.0, 5.0, 50.0),
}

PROGRAM = Path(__file__).resolve().parent.parent / "sample" / "sampleProgram" / "daifugou"
BEST_FILE = Path(__file__).resolve().parent.parent / "scripts" / "best_weights.json"

# 設定
N_GAMES = 800        # 1評価あたりの試合数
N_ITERS = 50         # チューニング反復数
N_PERTURB = 3        # 1反復で同時に動かすパラメータ数
PERTURB_PCT = 0.25   # 摂動の振れ幅（±25%）


def evaluate(weights: dict, n_games: int = N_GAMES) -> tuple[int, int]:
    """指定の重みで N ゲーム実行し、Group1 の (1位数, score) を返す."""
    env = os.environ.copy()
    for k, v in weights.items():
        env[k] = f"{v:.4f}"

    try:
        result = subprocess.run(
            [str(PROGRAM), '-a', '-n', str(n_games)],
            env=env, capture_output=True, text=True, timeout=300
        )
    except subprocess.TimeoutExpired:
        return (0, 0)

    output = result.stdout + result.stderr
    # Final Result の Group1 行を探す
    for line in output.split('\n'):
        if 'Group1' in line and 'place' in line:
            # 例: "1st place: Group1  : 19288 : 1535 1316 ..."
            m = re.search(r'Group1\s*:\s*(\d+)\s*:\s*(\d+)', line)
            if m:
                return (int(m.group(2)), int(m.group(1)))
    return (0, 0)


def perturb(weights: dict) -> dict:
    """重みを摂動して新しい候補を返す."""
    new_w = weights.copy()
    keys = random.sample(list(PARAM_SPACE.keys()), N_PERTURB)
    for k in keys:
        _, lo, hi = PARAM_SPACE[k]
        cur = new_w[k]
        # 現在値の ±PERTURB_PCT 範囲でランダム
        delta = cur * PERTURB_PCT
        new_val = cur + random.uniform(-delta, delta)
        # ただし最低でも各方向に hi-lo の 5% は動く
        min_step = (hi - lo) * 0.05
        if abs(new_val - cur) < min_step:
            new_val = cur + random.choice([-min_step, min_step])
        # 範囲内に収める
        new_val = max(lo, min(hi, new_val))
        new_w[k] = new_val
    return new_w


def main():
    if not PROGRAM.exists():
        print(f"ERROR: {PROGRAM} が見つかりません。先に make してください。")
        sys.exit(1)

    print(f"=== 重み自動チューニング開始 ===")
    print(f"ゲーム数/評価: {N_GAMES}, 反復数: {N_ITERS}")
    print(f"対戦相手: ThinkTA1, Simple x2, Default x1\n")

    # 初期値（現状の値）
    best_weights = {k: v[0] for k, v in PARAM_SPACE.items()}

    print("初期重みでベースライン測定中...")
    t0 = time.time()
    best_wins, best_score = evaluate(best_weights)
    t_per_eval = time.time() - t0
    print(f"  Baseline: 1位 {best_wins} / {N_GAMES} ({100*best_wins/N_GAMES:.1f}%), score {best_score}")
    print(f"  1評価あたり {t_per_eval:.1f}秒（推定総時間 {t_per_eval*N_ITERS:.0f}秒 = {t_per_eval*N_ITERS/60:.1f}分）\n")

    history = [{
        'iter': 0,
        'wins': best_wins,
        'score': best_score,
        'weights': dict(best_weights),
        'accepted': True,
    }]

    for it in range(1, N_ITERS + 1):
        candidate = perturb(best_weights)
        wins, score = evaluate(candidate)

        accepted = wins > best_wins
        marker = "✓ ACCEPT" if accepted else "  reject"
        print(f"[{it:3d}/{N_ITERS}] {marker}: 1位 {wins:4d} ({100*wins/N_GAMES:.1f}%), score {score:6d}",
              end='')

        history.append({
            'iter': it,
            'wins': wins,
            'score': score,
            'weights': candidate,
            'accepted': accepted,
        })

        if accepted:
            best_wins = wins
            best_score = score
            best_weights = candidate
            print(f"  [NEW BEST: {best_wins}]")
            # 都度保存
            with open(BEST_FILE, 'w') as f:
                json.dump({
                    'best_wins': best_wins,
                    'best_score': best_score,
                    'best_weights': best_weights,
                    'n_games': N_GAMES,
                    'history': history,
                }, f, indent=2)
        else:
            print()

    print(f"\n=== チューニング完了 ===")
    print(f"最良 1位回数: {best_wins} / {N_GAMES} ({100*best_wins/N_GAMES:.1f}%)")
    print(f"最良 score:   {best_score}")
    print(f"最良 weights:")
    for k, v in best_weights.items():
        print(f"  {k:<20}= {v:.4f}")
    print(f"\n結果を {BEST_FILE} に保存しました。")


if __name__ == '__main__':
    main()
