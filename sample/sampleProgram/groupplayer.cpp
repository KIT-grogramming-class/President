//
//  groupplayer.cpp
//  Group1 プレイヤー（v3: MCモード追加）
//

#include <algorithm>
#include <random>
#include <vector>
#include "card.h"
#include "cardset.h"
#include "gamestatus.h"
#include "groupplayer.h"

namespace {
    // 全プレイヤー共通の乱数（再現性のため固定シード）
    static std::mt19937 g_rng(0xC0FFEE);
}

// ----- 補助関数 -----

Card Group1::pileLead(const CardSet &pile) const {
    for (int i = 0; i < pile.size(); i++) {
        if (!pile.at(i).isJoker()) return pile.at(i);
    }
    return pile.size() > 0 ? pile.at(0) : Card();
}

int Group1::countByRank(int rank) const {
    int n = 0;
    for (int i = 0; i < hand.size(); i++) {
        if (!hand.at(i).isJoker() && hand.at(i).rank() == rank) n++;
    }
    return n;
}

bool Group1::hasJoker() const {
    for (int i = 0; i < hand.size(); i++) {
        if (hand.at(i).isJoker()) return true;
    }
    return false;
}

Card Group1::getJoker() const {
    for (int i = 0; i < hand.size(); i++) {
        if (hand.at(i).isJoker()) return hand.at(i);
    }
    return Card();
}

int Group1::unseenCountOfRank(int rank) const {
    int total = 4;
    for (int i = 0; i < played.size(); i++) {
        if (!played.at(i).isJoker() && played.at(i).rank() == rank) total--;
    }
    for (int i = 0; i < hand.size(); i++) {
        if (!hand.at(i).isJoker() && hand.at(i).rank() == rank) total--;
    }
    return std::max(0, total);
}

bool Group1::unseenJoker() const {
    for (int i = 0; i < played.size(); i++) {
        if (played.at(i).isJoker()) return false;
    }
    for (int i = 0; i < hand.size(); i++) {
        if (hand.at(i).isJoker()) return false;
    }
    return true;
}

// ----- 候補手の列挙 -----

std::vector<CardSet> Group1::enumerateMoves(const CardSet &pile) const {
    std::vector<CardSet> moves;
    int leadSize = pile.size();
    bool isLeader = (leadSize == 0);

    bool jokerHere = hasJoker();
    Card lead = isLeader ? Card() : pileLead(pile);

    int sizeMin = isLeader ? 1 : leadSize;
    int sizeMax = isLeader ? 4 : leadSize;

    for (int sz = sizeMin; sz <= sizeMax; sz++) {
        for (int r = 1; r <= 13; r++) {
            int cnt = countByRank(r);
            int useFromHand = std::min(cnt, sz);
            int needJoker = sz - useFromHand;

            if (needJoker > 1) continue;
            if (needJoker == 1 && !jokerHere) continue;
            if (useFromHand == 0) continue;

            if (!isLeader) {
                Card test(Card::SUIT_SPADE, r);
                if (!test.isGreaterThan(lead)) continue;
            }

            CardSet group;
            for (int i = 0; i < hand.size() && (int)group.size() < useFromHand; i++) {
                if (!hand.at(i).isJoker() && hand.at(i).rank() == r) {
                    group.insert(hand.at(i));
                }
            }
            if (needJoker == 1) group.insert(getJoker());

            if ((int)group.size() == sz) moves.push_back(group);
        }
    }

    if (jokerHere) {
        if (isLeader || (leadSize == 1 && !lead.isJoker())) {
            CardSet g;
            g.insert(getJoker());
            moves.push_back(g);
        }
    }

    return moves;
}

// ----- 解析的pass確率 -----

double Group1::analyticalPassProb(const CardSet &move, int leadSize) const {
    int moveRank = -1;
    bool useJoker = false;
    for (int i = 0; i < move.size(); i++) {
        if (move.at(i).isJoker()) useJoker = true;
        else if (moveRank == -1) moveRank = move.at(i).rank();
    }
    if (moveRank == -1) return 1.0;

    Card moveTest(Card::SUIT_SPADE, moveRank);

    int strongerRanksCanBeat = 0;
    int strongerRanksWithJoker = 0;
    int strongerRanksTotal = 0;

    for (int r = 1; r <= 13; r++) {
        Card test(Card::SUIT_SPADE, r);
        if (!test.isGreaterThan(moveTest)) continue;
        strongerRanksTotal++;

        int u = unseenCountOfRank(r);
        if (u >= leadSize) strongerRanksCanBeat++;
        else if (u >= leadSize - 1 && unseenJoker() && !useJoker) strongerRanksWithJoker++;
    }

    bool jokerThreat = (leadSize == 1 && !useJoker && unseenJoker());

    if (strongerRanksTotal == 0 && !jokerThreat) return 1.0;

    double base = 1.0;
    if (strongerRanksTotal > 0) {
        double threatRatio = (double)(strongerRanksCanBeat + 0.5 * strongerRanksWithJoker)
                              / strongerRanksTotal;
        base = 1.0 - threatRatio;
    }
    if (jokerThreat) base *= 0.85;
    return std::max(0.0, std::min(1.0, base));
}

// ----- MCベースpass確率 -----
// 相手の手札を実際にサンプリングして、誰かが返せるかを K 回試行

double Group1::mcPassProb(const CardSet &move, int leadSize,
                          const GameStatus &gstat) const {
    int moveRank = -1;
    bool useJoker = false;
    for (int i = 0; i < move.size(); i++) {
        if (move.at(i).isJoker()) useJoker = true;
        else if (moveRank == -1) moveRank = move.at(i).rank();
    }
    if (moveRank == -1) return 1.0;
    Card moveTest(Card::SUIT_SPADE, moveRank);

    // 場外（自分手札・既出を除く）プールを構築
    std::vector<Card> pool;
    pool.reserve(53);
    for (int s = Card::SUIT_SPADE; s <= Card::SUIT_CLUB; s++) {
        for (int r = 1; r <= 13; r++) {
            Card c(s, r);
            if (!hand.includes(c) && !played.includes(c)) pool.push_back(c);
        }
    }
    Card joker(Card::SUIT_JOKER, Card::RANK_JOKER);
    if (!hand.includes(joker) && !played.includes(joker)) pool.push_back(joker);

    if (pool.empty()) return 1.0;

    // 相手のターン後位置（自分以外）の手札枚数を取得
    int oppCount = 0;
    int oppSizes[8];
    for (int i = 0; i < gstat.numPlayers; i++) {
        if (i == gstat.turnIndex) continue;
        oppSizes[oppCount++] = gstat.numCards[i];
    }
    if (oppCount == 0) return 1.0;

    int K = mcSamples;
    int passes = 0;

    for (int k = 0; k < K; k++) {
        std::shuffle(pool.begin(), pool.end(), g_rng);

        // 各相手にランダム配布
        bool anyBeat = false;
        int idx = 0;
        for (int o = 0; o < oppCount && !anyBeat; o++) {
            int n = oppSizes[o];
            if (n == 0) continue;

            // この相手の手札（n枚）から、leadSize枚で moveTest より強い組が組めるか
            int rankCount[16] = {0};
            bool oppHasJoker = false;
            for (int j = 0; j < n && idx + j < (int)pool.size(); j++) {
                const Card &c = pool[idx + j];
                if (c.isJoker()) oppHasJoker = true;
                else rankCount[c.rank()]++;
            }
            idx += n;

            // ジョーカー単独で返せるか（leadSize=1 のとき）
            if (leadSize == 1 && oppHasJoker && !useJoker) {
                anyBeat = true;
                break;
            }

            // 強いランクで leadSize 枚揃うか
            for (int r = 1; r <= 13; r++) {
                Card test(Card::SUIT_SPADE, r);
                if (!test.isGreaterThan(moveTest)) continue;
                int needed = leadSize - rankCount[r];
                if (needed <= 0) { anyBeat = true; break; }
                if (needed == 1 && oppHasJoker) { anyBeat = true; break; }
            }
        }

        if (!anyBeat) passes++;
    }

    return (double)passes / K;
}

// ----- 手の点数評価 -----

double Group1::scoreMove(const CardSet &move, const GameStatus &gstat) const {
    int leadSize = gstat.pile.size();
    int effSize = (leadSize == 0) ? (int)move.size() : leadSize;

    double pass = useMC
        ? mcPassProb(move, effSize, gstat)
        : analyticalPassProb(move, effSize);

    int strength = 0;
    bool useJoker = false;
    int moveRank = -1;
    for (int i = 0; i < move.size(); i++) {
        if (move.at(i).isJoker()) {
            useJoker = true;
            strength = std::max(strength, 15);
        } else {
            int s = move.at(i).strength();
            strength = std::max(strength, s);
            if (moveRank == -1) moveRank = move.at(i).rank();
        }
    }

    int pairBroken = 0;
    if (moveRank != -1) {
        int cntInHand = countByRank(moveRank);
        int usedFromRank = (int)move.size() - (useJoker ? 1 : 0);
        if (cntInHand > usedFromRank && cntInHand >= 2) {
            pairBroken = cntInHand - usedFromRank;
        }
    }

    int minOpp = 99;
    int dangerCount = 0;  // 手札3枚以下の相手数
    for (int i = 0; i < gstat.numPlayers; i++) {
        if (i == gstat.turnIndex) continue;
        if (gstat.numCards[i] < minOpp) minOpp = gstat.numCards[i];
        if (gstat.numCards[i] <= 3) dangerCount++;
    }

    // 手札が少ないほど pass 確率の重みを上げる（終盤は通すことが重要）
    int hsize = hand.size();
    double passWeight = 3.0;
    if (hsize <= 5) passWeight = 6.0;
    if (hsize <= 3) passWeight = 9.0;
    if (hsize <= 1) passWeight = 12.0;

    // ジョーカー温存ペナルティも終盤は弱める（出し惜しみせず使う）
    double jokerPenalty = 8.0;
    if (hsize <= 4) jokerPenalty = 4.0;
    if (hsize <= 2) jokerPenalty = 0.0;

    double score = 0.0;
    score += passWeight * pass;
    score += (15 - strength) * 1.2;
    score -= useJoker ? jokerPenalty : 0.0;
    // ペア温存（手札が多いうちは強めに、終盤は弱めに）
    double pairWeight = 4.0;
    if (hsize <= 4) pairWeight = 2.0;
    if (hsize <= 2) pairWeight = 0.5;
    score -= pairBroken * pairWeight;

    // ライバル接近時の止め強化（人数で重みを倍化）
    if (dangerCount > 0) score += dangerCount * 3.0 * pass;
    if (minOpp <= 1) score += 4.0;  // 誰かリーチなら出すこと自体に価値

    // 終盤は積極的に出す（パスのベースライン 0 を超えやすくする）
    if (hsize <= 5) score += 3.0;
    if (hsize <= 3) score += 3.0;  // 追加ボーナス

    if (gstat.pile.size() == 0) {
        score += (15 - strength) * 0.5;
        // 複数枚リード（ペア・トリプルなど）は相手の選択肢を狭めるのでボーナス
        if ((int)move.size() >= 2) score += 1.5;
        if ((int)move.size() >= 3) score += 1.5;
    }

    // この手を出すと上がる／あと1手で上がれる、を判定
    int remTotal = (int)hand.size() - (int)move.size();
    if (remTotal == 0) {
        score += 100.0;  // 上がりの手
    } else {
        // 出した後の手札の分布を計算
        int remRankCount[14] = {0};
        bool remHasJoker = false;
        for (int i = 0; i < hand.size(); i++) {
            if (hand.at(i).isJoker()) remHasJoker = true;
            else if (hand.at(i).rank() >= 1 && hand.at(i).rank() <= 13)
                remRankCount[hand.at(i).rank()]++;
        }
        for (int i = 0; i < move.size(); i++) {
            if (move.at(i).isJoker()) remHasJoker = false;
            else if (move.at(i).rank() >= 1 && move.at(i).rank() <= 13)
                remRankCount[move.at(i).rank()]--;
        }
        int remDistinct = 0;
        for (int r = 1; r <= 13; r++) {
            if (remRankCount[r] > 0) remDistinct++;
        }
        // 残り手札が「同じランクのみ」または「ジョーカーのみ」なら1手で上がれる
        bool oneMore = (remDistinct == 0 && remHasJoker) || (remDistinct == 1);
        if (oneMore) score += 25.0;
    }

    return score;
}

// ----- 終盤ルックアヘッド -----

int Group1::minPlaysFromHand(const CardSet &h) const {
    if (h.size() == 0) return 0;
    int rankCount[14] = {0};
    bool hasJokerLocal = false;
    for (int i = 0; i < h.size(); i++) {
        if (h.at(i).isJoker()) hasJokerLocal = true;
        else if (h.at(i).rank() >= 1 && h.at(i).rank() <= 13)
            rankCount[h.at(i).rank()]++;
    }
    int distinct = 0;
    for (int r = 1; r <= 13; r++) if (rankCount[r] > 0) distinct++;
    // ジョーカーは distinct >= 1 ならいずれかの組に吸収可能
    if (distinct == 0 && hasJokerLocal) return 1;
    return distinct;
}

int Group1::endgameLookahead(const std::vector<CardSet> &moves,
                             const GameStatus &gstat) const {
    if (moves.empty()) return -1;

    int bestIdx = -1;
    double bestVal = -1e18;

    int currentMin = minPlaysFromHand(hand);

    for (size_t i = 0; i < moves.size(); i++) {
        const CardSet &m = moves[i];

        // 出した後の手札を作る
        CardSet remaining(hand);
        remaining.remove(m);

        int afterMin = minPlaysFromHand(remaining);

        // 評価値:
        //   1. 最少手数の減少量 (普通は1、無駄打ちなら0)
        //   2. 残り手札のサイズ (少ないほど良い)
        //   3. 既存スコア（通る確率や強さ）
        double v = 0;
        v += (currentMin - afterMin) * 10.0;        // 手数を減らす効果
        v += (hand.size() - remaining.size()) * 2.0; // カード数を減らす効果（普通 = move.size()）
        v += scoreMove(m, gstat) * 0.5;             // 既存評価との折衷
        // 上がりは確実な最大値
        if (remaining.size() == 0) v += 1000.0;
        // 1手で上がれる残りなら高ボーナス
        else if (afterMin <= 1) v += 50.0;

        if (v > bestVal) {
            bestVal = v;
            bestIdx = (int)i;
        }
    }
    return bestIdx;
}

// ----- インターフェース -----

void Group1::ready() {
    played.clear();
    hand.sort(weakFirst);
}

bool Group1::approve(const GameStatus &gstat) {
    for (int i = 0; i < gstat.pile.size(); i++) {
        Card c = gstat.pile.at(i);
        if (!played.includes(c)) {
            played.insert(c);
        }
    }
    return true;
}

bool Group1::follow(const GameStatus &gstat, CardSet &cards) {
    CardSet pile(gstat.pile);
    bool isLeader = (pile.size() == 0);

    hand.sort(weakFirst);
    cards.clear();

    std::vector<CardSet> moves = enumerateMoves(pile);

    if (moves.empty()) return true;

    int bestIdx = -1;

    // 終盤ルックアヘッド（手札 <= 閾値）
    if ((int)hand.size() <= endgameThreshold) {
        bestIdx = endgameLookahead(moves, gstat);
    }

    if (bestIdx == -1) {
        // 通常評価: 各手をスコアリングして最高得点を選ぶ
        // パスの基準点（通常 0、終盤はパスのコストが高い）
        double passBaseline = 0.0;
        if (!isLeader) {
            if ((int)hand.size() <= 3) passBaseline = -10.0;
            else if ((int)hand.size() <= 5) passBaseline = -3.0;
        }

        double bestScore = isLeader ? -1e18 : passBaseline;

        for (size_t i = 0; i < moves.size(); i++) {
            double s = scoreMove(moves[i], gstat);
            if (s > bestScore) {
                bestScore = s;
                bestIdx = (int)i;
            }
        }

        if (bestIdx == -1) return true;
    }

    cards = moves[bestIdx];
    hand.remove(cards);
    return true;
}
