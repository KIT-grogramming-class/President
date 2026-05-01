//
//  simpleplayer.cpp
//  PlayingCard
//
//  Created by 下薗 真一 on 09/04/12.
//  Modified by Kazutaka Shimada on 09/04/21.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  Modified by Teigo Nakamura
//

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "card.h"
#include "cardset.h"
#include "player.h"
#include "gamestatus.h"
#include "simpleplayer.h"

namespace {

const int kWinScore = 100000;
const int kLeadCardCountWeight = 760;
const int kLeadGroupBonus = 520;
const int kFollowBeatBonus = 2500;
const int kFollowCardCountWeight = 190;
const int kFollowStrengthPenalty = 90;
const int kWeakCardBonus = 75;
const int kShapeWeight = 1;
const int kBreakPairPenalty = 720;
const int kFinishTurnPenalty = 1450;
const int kEndgameCardBonus = 860;
const int kNearEndgameCardBonus = 360;
const int kDangerTakeLeadBonus = 650;
const int kPassBaseScore = 40;
const int kPassConserveBonus = 0;
const int kPassEndgamePenalty = 1450;
const int kPassNearEndgamePenalty = 520;
const int kPassDangerPenalty = 1250;

struct CandidateMove {
    CardSet cards;
    bool pass;

    CandidateMove() : pass(false) { }
};

struct FinishPlan {
    int turns;
    int score;

    FinishPlan() : turns(0), score(0) { }
    FinishPlan(int t, int s) : turns(t), score(s) { }
};

bool weakCardOrder(const Card &a, const Card &b) {
    if (a.strength() != b.strength()) {
        return a.strength() < b.strength();
    }
    return a.suit() < b.suit();
}

int strengthOfRank(int rank) {
    return (rank + 10) % 13 + 1;
}

std::vector<Card> toSortedCards(const CardSet &cards) {
    std::vector<Card> ret;
    for (int i = 0; i < cards.size(); i++) {
        ret.push_back(cards.at(i));
    }
    std::sort(ret.begin(), ret.end(), weakCardOrder);
    return ret;
}

std::vector<std::vector<Card> > cardsByRank(const CardSet &cards) {
    std::vector<std::vector<Card> > ranks(14);
    for (int i = 0; i < cards.size(); i++) {
        const Card &card = cards.at(i);
        if (!card.isJoker() && 1 <= card.rank() && card.rank() <= 13) {
            ranks[card.rank()].push_back(card);
        }
    }
    for (int rank = 1; rank <= 13; rank++) {
        std::sort(ranks[rank].begin(), ranks[rank].end(), weakCardOrder);
    }
    return ranks;
}

std::vector<Card> jokersIn(const CardSet &cards) {
    std::vector<Card> ret;
    for (int i = 0; i < cards.size(); i++) {
        if (cards.at(i).isJoker()) {
            ret.push_back(cards.at(i));
        }
    }
    return ret;
}

CardSet makeSet(const std::vector<Card> &cards) {
    CardSet ret;
    for (size_t i = 0; i < cards.size(); i++) {
        ret.insert(cards[i]);
    }
    return ret;
}

void addMove(std::vector<CandidateMove> &moves, const std::vector<Card> &cards) {
    CandidateMove move;
    move.cards = makeSet(cards);
    move.pass = false;
    moves.push_back(move);
}

int effectiveStrength(const CardSet &cards) {
    if (cards.isEmpty()) {
        return 0;
    }
    for (int i = 0; i < cards.size(); i++) {
        if (!cards.at(i).isJoker()) {
            return cards.at(i).strength();
        }
    }
    return cards.at(0).strength();
}

int minimumOpponentCards(const GameStatus &gstat, int myId) {
    int myIndex = gstat.turnIndex;
    for (int i = 0; i < gstat.numPlayers; i++) {
        if (gstat.playerID[i] == myId) {
            myIndex = i;
            break;
        }
    }

    int minimum = 99;
    for (int i = 0; i < gstat.numPlayers; i++) {
        if (i != myIndex) {
            minimum = std::min(minimum, gstat.numCards[i]);
        }
    }
    return minimum == 99 ? 0 : minimum;
}

int premiumPenalty(const Card &card) {
    if (card.isJoker()) {
        return 1550;
    }
    switch (card.strength()) {
    case 13:
        return 760;   // 2
    case 12:
        return 520;   // A
    case 11:
        return 280;   // K
    default:
        return card.strength() >= 10 ? 120 : 0;
    }
}

int premiumPenalty(const CardSet &cards) {
    int penalty = 0;
    for (int i = 0; i < cards.size(); i++) {
        penalty += premiumPenalty(cards.at(i));
    }
    return penalty;
}

int weakCardBonus(const CardSet &cards) {
    int bonus = 0;
    for (int i = 0; i < cards.size(); i++) {
        const Card &card = cards.at(i);
        if (!card.isJoker()) {
            bonus += std::max(0, 8 - card.strength()) * kWeakCardBonus;
        }
    }
    return bonus;
}

bool containsJoker(const CardSet &cards) {
    for (int i = 0; i < cards.size(); i++) {
        if (cards.at(i).isJoker()) {
            return true;
        }
    }
    return false;
}

bool canPlayInOneMove(const CardSet &cards) {
    if (cards.isEmpty()) {
        return true;
    }

    int rank = -1;
    for (int i = 0; i < cards.size(); i++) {
        const Card &card = cards.at(i);
        if (card.isJoker()) {
            continue;
        }
        if (rank == -1) {
            rank = card.rank();
        } else if (rank != card.rank()) {
            return false;
        }
    }
    return true;
}

bool hardToBeat(const CardSet &cards);

int oneMoveFinishBonus(const CardSet &cards) {
    if (cards.isEmpty()) {
        return 0;
    }

    const int strength = effectiveStrength(cards);
    if (cards.size() == 1 && !containsJoker(cards)) {
        if (strength <= 6) {
            return 1450 + strength * 120;
        }
        if (strength >= 11) {
            return 5200;
        }
        return 3300;
    }

    if (hardToBeat(cards)) {
        return 5700;
    }
    return 4100 + cards.size() * 260;
}

int naturalGroupCount(const CardSet &cards) {
    std::vector<std::vector<Card> > ranks = cardsByRank(cards);
    int count = 0;
    for (int rank = 1; rank <= 13; rank++) {
        if (!ranks[rank].empty()) {
            count++;
        }
    }
    if (count == 0 && containsJoker(cards)) {
        count = 1;
    }
    return count;
}

int remainingShapeScore(const CardSet &cards);

FinishPlan analyzeFinishPlan(const CardSet &cards) {
    if (cards.isEmpty()) {
        return FinishPlan(0, 0);
    }

    std::vector<std::vector<Card> > ranks = cardsByRank(cards);
    const bool hasJoker = containsJoker(cards);
    int turns = 0;
    int score = 0;
    int weakestSingleStrength = 99;

    for (int rank = 1; rank <= 13; rank++) {
        const int count = static_cast<int>(ranks[rank].size());
        if (count == 0) {
            continue;
        }

        turns++;
        const int strength = strengthOfRank(rank);
        score += count * count * 300 - kFinishTurnPenalty;

        if (count >= 2) {
            score += 520 + count * 180;
        } else if (strength <= 6) {
            score -= (7 - strength) * 260;
            weakestSingleStrength = std::min(weakestSingleStrength, strength);
        } else if (strength >= 11) {
            score += (strength - 10) * 280;
        } else {
            score -= 90;
        }
    }

    if (hasJoker) {
        if (turns == 0) {
            turns = 1;
            score += 1200 - kFinishTurnPenalty;
        } else if (weakestSingleStrength != 99) {
            score += 1200 + (7 - weakestSingleStrength) * 180;
        } else {
            score += 650;
        }
    }

    if (turns <= 2) {
        score += 1400;
    } else if (turns == 3) {
        score += 500;
    }

    return FinishPlan(turns, score);
}

int remainingShapeScore(const CardSet &cards) {
    if (cards.isEmpty()) {
        return 0;
    }

    std::vector<std::vector<Card> > ranks = cardsByRank(cards);
    const bool hasJoker = containsJoker(cards);
    int score = 0;
    int weakSingles = 0;
    int strongSingles = 0;

    for (int rank = 1; rank <= 13; rank++) {
        const int count = static_cast<int>(ranks[rank].size());
        const int strength = strengthOfRank(rank);
        if (count >= 2) {
            score += count * count * 150;
            if (count >= 4) {
                score += 360;
            }
        } else if (count == 1) {
            if (strength <= 6) {
                weakSingles++;
                score -= (7 - strength) * 115;
            } else if (strength >= 11) {
                strongSingles++;
                score += (strength - 10) * 140;
            } else {
                score -= 45;
            }
        }
    }

    if (hasJoker) {
        score += 820;
        if (weakSingles > 0) {
            score += 260;  // Joker は弱い単体をペア化できる保険になる。
        }
    }

    if (canPlayInOneMove(cards)) {
        score += oneMoveFinishBonus(cards);
    } else if (naturalGroupCount(cards) <= 2) {
        score += 1750;
    }

    score -= weakSingles * 130;
    score += strongSingles * 55;
    return score;
}

int groupBreakPenalty(const CardSet &before, const CardSet &played) {
    std::vector<std::vector<Card> > beforeRanks = cardsByRank(before);
    std::vector<std::vector<Card> > playedRanks = cardsByRank(played);
    int penalty = 0;

    for (int rank = 1; rank <= 13; rank++) {
        const int beforeCount = static_cast<int>(beforeRanks[rank].size());
        const int usedCount = static_cast<int>(playedRanks[rank].size());
        const int restCount = beforeCount - usedCount;
        if (usedCount == 0 || beforeCount < 2) {
            continue;
        }

        if (restCount == 1) {
            penalty += kBreakPairPenalty;
        } else if (usedCount == 1 && beforeCount >= 3) {
            penalty += 240;
        }
    }

    return penalty;
}

bool hardToBeat(const CardSet &cards) {
    const int strength = effectiveStrength(cards);
    if (strength >= 15) {
        return true;
    }
    if (strength >= 13 && cards.size() >= 2) {
        return true;
    }
    return strength >= 14;
}

int adjustedPremiumPenalty(const CardSet &cards, bool win, bool endgame, bool nearEndgame, bool opponentDanger) {
    if (win) {
        return 0;
    }

    int penalty = premiumPenalty(cards);
    if (endgame) {
        penalty = penalty * 25 / 100;
    } else if (nearEndgame) {
        penalty = penalty * 60 / 100;
    } else if (opponentDanger) {
        penalty = penalty * 55 / 100;
    }
    return penalty;
}

int scorePass(const GameStatus &gstat, int myId, bool endgame, bool nearEndgame) {
    const int opponentMin = minimumOpponentCards(gstat, myId);
    const bool opponentDanger = opponentMin <= 2;
    int score = kPassBaseScore;

    if (!endgame && !opponentDanger) {
        score += kPassConserveBonus;
    }
    if (effectiveStrength(gstat.pile) >= 12) {
        score += 260;  // 高い場札には無理に付き合わない。
    }
    if (endgame) {
        score -= kPassEndgamePenalty;
    } else if (nearEndgame) {
        score -= kPassNearEndgamePenalty;
    }
    if (opponentDanger) {
        score -= kPassDangerPenalty;
    }
    return score;
}

int scoreMove(const CandidateMove &move, const CardSet &hand, const GameStatus &gstat, int myId) {
    if (move.pass) {
        return scorePass(gstat, myId, hand.size() <= 4, hand.size() <= 8);
    }

    CardSet remaining(hand);
    remaining.remove(move.cards);

    const bool leadEmpty = gstat.pile.isEmpty();
    const bool win = remaining.isEmpty();
    const bool endgame = hand.size() <= 4;
    const bool nearEndgame = hand.size() <= 8;
    const bool opponentDanger = minimumOpponentCards(gstat, myId) <= 2;
    const int playedSize = move.cards.size();
    int score = 0;

    if (win) {
        score += kWinScore + playedSize * 2400;
    }

    const FinishPlan finishPlan = analyzeFinishPlan(remaining);
    score += remainingShapeScore(remaining) * kShapeWeight;
    if (finishPlan.turns == 1) {
        score += 3600;
    } else if (finishPlan.turns == 2) {
        score += 1600;
    } else if (finishPlan.turns == 3) {
        score += 500;
    }
    score += weakCardBonus(move.cards);
    score -= adjustedPremiumPenalty(move.cards, win, endgame, nearEndgame, opponentDanger);
    score -= groupBreakPenalty(hand, move.cards);

    if (leadEmpty) {
        // リード時は弱い複数枚を処理し、強い単体を残す。
        score += playedSize * kLeadCardCountWeight;
        if (playedSize >= 2) {
            score += (playedSize - 1) * kLeadGroupBonus;
        }
        score -= effectiveStrength(move.cards) * 28;

        if (opponentDanger) {
            // 相手が残り少ない時は、弱い単体で相手を上がらせない。
            score += effectiveStrength(move.cards) * 95;
            if (playedSize >= 2) {
                score += 980;
            }
            if (hardToBeat(move.cards)) {
                score += 880;
            }
        }
    } else {
        // 返す時は「勝てる中で一番弱い手」を基本にする。
        score += kFollowBeatBonus;
        score += playedSize * kFollowCardCountWeight;
        score -= effectiveStrength(move.cards) * kFollowStrengthPenalty;

        if (hardToBeat(move.cards)) {
            score += (endgame || opponentDanger) ? 720 : -180;
        }
        if (opponentDanger) {
            score += kDangerTakeLeadBonus;
        }
    }

    if (endgame) {
        score += playedSize * kEndgameCardBonus;
        if (finishPlan.turns <= 1) {
            score += 2600;
        } else if (finishPlan.turns <= 2) {
            score += 1100;
        }
    } else if (nearEndgame) {
        score += playedSize * kNearEndgameCardBonus;
        if (finishPlan.turns <= 1) {
            score += 1300;
        } else if (finishPlan.turns <= 2) {
            score += 620;
        }
    }

    return score;
}

std::vector<CandidateMove> enumerateLeadMoves(const CardSet &hand) {
    std::vector<CandidateMove> moves;
    std::vector<std::vector<Card> > ranks = cardsByRank(hand);
    std::vector<Card> jokers = jokersIn(hand);

    for (int rank = 1; rank <= 13; rank++) {
        const int count = static_cast<int>(ranks[rank].size());
        for (int size = 1; size <= count; size++) {
            std::vector<Card> cards(ranks[rank].begin(), ranks[rank].begin() + size);
            addMove(moves, cards);
        }

        if (!jokers.empty()) {
            for (int natural = 1; natural <= count && natural + 1 <= 4; natural++) {
                std::vector<Card> cards(ranks[rank].begin(), ranks[rank].begin() + natural);
                cards.push_back(jokers[0]);
                addMove(moves, cards);
            }
        }
    }

    if (!jokers.empty()) {
        std::vector<Card> cards;
        cards.push_back(jokers[0]);
        addMove(moves, cards);
    }

    return moves;
}

std::vector<CandidateMove> enumerateFollowMoves(const CardSet &hand, const CardSet &pile) {
    std::vector<CandidateMove> moves;
    std::vector<std::vector<Card> > ranks = cardsByRank(hand);
    std::vector<Card> jokers = jokersIn(hand);
    const int requiredSize = pile.size();
    const int pileStrength = effectiveStrength(pile);

    if (requiredSize <= 0) {
        return moves;
    }

    if (requiredSize == 1) {
        std::vector<Card> cards = toSortedCards(hand);
        for (size_t i = 0; i < cards.size(); i++) {
            if (cards[i].strength() > pileStrength) {
                std::vector<Card> one;
                one.push_back(cards[i]);
                addMove(moves, one);
            }
        }
        return moves;
    }

    for (int rank = 1; rank <= 13; rank++) {
        const int count = static_cast<int>(ranks[rank].size());
        const int rankStrength = strengthOfRank(rank);
        if (rankStrength <= pileStrength) {
            continue;
        }

        if (count >= requiredSize) {
            std::vector<Card> cards(ranks[rank].begin(), ranks[rank].begin() + requiredSize);
            addMove(moves, cards);
        }

        if (!jokers.empty() && count >= requiredSize - 1) {
            std::vector<Card> cards(ranks[rank].begin(), ranks[rank].begin() + requiredSize - 1);
            cards.push_back(jokers[0]);
            addMove(moves, cards);
        }
    }

    return moves;
}

std::vector<CandidateMove> enumerateLegalMoves(const CardSet &hand, const GameStatus &gstat) {
    std::vector<CandidateMove> moves = gstat.pile.isEmpty()
        ? enumerateLeadMoves(hand)
        : enumerateFollowMoves(hand, gstat.pile);

    if (!gstat.pile.isEmpty()) {
        CandidateMove passMove;
        passMove.pass = true;
        moves.push_back(passMove);
    }
    return moves;
}

CandidateMove chooseBestMove(const CardSet &hand, const GameStatus &gstat, int myId) {
    std::vector<CandidateMove> moves = enumerateLegalMoves(hand, gstat);
    CandidateMove best;
    int bestScore = std::numeric_limits<int>::min();

    for (size_t i = 0; i < moves.size(); i++) {
        const int score = scoreMove(moves[i], hand, gstat, myId);
        if (score > bestScore) {
            bestScore = score;
            best = moves[i];
        }
    }

    return best;
}

} // namespace

void SimplePlayer::ready() {
    memory.clear();
    // trump.clear();
    hand.sort(myCardCmp);
}

// 合法手を列挙し、評価関数で一番よい手を選ぶヒューリスティック戦略。
bool SimplePlayer::follow(const GameStatus &gstat, CardSet &cs) {
    cs.clear();
    hand.sort(myCardCmp);

    CandidateMove best = chooseBestMove(hand, gstat, getId());
    if (best.pass || best.cards.isEmpty()) {
        return true;
    }

    cs.insert(best.cards);
    hand.remove(cs);
    return true;
}

bool SimplePlayer::approve(const GameStatus &gstat) {
    (void)gstat;
    return true;
}
