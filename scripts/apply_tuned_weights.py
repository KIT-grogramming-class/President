#!/usr/bin/env python3
"""
tune_weights.py が生成した best_weights.json を読み、
groupplayer.cpp の Weights 構造体のデフォルト値を更新する。

使い方:
    python3 scripts/apply_tuned_weights.py
"""

import json
import re
import sys
from pathlib import Path

JSON_PATH = Path(__file__).resolve().parent / "best_weights.json"
CPP_PATH = Path(__file__).resolve().parent.parent / "sample" / "sampleProgram" / "groupplayer.cpp"

# 環境変数名 -> Weights構造体のメンバ名 のマッピング
NAME_MAP = {
    'W_PASS':         'pass',
    'W_WEAK':         'weak',
    'W_JOKER':        'joker',
    'W_PAIR':         'pair',
    'W_DANGER':       'danger',
    'W_MINOPP':       'minOpp',
    'W_ENDGAME':      'endgame',
    'W_ENDGAME_DEEP': 'endgameDeep',
    'W_LEADER_WEAK':  'leaderWeak',
    'W_MULTI_LEAD':   'multiLead',
    'W_ONE_MORE':     'oneMore',
}


def main():
    if not JSON_PATH.exists():
        print(f"ERROR: {JSON_PATH} not found.")
        sys.exit(1)

    with open(JSON_PATH) as f:
        data = json.load(f)

    weights = data['best_weights']
    print(f"Best weights (wins={data['best_wins']}, score={data['best_score']}):")
    for k, v in weights.items():
        print(f"  {k:<20}= {v:.4f}")

    if not CPP_PATH.exists():
        print(f"ERROR: {CPP_PATH} not found.")
        sys.exit(1)

    src = CPP_PATH.read_text()
    new_src = src

    # Weights構造体のデフォルト値を置換
    # struct Weights {
    #     double pass        = 3.0;
    #     ...
    # };
    for env_name, member in NAME_MAP.items():
        val = weights[env_name]
        # 既存のフィールド宣言を見つけて置換
        pattern = rf'(double\s+{re.escape(member)}\s*=\s*)[\d.]+;'
        replacement = rf'\g<1>{val:.4f};'
        new_src, n = re.subn(pattern, replacement, new_src, count=1)
        if n == 0:
            print(f"  ! WARNING: '{member}' field not found in cpp, skipping")
        else:
            print(f"  ✓ updated {member} -> {val:.4f}")

    if new_src == src:
        print("\nNo changes made.")
        return

    CPP_PATH.write_text(new_src)
    print(f"\nUpdated {CPP_PATH}")
    print("Run 'cd sample/sampleProgram && make && ./daifugou -a -n 5000' to verify.")


if __name__ == '__main__':
    main()
