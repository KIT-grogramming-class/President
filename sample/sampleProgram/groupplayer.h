//
//  groupplayer.h
//  Group1 プレイヤー（v3: MCモード追加）
//
//  改良点:
//   - approve() で既出カードを記録（信念追跡）
//   - 候補手を全列挙
//   - 既出カードから「通る確率」を概算（analytical or MC）
//   - 各手を点数化して最高得点を選択
//

#ifndef _GROUPPLAYER_H_
#define _GROUPPLAYER_H_

#include <vector>
#include "player.h"
#include "gamestatus.h"

class Group1 : public Player {
private:
    CardSet played;       // ゲーム中に出たカードの累積
    bool    useMC;        // true: MCでpass確率を計算 / false: 解析的
    int     mcSamples;    // MCのサンプル数
    int     endgameThreshold;  // 手札がこの枚数以下になったら終盤ルックアヘッド

    // 比較関数（弱い→強い順）
    static bool weakFirst(const Card &a, const Card &b) {
        return a.strength() < b.strength();
    }

    // 補助関数
    Card pileLead(const CardSet &pile) const;
    int  countByRank(int rank) const;
    bool hasJoker() const;
    Card getJoker() const;

    int  unseenCountOfRank(int rank) const;
    bool unseenJoker() const;

    std::vector<CardSet> enumerateMoves(const CardSet &pile) const;

    double analyticalPassProb(const CardSet &move, int leadSize) const;
    double mcPassProb(const CardSet &move, int leadSize, const GameStatus &gstat) const;

    double scoreMove(const CardSet &move, const GameStatus &gstat) const;

    // ----- 終盤ルックアヘッド用 -----
    // 仮想的な手札からの「最少必要手数」を計算（簡易版: distinct rank数）
    int minPlaysFromHand(const CardSet &h) const;

    // 終盤ルックアヘッド: 手札 ≤ endgameThreshold で発動
    // 各候補手 m について、出した後の手札を評価し、最良手を返す
    // 返り値: 最良候補手のインデックス（moves配列内）。見つからなければ -1
    int endgameLookahead(const std::vector<CardSet> &moves,
                         const GameStatus &gstat) const;

public:
    Group1(const char *name = "Group1", bool mc = false, int samples = 20,
           int egThreshold = 4)
        : Player(name), useMC(mc), mcSamples(samples), endgameThreshold(egThreshold) {
        played.clear();
    }
    ~Group1() { }

    void ready();
    bool follow(const GameStatus &gstat, CardSet &cards);
    bool approve(const GameStatus &gstat);
};

#endif
