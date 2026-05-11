#!/usr/bin/env python3
"""Tune SimplePlayer heuristic weights against ThinkTA1.

This script intentionally edits only the copied source under this directory:
    ai-tuning-lab/sampleProgramForMac/simpleplayer.cpp

The objective is the average score margin: Simple1 - ThinkTA1.
Because the game result is noisy, use small runs for search and confirm the
final candidate with larger runs such as 10000 games.
"""

from __future__ import annotations

import argparse
import csv
import json
import random
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


ROOT = Path(__file__).resolve().parent
SAMPLE_DIR = ROOT / "sampleProgramForMac"
SOURCE_FILE = SAMPLE_DIR / "simpleplayer.cpp"
RESULTS_FILE = ROOT / "results.csv"
BEST_FILE = ROOT / "best-params.json"
KNOWN_GOOD_FILE = ROOT / "known-good-params.json"


# name: (minimum, maximum, coordinate_step)
PARAM_SPECS: Dict[str, Tuple[int, int, int]] = {
    "kLeadCardCountWeight": (500, 1150, 60),
    "kLeadGroupBonus": (200, 950, 60),
    "kFollowBeatBonus": (1500, 3400, 120),
    "kFollowCardCountWeight": (60, 420, 30),
    "kFollowStrengthPenalty": (30, 180, 15),
    "kWeakCardBonus": (20, 160, 10),
    "kShapeWeight": (0, 5, 1),
    "kBreakPairPenalty": (250, 1200, 70),
    "kFinishTurnPenalty": (700, 2600, 120),
    "kEndgameCardBonus": (400, 1500, 70),
    "kNearEndgameCardBonus": (100, 800, 50),
    "kDangerTakeLeadBonus": (150, 1400, 80),
    "kPassBaseScore": (-300, 300, 40),
    "kPassConserveBonus": (0, 600, 60),
    "kPassEndgamePenalty": (700, 2800, 120),
    "kPassNearEndgamePenalty": (100, 1200, 80),
    "kPassDangerPenalty": (500, 2600, 120),
    "kMcCandidateLimit": (2, 5, 1),
    "kMcPlayouts": (2, 12, 2),
    "kMcThreads": (1, 4, 1),
    "kMcMaxPlies": (25, 70, 5),
    "kMcWinRateWeight": (0, 1400, 100),
    "kMcFinishEaseWeight": (0, 300, 30),
    "kMcOverrideWindow": (300, 2500, 200),
    "kMcOverrideThreshold": (0, 1000, 100),
}

CONST_RE = re.compile(r"^const int (k[A-Za-z0-9_]+) = (-?\d+);$", re.MULTILINE)
SCORE_RE = re.compile(
    r"^\s*\d+(?:st|nd|rd|th) place:\s*(.+?)\s*:\s*(-?\d+)\s*:"
    r"\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)",
    re.MULTILINE,
)


@dataclass
class MatchResult:
    margin: float
    simple_score: float
    think_score: float
    simple_firsts: float
    think_firsts: float
    raw_runs: int


def clamp(value: int, name: str) -> int:
    lower, upper, _ = PARAM_SPECS[name]
    return max(lower, min(upper, value))


def read_source_params() -> Dict[str, int]:
    text = SOURCE_FILE.read_text()
    found = {name: int(value) for name, value in CONST_RE.findall(text)}
    missing = [name for name in PARAM_SPECS if name not in found]
    if missing:
        raise RuntimeError(f"Missing tunable constants in {SOURCE_FILE}: {missing}")
    return {name: found[name] for name in PARAM_SPECS}


def read_params_file(path: Path) -> Dict[str, int]:
    loaded = json.loads(path.read_text())
    source_params = read_source_params()
    params: Dict[str, int] = {}
    for name in PARAM_SPECS:
        params[name] = clamp(int(loaded.get(name, source_params[name])), name)
    return params


def read_best_params() -> Dict[str, int]:
    if BEST_FILE.exists():
        return read_params_file(BEST_FILE)
    return read_source_params()


def read_known_good_params() -> Dict[str, int]:
    if KNOWN_GOOD_FILE.exists():
        return read_params_file(KNOWN_GOOD_FILE)
    if BEST_FILE.exists():
        return read_best_params()
    return read_source_params()


def write_params(params: Dict[str, int]) -> None:
    text = SOURCE_FILE.read_text()

    def replace(match: re.Match[str]) -> str:
        name = match.group(1)
        if name in params:
            return f"const int {name} = {clamp(int(params[name]), name)};"
        return match.group(0)

    SOURCE_FILE.write_text(CONST_RE.sub(replace, text))


def save_best(params: Dict[str, int]) -> None:
    BEST_FILE.write_text(json.dumps(params, indent=2, sort_keys=True) + "\n")


def run_command(command: List[str], cwd: Path, timeout: int) -> str:
    completed = subprocess.run(
        command,
        cwd=str(cwd),
        text=True,
        capture_output=True,
        timeout=timeout,
    )
    if completed.returncode != 0:
        output = completed.stdout + completed.stderr
        raise RuntimeError(f"Command failed: {' '.join(command)}\n{output[-4000:]}")
    return completed.stdout + completed.stderr


def build(timeout: int) -> None:
    run_command(["make"], SAMPLE_DIR, timeout)


def parse_match_output(output: str) -> Tuple[int, int, int, int]:
    scores: Dict[str, Tuple[int, int]] = {}
    for match in SCORE_RE.finditer(output):
        name = match.group(1).strip()
        score = int(match.group(2))
        firsts = int(match.group(3))
        scores[name] = (score, firsts)

    simple_names = [name for name in scores if name.startswith("Simple")]
    if not simple_names or "ThinkTA1" not in scores:
        tail = "\n".join(output.splitlines()[-30:])
        raise RuntimeError(f"Could not parse match output.\n{tail}")

    simple_name = max(simple_names, key=lambda name: scores[name][0])
    simple_score, simple_firsts = scores[simple_name]
    think_score, think_firsts = scores["ThinkTA1"]
    return simple_score, think_score, simple_firsts, think_firsts


def run_match(games: int, repeats: int, timeout: int) -> MatchResult:
    margins: List[int] = []
    simple_scores: List[int] = []
    think_scores: List[int] = []
    simple_firsts: List[int] = []
    think_firsts: List[int] = []

    for _ in range(repeats):
        output = run_command(["./daifugou", "-a", "-n", str(games)], SAMPLE_DIR, timeout)
        simple_score, think_score, simple_1st, think_1st = parse_match_output(output)
        margins.append(simple_score - think_score)
        simple_scores.append(simple_score)
        think_scores.append(think_score)
        simple_firsts.append(simple_1st)
        think_firsts.append(think_1st)

    return MatchResult(
        margin=sum(margins) / len(margins),
        simple_score=sum(simple_scores) / len(simple_scores),
        think_score=sum(think_scores) / len(think_scores),
        simple_firsts=sum(simple_firsts) / len(simple_firsts),
        think_firsts=sum(think_firsts) / len(think_firsts),
        raw_runs=repeats,
    )


def log_result(label: str, params: Dict[str, int], result: MatchResult) -> None:
    new_file = not RESULTS_FILE.exists()
    with RESULTS_FILE.open("a", newline="") as f:
        writer = csv.writer(f)
        if new_file:
            writer.writerow(
                [
                    "time",
                    "label",
                    "margin",
                    "simple_score",
                    "think_score",
                    "simple_firsts",
                    "think_firsts",
                    "runs",
                    *PARAM_SPECS.keys(),
                ]
            )
        writer.writerow(
            [
                int(time.time()),
                label,
                f"{result.margin:.2f}",
                f"{result.simple_score:.2f}",
                f"{result.think_score:.2f}",
                f"{result.simple_firsts:.2f}",
                f"{result.think_firsts:.2f}",
                result.raw_runs,
                *[params[name] for name in PARAM_SPECS],
            ]
        )


def evaluate(label: str, params: Dict[str, int], games: int, repeats: int, timeout: int) -> MatchResult:
    write_params(params)
    build(timeout)
    result = run_match(games, repeats, timeout)
    log_result(label, params, result)
    print(
        f"{label}: margin={result.margin:.1f}, "
        f"Simple={result.simple_score:.1f}, ThinkTA1={result.think_score:.1f}, "
        f"1st={result.simple_firsts:.1f}/{result.think_firsts:.1f}",
        flush=True,
    )
    return result


def candidate_values(current: Dict[str, int], name: str, step_scale: float) -> Iterable[int]:
    _, _, base_step = PARAM_SPECS[name]
    step = max(1, int(round(base_step * step_scale)))
    value = current[name]
    values = [clamp(value - step, name), clamp(value + step, name)]
    return [v for v in values if v != value]


def coordinate_search(
    initial: Dict[str, int],
    games: int,
    repeats: int,
    passes: int,
    timeout: int,
) -> Tuple[Dict[str, int], MatchResult]:
    best_params = dict(initial)
    best_result = evaluate("baseline", best_params, games, repeats, timeout)

    step_scale = 1.0
    for pass_index in range(1, passes + 1):
        improved = False
        for name in PARAM_SPECS:
            local_best_params = dict(best_params)
            local_best_result = best_result
            for value in candidate_values(best_params, name, step_scale):
                trial = dict(best_params)
                trial[name] = value
                result = evaluate(f"coord-p{pass_index}-{name}={value}", trial, games, repeats, timeout)
                if result.margin > local_best_result.margin:
                    local_best_params = trial
                    local_best_result = result
                    improved = True
            best_params = local_best_params
            best_result = local_best_result
        if not improved:
            step_scale *= 0.5
            print(f"no improvement; shrinking step scale to {step_scale:.2f}", flush=True)
    return best_params, best_result


def random_params(base: Dict[str, int], rng: random.Random, wide: bool) -> Dict[str, int]:
    params = dict(base)
    for name, (lower, upper, step) in PARAM_SPECS.items():
        if wide:
            params[name] = rng.randint(lower, upper)
        else:
            delta = rng.randint(-2, 2) * step
            params[name] = clamp(params[name] + delta, name)
    return params


def params_key(params: Dict[str, int]) -> Tuple[int, ...]:
    return tuple(params[name] for name in PARAM_SPECS)


def mutate_params(
    base: Dict[str, int],
    rng: random.Random,
    mutation_rate: float,
    mutation_scale: float,
) -> Dict[str, int]:
    params = dict(base)
    changed = False
    for name, (_, _, step) in PARAM_SPECS.items():
        if rng.random() <= mutation_rate:
            units = int(round(rng.gauss(0.0, mutation_scale)))
            if units == 0:
                units = 1 if rng.random() < 0.5 else -1
            params[name] = clamp(params[name] + units * step, name)
            changed = True

    # Avoid returning an unchanged child when mutation_rate is small.
    if not changed:
        name = rng.choice(list(PARAM_SPECS.keys()))
        _, _, step = PARAM_SPECS[name]
        params[name] = clamp(params[name] + (step if rng.random() < 0.5 else -step), name)
    return params


def crossover_params(
    parent_a: Dict[str, int],
    parent_b: Dict[str, int],
    rng: random.Random,
) -> Dict[str, int]:
    child: Dict[str, int] = {}
    for name in PARAM_SPECS:
        child[name] = parent_a[name] if rng.random() < 0.5 else parent_b[name]
    return child


def tournament_pick(
    scored: List[Tuple[MatchResult, Dict[str, int]]],
    rng: random.Random,
    tournament_size: int,
) -> Dict[str, int]:
    size = max(1, min(tournament_size, len(scored)))
    contestants = rng.sample(scored, size)
    return max(contestants, key=lambda item: item[0].margin)[1]


def random_search(
    initial: Dict[str, int],
    games: int,
    repeats: int,
    trials: int,
    timeout: int,
    seed: int,
    wide: bool,
) -> Tuple[Dict[str, int], MatchResult]:
    rng = random.Random(seed)
    best_params = dict(initial)
    best_result = evaluate("baseline", best_params, games, repeats, timeout)

    for trial_index in range(1, trials + 1):
        trial = random_params(best_params, rng, wide)
        result = evaluate(f"random-{trial_index}", trial, games, repeats, timeout)
        if result.margin > best_result.margin:
            best_params = trial
            best_result = result
            print(f"new best at trial {trial_index}: margin={best_result.margin:.1f}", flush=True)
    return best_params, best_result


def genetic_search(
    initial: Dict[str, int],
    guard_params: Dict[str, int],
    games: int,
    repeats: int,
    verify_top: int,
    verify_games: int,
    verify_repeats: int,
    promote_margin: float,
    population_size: int,
    generations: int,
    elite_size: int,
    mutation_rate: float,
    mutation_scale: float,
    tournament_size: int,
    timeout: int,
    seed: int,
    wide: bool,
) -> Tuple[Dict[str, int], MatchResult]:
    if population_size < 2:
        raise ValueError("--population must be at least 2")
    if generations < 1:
        raise ValueError("--generations must be at least 1")

    rng = random.Random(seed)
    elite_size = max(1, min(elite_size, population_size - 1))
    mutation_rate = max(0.01, min(1.0, mutation_rate))
    mutation_scale = max(0.1, mutation_scale)
    verify_top = max(1, min(verify_top, population_size))
    verify_games = max(games, verify_games)
    verify_repeats = max(1, verify_repeats)

    guard_params = dict(guard_params)
    guard_result = evaluate("known-good-guard", guard_params, verify_games, verify_repeats, timeout)

    population: List[Dict[str, int]] = []
    seen: Set[Tuple[int, ...]] = set()
    for seed_params in [guard_params, initial]:
        key = params_key(seed_params)
        if key not in seen:
            population.append(dict(seed_params))
            seen.add(key)
    while len(population) < population_size:
        candidate = random_params(initial, rng, wide)
        attempts = 0
        while params_key(candidate) in seen and attempts < 20:
            candidate = mutate_params(candidate, rng, mutation_rate, mutation_scale)
            attempts += 1
        seen.add(params_key(candidate))
        population.append(candidate)

    best_params = dict(guard_params)
    best_result: MatchResult = guard_result
    scout_cache: Dict[Tuple[int, ...], MatchResult] = {}
    verify_cache: Dict[Tuple[int, ...], MatchResult] = {params_key(guard_params): guard_result}

    print(
        f"genetic: population={population_size}, generations={generations}, "
        f"elite={elite_size}, mutation_rate={mutation_rate:.2f}, "
        f"mutation_scale={mutation_scale:.2f}, verify_top={verify_top}, "
        f"verify_games={verify_games}, verify_repeats={verify_repeats}",
        flush=True,
    )

    for generation in range(1, generations + 1):
        scored: List[Tuple[MatchResult, Dict[str, int]]] = []
        for index, params in enumerate(population):
            key = params_key(params)
            if key in scout_cache:
                result = scout_cache[key]
                print(f"gen{generation}-i{index}: cached margin={result.margin:.1f}", flush=True)
            else:
                result = evaluate(f"gen{generation}-i{index}", params, games, repeats, timeout)
                scout_cache[key] = result
            scored.append((result, params))

        scored.sort(key=lambda item: item[0].margin, reverse=True)

        verified_top: List[Tuple[MatchResult, Dict[str, int]]] = []
        for rank, (_, params) in enumerate(scored[:verify_top], start=1):
            key = params_key(params)
            if key in verify_cache:
                result = verify_cache[key]
                print(
                    f"gen{generation}-top{rank}-verify: cached margin={result.margin:.1f}",
                    flush=True,
                )
            else:
                result = evaluate(
                    f"gen{generation}-top{rank}-verify",
                    params,
                    verify_games,
                    verify_repeats,
                    timeout,
                )
                verify_cache[key] = result
            verified_top.append((result, params))

            # Only longer verification can promote a new best. This prevents
            # noisy short matches from overwriting the known-good baseline.
            if result.margin > best_result.margin + promote_margin:
                best_params = dict(params)
                best_result = result
                print(
                    f"verified new best at generation {generation}, rank {rank}: "
                    f"margin={best_result.margin:.1f}",
                    flush=True,
                )

        selection_scored: List[Tuple[MatchResult, Dict[str, int]]] = []
        for scout_result, params in scored:
            selection_result = verify_cache.get(params_key(params), scout_result)
            selection_scored.append((selection_result, params))
        selection_scored.sort(key=lambda item: item[0].margin, reverse=True)

        best_verified = max(verified_top, key=lambda item: item[0].margin)
        print(
            f"generation {generation}: scout_best={scored[0][0].margin:.1f}, "
            f"verified_best={best_verified[0].margin:.1f}, "
            f"kept_best={best_result.margin:.1f}",
            flush=True,
        )

        if generation == generations:
            break

        next_population: List[Dict[str, int]] = []
        next_seen: Set[Tuple[int, ...]] = set()

        def add_next(params: Dict[str, int]) -> None:
            key = params_key(params)
            if key not in next_seen and len(next_population) < population_size:
                next_seen.add(key)
                next_population.append(dict(params))

        add_next(best_params)
        for _, params in selection_scored[:elite_size]:
            add_next(params)

        while len(next_population) < population_size:
            parent_a = tournament_pick(selection_scored, rng, tournament_size)
            parent_b = tournament_pick(selection_scored, rng, tournament_size)
            child = crossover_params(parent_a, parent_b, rng)
            child = mutate_params(child, rng, mutation_rate, mutation_scale)

            attempts = 0
            while params_key(child) in next_seen and attempts < 20:
                child = mutate_params(child, rng, mutation_rate, mutation_scale)
                attempts += 1
            before = len(next_population)
            add_next(child)
            if len(next_population) == before:
                fallback = random_params(best_params, rng, wide)
                add_next(fallback)
            if len(next_population) == before:
                for name, (_, _, step) in PARAM_SPECS.items():
                    fallback = dict(best_params)
                    fallback[name] = clamp(fallback[name] + step, name)
                    add_next(fallback)
                    if len(next_population) > before:
                        break
        population = next_population

    return best_params, best_result


def main() -> int:
    parser = argparse.ArgumentParser(description="Tune SimplePlayer parameters against ThinkTA1.")
    parser.add_argument("--mode", choices=["baseline", "coordinate", "random", "genetic"], default="baseline")
    parser.add_argument("--games", type=int, default=1000, help="games per evaluation run")
    parser.add_argument("--repeats", type=int, default=2, help="evaluation repeats per candidate")
    parser.add_argument("--passes", type=int, default=2, help="coordinate-search passes")
    parser.add_argument("--trials", type=int, default=20, help="random-search trials")
    parser.add_argument("--population", type=int, default=10, help="genetic-search population size")
    parser.add_argument("--generations", type=int, default=5, help="genetic-search generations")
    parser.add_argument("--verify-top", type=int, default=3, help="genetic-search candidates to recheck")
    parser.add_argument("--verify-games", type=int, default=3000, help="games per genetic recheck")
    parser.add_argument("--verify-repeats", type=int, default=2, help="repeats per genetic recheck")
    parser.add_argument("--promote-margin", type=float, default=0.0, help="extra verified margin required over known-good")
    parser.add_argument("--elite", type=int, default=2, help="genetic-search elite survivors")
    parser.add_argument("--mutation-rate", type=float, default=0.25, help="per-parameter mutation probability")
    parser.add_argument("--mutation-scale", type=float, default=1.5, help="mutation size in step units")
    parser.add_argument("--tournament", type=int, default=3, help="genetic-search tournament size")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--timeout", type=int, default=180, help="timeout seconds per make/match command")
    parser.add_argument("--from-source", action="store_true", help="start from current source constants")
    parser.add_argument("--from-best", action="store_true", help="start from best-params.json instead of known-good")
    parser.add_argument("--wide", action="store_true", help="use full parameter ranges in random/genetic mode")
    parser.add_argument("--no-keep-best", action="store_true", help="restore the starting constants at the end")
    parser.add_argument("--restore-known-good", action="store_true", help="copy known-good params to source and best file")
    args = parser.parse_args()

    if not SOURCE_FILE.exists():
        raise RuntimeError(f"Missing source file: {SOURCE_FILE}")

    known_good_params = read_known_good_params()
    original_params = read_source_params()

    if args.restore_known_good:
        write_params(known_good_params)
        save_best(known_good_params)
        build(args.timeout)
        print(f"restored known-good params from {KNOWN_GOOD_FILE}")
        return 0

    if args.from_source:
        starting_params = read_source_params()
    elif args.from_best:
        starting_params = read_best_params()
    elif args.mode == "genetic":
        # Genetic search starts from the protected baseline unless explicitly
        # asked to continue from best-params.json.
        starting_params = dict(known_good_params)
    else:
        starting_params = read_best_params()

    if args.mode == "baseline":
        best_params = dict(starting_params)
        best_result = evaluate("baseline", best_params, args.games, args.repeats, args.timeout)
    elif args.mode == "coordinate":
        best_params, best_result = coordinate_search(
            starting_params,
            args.games,
            args.repeats,
            args.passes,
            args.timeout,
        )
    elif args.mode == "random":
        best_params, best_result = random_search(
            starting_params,
            args.games,
            args.repeats,
            args.trials,
            args.timeout,
            args.seed,
            args.wide,
        )
    else:
        guard_params = starting_params if args.from_best else known_good_params
        best_params, best_result = genetic_search(
            starting_params,
            guard_params,
            args.games,
            args.repeats,
            args.verify_top,
            args.verify_games,
            args.verify_repeats,
            args.promote_margin,
            args.population,
            args.generations,
            args.elite,
            args.mutation_rate,
            args.mutation_scale,
            args.tournament,
            args.timeout,
            args.seed,
            args.wide,
        )

    if args.no_keep_best:
        write_params(original_params)
    else:
        save_best(best_params)
        write_params(best_params)
        build(args.timeout)

    print("\nBEST")
    print(json.dumps(best_params, indent=2, sort_keys=True))
    print(f"best_margin={best_result.margin:.1f}")
    print(f"results={RESULTS_FILE}")
    print(f"best_params={BEST_FILE}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        raise SystemExit(130)
