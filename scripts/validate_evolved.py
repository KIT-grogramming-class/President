#!/usr/bin/env python3
"""
進化チューニングで得た重みを「公式卓 (TA1あり)」で検証するスクリプト。

evolve_weights.py の出力 (scripts/evolved_weights.json) からベスト個体の
重みを読み込み、PLAYERS=official (既定) の卓で N 試合 × M 試行回し、
手調整版 v2.7 と統計的に比較する。

学習中は TA1 を全く見ていないので、ここで TA1 を上回れば
「TA に特化していない汎化が達成された」根拠になる。
"""

import json
import math
import os
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PROGRAM = REPO_ROOT / "sample" / "sampleProgram" / "daifugou"
EVOLVED_FILE = REPO_ROOT / "scripts" / "evolved_weights.json"

GAMES_PER_RUN = 10000
N_RUNS = 3

PLAYER_REGEX = re.compile(
    r'place:\s*(\S+)\s*:\s*(\d+)\s*:\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)'
)


def run_official_table(weights: dict | None, n_games: int) -> dict:
    """公式卓 (Default + Simple×2 + ThinkTA1 + Group1) で n_games 走らせる。
    weights=None なら C++ デフォルト (現状の v2.7 手調整値)。
    weights=dict なら env var で上書き。
    各プレイヤーの 1位回数を dict で返す。"""
    env = os.environ.copy()
    env.pop('PLAYERS', None)  # official に固定
    if weights:
        for k, v in weights.items():
            env[k] = f"{v:.4f}"

    result = subprocess.run(
        [str(PROGRAM), '-a', '-n', str(n_games)],
        env=env, capture_output=True, text=True, timeout=1800
    )
    output = result.stdout + result.stderr

    wins = {}
    for line in output.splitlines():
        m = PLAYER_REGEX.search(line)
        if m:
            name = m.group(1).strip()
            wins[name] = {
                'score': int(m.group(2)),
                '1st': int(m.group(3)),
                '2nd': int(m.group(4)),
                '3rd': int(m.group(5)),
                '4th': int(m.group(6)),
                '5th': int(m.group(7)),
            }
    return wins


def summarize(runs: list, label: str, n_games: int):
    keys = ['Group1', 'ThinkTA1', 'Simple1', 'Simple2', 'Default1']
    print(f"\n=== {label} ({len(runs)} runs × {n_games} games) ===")
    print(f"{'player':<10}  {'1位率(平均)':>12}  {'1位率 SE':>10}  "
          f"{'score平均':>10}")
    for k in keys:
        first_rates = [r[k]['1st'] / n_games for r in runs if k in r]
        scores = [r[k]['score'] for r in runs if k in r]
        if not first_rates:
            continue
        mean = sum(first_rates) / len(first_rates)
        if len(first_rates) >= 2:
            var = sum((x - mean) ** 2 for x in first_rates) / (len(first_rates) - 1)
            se = math.sqrt(var / len(first_rates))
        else:
            se = float('nan')
        score_mean = sum(scores) / len(scores)
        print(f"{k:<10}  {100*mean:>10.2f}%   {100*se:>8.2f}%   {score_mean:>10.1f}")


def main():
    if not PROGRAM.exists():
        print(f"ERROR: {PROGRAM} が見つかりません。先に make してください。")
        sys.exit(1)
    if not EVOLVED_FILE.exists():
        print(f"ERROR: {EVOLVED_FILE} が見つかりません。先に evolve_weights.py を走らせてください。")
        sys.exit(1)

    with open(EVOLVED_FILE) as f:
        data = json.load(f)
    evolved = data['best_weights_ever']
    print("進化で得た最良 weights:")
    for k, v in evolved.items():
        print(f"  {k:<20}= {v:.4f}")

    # --- ベースライン (v2.7 手調整) ---
    print(f"\n=== ベースライン (手調整 v2.7) を測定 ===")
    baseline_runs = []
    for i in range(N_RUNS):
        print(f"  run {i+1}/{N_RUNS} ...", flush=True)
        baseline_runs.append(run_official_table(None, GAMES_PER_RUN))

    # --- 進化版 ---
    print(f"\n=== 進化版 weights を測定 ===")
    evolved_runs = []
    for i in range(N_RUNS):
        print(f"  run {i+1}/{N_RUNS} ...", flush=True)
        evolved_runs.append(run_official_table(evolved, GAMES_PER_RUN))

    summarize(baseline_runs, "ベースライン (v2.7 手調整)", GAMES_PER_RUN)
    summarize(evolved_runs, "進化版 (TA1 不在学習)", GAMES_PER_RUN)

    # 差の検定
    base_g1 = [r['Group1']['1st'] / GAMES_PER_RUN for r in baseline_runs]
    evo_g1 = [r['Group1']['1st'] / GAMES_PER_RUN for r in evolved_runs]
    base_mean = sum(base_g1) / len(base_g1)
    evo_mean = sum(evo_g1) / len(evo_g1)
    diff = evo_mean - base_mean
    print(f"\nGroup1 1位率: baseline {100*base_mean:.2f}% → evolved {100*evo_mean:.2f}% "
          f"(差 {100*diff:+.2f}pp)")
    print(f"Group1 v.s. ThinkTA1 (進化版):")
    evo_ta = [r['ThinkTA1']['1st'] / GAMES_PER_RUN for r in evolved_runs]
    print(f"  Group1 {100*evo_mean:.2f}% vs ThinkTA1 {100*sum(evo_ta)/len(evo_ta):.2f}% "
          f"(差 {100*(evo_mean - sum(evo_ta)/len(evo_ta)):+.2f}pp)")


if __name__ == '__main__':
    main()
