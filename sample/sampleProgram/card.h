//
// card.h - トランプカードの型
//  作者: (あなたの名前); 日付: (完成した日付)
//

#ifndef _CARD_H_
#define _CARD_H_

#include <cstdio>
#include <iostream>

//
// Card - トランプカード型
//
class Card {
 public:
    // クラス定数
    // トランプの組(suit)のコード
    enum {
        SUIT_BLANK = 0,
        SUIT_SPADE = 1,
        SUIT_DIAMOND, // 以下，自動的に 2, 3, ... と割当てられる
        SUIT_HEART,
        SUIT_CLUB,
        SUIT_JOKER,
        RANK_JOKER = 15, // Joker の rank 用のダミー値
    };

 private:
    ////////////////////////////////////////////////////////////
    // メンバ変数
    ////////////////////////////////////////////////////////////

    //  インスタンスメンバ変数．Card 型データ（オブジェクト）がそれぞれ持っているデータ．
    int suitval;    // 組
    int rankval;    // 番号

    ////////////////////////////////////////////////////////////
    // クラス変数
    ////////////////////////////////////////////////////////////

    //  static は，クラスメンバのこと．Card クラスの中で共通の定数として
    //  Card::suitnames で参照できる．
    //  クラス変数（定数）．値の初期化は .cpp で main の外に書いて行う
    //  実際には card.cpp の中で以下の文字列に設定している
    //      スート名の文字列：__, S, D, H, C, Jkr
    //      ランク名の文字列：?, A, 2, ..., Q, K, _, Jkr
    static const char *suitnames[];
    static const char *ranknames[];

 public:
    ////////////////////////////////////////////////////////////
    // メンバ関数
    ////////////////////////////////////////////////////////////

    // デフォルトコンストラクタ（初期値はブランクカード）
    Card(void) : suitval(SUIT_BLANK), rankval(0) { }

    // 組と番号を与えるコンストラクタ
    Card(const int st, const int num) : suitval(st), rankval(num) { }

    // カードを与えるコンストラクタ
    Card(const Card &org) : suitval(org.suitval), rankval(org.rankval) { }

    // 組と番号を設定する
    void set(int st, int num) { suitval = st; rankval = num; }

    // アクセサ
    int rank(void) const { return rankval; }
    int suit(void) const { return suitval; }

    // まともなカードか？
    bool isValid(void) const;

    // 自身と tgt のカードの組，番号が等しいか判定 (true: 同; false: 異)
    // 同じアドレスにあるデータかどうか，ではない．
    bool equals(const Card &tgt) const {
        return (suitval == tgt.suitval) && (rankval == tgt.rankval || suitval == SUIT_JOKER);
    }

    // 文字列から自身の値をセット
    const char *readFrom(const char *);

    // ストリームから自身の値をセット
    std::istream &readFrom(std::istream &);

    // ファイルストリームに出力する
    // 第二引数に false を与えると，カードを伏せて表示
    std::ostream &printOn(std::ostream &, const bool faceup = true) const;

    // ストリーム出力演算子
    friend std::ostream &operator<<(std::ostream &os, const Card &c) {
        return c.printOn(os);
    }

    //////////////////////////////////////////////////////////////////////
    // 「大富豪」で追加したもの
    //////////////////////////////////////////////////////////////////////
    // Joker か？
    bool isJoker() const { return suitval == SUIT_JOKER; }

    // 大富豪的な意味で強いカードか？
    bool isGreaterThan(const Card &c) const;

    // 大富豪的な意味での強さ (0〜15：値が大きいほど強い)
    // Joker:15, 2:13, A:12, K:11, Q:10, J:9, 10:8, ..., 4:2, 3:1
    int strength(void) const;

    // 文字列化
    std::string toString() const;

    // 比較用のクラスメソッド
    // 強さが同じ場合でもスート順で大小を決める
    // a が b より強いか，同じ強さでスート順が先なら true
    static bool cmp(const Card &a, const Card &b) {
      return a.isGreaterThan(b) || (a.rank() == b.rank() && a.suit() < b.suit());
    }

    //////////////////////////////////////////////////////////////////////
    // for backward compatibility
    //////////////////////////////////////////////////////////////////////

    // アクセサ
    int getRank(void) const { return rank(); }
    int getNumber(void) const { return rank(); }
    int getSuit(void) const { return rank(); }

    // equals の別名
    bool equal(const Card &tgt) const { return equals(tgt); }

    // 標準入力から自身に入力する(true: 正常終了; false: 異常終了)
    bool scan(void);

    // 自身の値を標準出力に出力する
    void print(void) const { std::cout << toString(); }
    
};

#endif
