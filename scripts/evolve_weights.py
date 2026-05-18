#!/usr/bin/env python3
"""
世代型進化による self-play 重みチューナ。

第0世代はパラメータ範囲内の一様乱数で初期化する（手調整値を一切使わない、
「何も知らない」集団から始める）。各世代で self-play 卓 (PLAYERS=selfplay:
Default + Simple×2 + Group1_A + Group1_B、TA1 不在) で相互対戦し、
1位率の高い個体を選抜、下位を選抜個体の摂動子で置換する。

これにより重みは「TA1 に対する勝ち方」ではなく「大富豪のルール自体で勝つ」
方向に進化することを狙う。

使い方:
    cd sample/sampleProgram && make
    python3 scripts/evolve_weights.py

途中経過は scripts/evolved_weights.json に逐次保存される。最終的に得られた
ベスト weights は scripts/validate_evolved.py で公式卓 (TA1あり) と対戦させ、
TA1 不在で学習しても TA1 を上回るか（汎化したか）を検証する。
"""

import json
import os
import random
import re
import subprocess
import sys
import time
from pathlib import Path

# 各重みの探索範囲 (lo, hi)。第0世代は一様乱数で生成される。
PARAM_SPACE = {
    'W_PASS':         (1.0, 6.0),
    'W_WEAK':         (0.5, 2.5),
    'W_JOKER':        (4.0, 12.0),
    'W_PAIR':         (1.0, 7.0),
    'W_DANGER':       (1.0, 5.0),
    'W_MINOPP':       (1.0, 8.0),
    'W_ENDGAME':      (0.0, 6.0),
    'W_ENDGAME_DEEP': (0.0, 6.0),
    'W_LEADER_WEAK':  (0.0, 1.5),
    'W_MULTI_LEAD':   (0.0, 4.0),
    'W_ONE_MORE':     (5.0, 50.0),
}

REPO_ROOT = Path(__file__).resolve().parent.parent
PROGRAM = REPO_ROOT / "sample" / "sampleProgram" / "daifugou"
OUTFILE = REPO_ROOT / "scripts" / "evolved_weights.json"

# --- 進化パラメータ（小さく始めて徐々に大きくできる） ---
POP_SIZE = 12                # 集団サイズ
N_GENERATIONS = 20           # 世代数
GAMES_PER_EVAL = 3000        # 1個体あたりの総対戦数
PAIRINGS_PER_INDIVIDUAL = 3  # 各個体がぶつかる対戦相手数（GAMES_PER_EVAL を分割）
ELITE_K = POP_SIZE // 2      # 上位 K を生存させる
MUTATION_PCT = 0.15          # 摂動の標準偏差（パラメータ範囲幅の何%）
RNG_SEED = 0                 # 進化側の乱数シード（再現性のため固定）

# CLI で上書き可能にする簡易フック（環境変数 EVOLVE_* で上書き）
def _env_int(name, default):
    v = os.environ.get(name)
    return int(v) if v else default
def _env_float(name, default):
    v = os.environ.get(name)
    return float(v) if v else default

POP_SIZE = _env_int('EVOLVE_POP', POP_SIZE)
N_GENERATIONS = _env_int('EVOLVE_GENS', N_GENERATIONS)
GAMES_PER_EVAL = _env_int('EVOLVE_GAMES', GAMES_PER_EVAL)
PAIRINGS_PER_INDIVIDUAL = _env_int('EVOLVE_PAIRS', PAIRINGS_PER_INDIVIDUAL)
ELITE_K = _env_int('EVOLVE_ELITE', ELITE_K)
MUTATION_PCT = _env_float('EVOLVE_MUT', MUTATION_PCT)
RNG_SEED = _env_int('EVOLVE_SEED', RNG_SEED)


def rand_weights(rng: random.Random) -> dict:
    """探索範囲内の一様乱数で重みベクトルを生成する（「何も知らない」初期世代用）。"""
    return {k: rng.uniform(lo, hi) for k, (lo, hi) in PARAM_SPACE.items()}


def mutate(parent: dict, rng: random.Random) -> dict:
    """親重みにガウシアン摂動を加えて子を作る。範囲外はクリップ。"""
    child = {}
    for k, (lo, hi) in PARAM_SPACE.items():
        sigma = (hi - lo) * MUTATION_PCT
        v = parent[k] + rng.gauss(0.0, sigma)
        child[k] = max(lo, min(hi, v))
    return child


WIN_REGEX_A = re.compile(r'Group1A\s*:\s*\d+\s*:\s*(\d+)')
WIN_REGEX_B = re.compile(r'Group1B\s*:\s*\d+\s*:\s*(\d+)')


def play_match(a_weights: dict, b_weights: dict, n_games: int) -> tuple[int, int]:
    """A_/B_ env で重みを与えて self-play 卓を n_games 回す。
    Group1A と Group1B の 1位回数 (a_wins, b_wins) を返す。"""
    env = os.environ.copy()
    env['PLAYERS'] = 'selfplay'
    for k, v in a_weights.items():
        env['A_' + k] = f"{v:.4f}"
    for k, v in b_weights.items():
        env['B_' + k] = f"{v:.4f}"
    try:
        result = subprocess.run(
            [str(PROGRAM), '-a', '-n', str(n_games)],
            env=env, capture_output=True, text=True, timeout=900
        )
    except subprocess.TimeoutExpired:
        return (0, 0)
    output = result.stdout + result.stderr
    a_wins = b_wins = 0
    for line in output.splitlines():
        ma = WIN_REGEX_A.search(line)
        if ma:
            a_wins = int(ma.group(1))
        mb = WIN_REGEX_B.search(line)
        if mb:
            b_wins = int(mb.group(1))
    return a_wins, b_wins


def evaluate_population(pop: list, rng: random.Random) -> list:
    """各個体について、ランダムに選んだ PAIRINGS_PER_INDIVIDUAL 人と
    games_per_pairing 試合ずつ対戦し、累計 1位回数を返す。
    各個体は常に Group1A スロット（player 3）で評価され公平性が保たれる。"""
    N = len(pop)
    K = PAIRINGS_PER_INDIVIDUAL
    games_per_pairing = max(1, GAMES_PER_EVAL // K)

    wins = [0] * N
    pairings_log = [[] for _ in range(N)]

    for i in range(N):
        opponents = rng.sample([j for j in range(N) if j != i], K)
        for j in opponents:
            a_wins, _ = play_match(pop[i], pop[j], games_per_pairing)
            wins[i] += a_wins
            pairings_log[i].append((j, a_wins))
        sys.stdout.write(f"    [{i+1:2d}/{N}] indiv #{i}: {wins[i]} wins "
                         f"({100*wins[i]/(K*games_per_pairing):.1f}%) "
                         f"vs opponents {opponents}\n")
        sys.stdout.flush()

    return wins, pairings_log


def next_generation(pop: list, scores: list, rng: random.Random) -> list:
    """上位 ELITE_K を残し、残りはエリートからの摂動子で埋める。"""
    order = sorted(range(len(pop)), key=lambda i: -scores[i])
    elites = [pop[i] for i in order[:ELITE_K]]
    new_pop = [dict(e) for e in elites]
    while len(new_pop) < POP_SIZE:
        parent = rng.choice(elites)
        new_pop.append(mutate(parent, rng))
    return new_pop


def main():
    if not PROGRAM.exists():
        print(f"ERROR: {PROGRAM} が見つかりません。先に make してください。")
        sys.exit(1)

    rng = random.Random(RNG_SEED)

    print(f"=== 世代型進化チューニング ===")
    print(f"POP_SIZE={POP_SIZE}, GENERATIONS={N_GENERATIONS}")
    print(f"GAMES/EVAL={GAMES_PER_EVAL}, PAIRINGS={PAIRINGS_PER_INDIVIDUAL}")
    print(f"  → 1個体あたり {GAMES_PER_EVAL // PAIRINGS_PER_INDIVIDUAL} games × "
          f"{PAIRINGS_PER_INDIVIDUAL} 対戦相手")
    print(f"ELITE_K={ELITE_K}, MUTATION_PCT={MUTATION_PCT}")
    print(f"RNG_SEED={RNG_SEED}")
    print(f"対戦相手: Default + Simple×2 (TA1 不在)\n")

    # 第0世代: ランダム初期化（手調整値は使わない）
    pop = [rand_weights(rng) for _ in range(POP_SIZE)]

    history = []
    best_ever_wins = -1
    best_ever_weights = None
    best_ever_gen = -1

    for gen in range(N_GENERATIONS):
        t0 = time.time()
        print(f"--- Generation {gen} ---")
        scores, pairings = evaluate_population(pop, rng)
        elapsed = time.time() - t0

        ranked_idx = sorted(range(POP_SIZE), key=lambda i: -scores[i])
        ranked_scores = [scores[i] for i in ranked_idx]
        gen_best = ranked_scores[0]
        gen_median = ranked_scores[POP_SIZE // 2]
        gen_worst = ranked_scores[-1]
        max_games = (GAMES_PER_EVAL // PAIRINGS_PER_INDIVIDUAL) * PAIRINGS_PER_INDIVIDUAL
        print(f"  top: {gen_best} ({100*gen_best/max_games:.1f}%), "
              f"median: {gen_median} ({100*gen_median/max_games:.1f}%), "
              f"worst: {gen_worst} ({100*gen_worst/max_games:.1f}%)")
        print(f"  elapsed: {elapsed:.1f}s")

        if gen_best > best_ever_wins:
            best_ever_wins = gen_best
            best_ever_weights = dict(pop[ranked_idx[0]])
            best_ever_gen = gen
            print(f"  ✓ NEW BEST: gen={gen}, wins={best_ever_wins} / {max_games}")

        history.append({
            'generation': gen,
            'scores': scores,
            'population': pop,
            'pairings': pairings,
            'gen_best_wins': gen_best,
            'gen_median_wins': gen_median,
            'gen_worst_wins': gen_worst,
            'time_seconds': elapsed,
        })

        with open(OUTFILE, 'w') as f:
            json.dump({
                'best_wins_ever': best_ever_wins,
                'best_weights_ever': best_ever_weights,
                'best_gen_ever': best_ever_gen,
                'max_games_per_eval': max_games,
                'config': {
                    'POP_SIZE': POP_SIZE,
                    'N_GENERATIONS': N_GENERATIONS,
                    'GAMES_PER_EVAL': GAMES_PER_EVAL,
                    'PAIRINGS_PER_INDIVIDUAL': PAIRINGS_PER_INDIVIDUAL,
                    'ELITE_K': ELITE_K,
                    'MUTATION_PCT': MUTATION_PCT,
                    'RNG_SEED': RNG_SEED,
                    'PARAM_SPACE': {k: list(v) for k, v in PARAM_SPACE.items()},
                },
                'history': history,
            }, f, indent=2)

        if gen < N_GENERATIONS - 1:
            pop = next_generation(pop, scores, rng)

    print(f"\n=== 完了 ===")
    print(f"歴代ベスト (gen {best_ever_gen}): {best_ever_wins} 1st-place wins / {max_games}")
    print(f"  ({100 * best_ever_wins / max_games:.2f}%)")
    print(f"ベスト weights:")
    for k, v in best_ever_weights.items():
        print(f"  {k:<20}= {v:.4f}")
    print(f"\n結果を {OUTFILE} に保存しました。")
    print(f"次は scripts/validate_evolved.py で公式卓 (TA1あり) と対戦させて汎化を確認。")


if __name__ == '__main__':
    main()
