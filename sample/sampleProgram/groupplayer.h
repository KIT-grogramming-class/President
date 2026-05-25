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
public:
    // 評価関数の重み。インスタンスごとに保持し、self-play で複数の Group1
    // を別重みで対戦させられるようにしている。
    struct Weights {
        // 進化チューニング (scripts/evolve_weights.py, gen 0) で得たベスト値。
        // 公式卓 10000 games × 3 試行で baseline (v2.7 手調整) 30.63% → 31.04%
        // (+0.41pp)。本番環境は env var で渡せないため、ここに固定値として埋める。
        double pass        = 3.5234;
        double weak        = 1.0637;
        double joker       = 10.0464;
        double pair        = 4.7102;
        double danger      = 2.0020;
        double minOpp      = 7.3682;
        double endgame     = 5.8967;
        double endgameDeep = 4.8613;
        double leaderWeak  = 1.3532;
        double multiLead   = 1.2406;
        double oneMore     = 37.8424;

        // 環境変数から重みを構築する。prefix は "" / "A_" / "B_" などで、
        // それぞれ W_PASS, A_W_PASS, B_W_PASS のように読み分ける。
        static Weights fromEnv(const char *prefix = "");
    };

private:
    Weights w;
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

    // ----- MCプレイアウト用 -----
    // 終盤(handSize <= mcEndgame) で各候補手をMC評価して最良を返す
    int mcPlayoutSelect(const std::vector<CardSet> &moves,
                        const GameStatus &gstat) const;

    bool useMCPlayout;        // true: 終盤でMCプレイアウトを使う
    int  mcPlayoutSamples;    // プレイアウト回数
    int  mcPlayoutThreshold;  // 手札枚数 <= これで発動

public:
    Group1(const char *name = "Group1", bool mc = false, int samples = 20,
           int egThreshold = 4, bool mcPlayout = false, int playoutSamples = 15,
           int playoutThreshold = 5)
        : Player(name), useMC(mc), mcSamples(samples), endgameThreshold(egThreshold),
          useMCPlayout(mcPlayout), mcPlayoutSamples(playoutSamples),
          mcPlayoutThreshold(playoutThreshold) {
        played.clear();
        // 既定では無接頭辞の env var (W_PASS 等) を読み込み、現状互換を維持する。
        w = Weights::fromEnv("");
    }
    ~Group1() { }

    // self-play 用: 同一プロセスに重み違いの Group1 を並べたいときに使う。
    void setWeights(const Weights &nw) { w = nw; }

    void ready();
    bool follow(const GameStatus &gstat, CardSet &cards);
    bool approve(const GameStatus &gstat);
};

#endif
