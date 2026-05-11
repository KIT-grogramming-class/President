//
// cardset.h - トランプカードの集合型(C++版)
//  作者: (あなたの名前); 日付: (完成した日付)
//

#ifndef _CARDSET_H_
#define _CARDSET_H_

#include <cstdlib>
#include <random>

#include "card.h"

//
// CardSet - トランプカードの集合型
//
class CardSet {
// 定義・宣言
 public:
    const static int maxnumcard = 53;   // カード総数

 private:
    ////////////////////////////////////////////////////////////////////////
    // メンバ変数
    ////////////////////////////////////////////////////////////////////////
    int count;      // 現在の集合内のカード数
    Card cards[maxnumcard]; // カードのデータ

    ////////////////////////////////////////////////////////////////////////
    // (非公開)メンバ関数 (内部で作業用に使用)
    ////////////////////////////////////////////////////////////////////////
    // target に等しいカードの cards 中の格納位置を返す(-1: ない)
    int find(const Card &target) const;
    // rank が num に等しいカードの格納位置を返す(-1: ない)
    int find(int num) const;

 public:
    ////////////////////////////////////////////////////////////////////////
    // (公開)メンバ関数
    ////////////////////////////////////////////////////////////////////////
    // デフォルトコンストラクタ(初期値空集合)
    CardSet(void) { clear(); }

    // コピーコンストラクタ
    CardSet(const CardSet &cset);

    // 自身を空集合にする
    void clear(void) { count = 0; }
    void makeEmpty(void) { clear(); }

    // 自身が空集合か否かの判定 (true: 空; false: 非空)
    bool isEmpty(void) const { return count == 0; }

    // カード枚数のアクセサ
    int size(void) const { return count; }

    // 添字でアクセスする
    const Card &at(int i) const { return cards[i]; }

    // 自身に１デッキすべての(maxnumcard 枚の)カードを入れる
    void setupDeck(void);

    // target に等しいカードがあるかどうかを返す (true: ある; false: ない)
    bool includes(const Card &target) const { return find(target) != -1; }

    // rank が num に等しいカードがあるかどうかを返す (true: ある; false: ない)
    bool includes(int num) const { return find(num) != -1; }

    // newcard を追加する (false: 失敗; true: 成功)
    // 既にあるカードを追加しようとしたか，または，満杯状態の場合は失敗
    bool insert(const Card &newcard);

    // target に等しいカードを取り除く (false: 失敗; true: 成功)
    // 該当するカードが無い場合は失敗
    bool remove(const Card &target);

    // rank が num に等しいカードを1枚除く (false: 失敗; true: 成功)
    // 該当するカードが複数枚あっても，除くのは1枚だけ
    bool remove(int num);

    // 自身から targetpos 枚目 (先頭は 0 枚目) のカードを除き，
    // そのカードを引数 card で返す
    // targetpos が 負数 またはカード数以上のときはランダムに選ぶ
    // 第2引数を省略した場合は -1 が指定されたものとみなす(デフォルト引数)
    // (false: 失敗; true: 成功)
    bool draw(Card &card, int targetpos = -1);

    // シャッフルする
    void shuffle(void);

    // rank と suit で昇順にソートする
    void sort(void);

    // ストリーム出力
    std::ostream &printOn(std::ostream &out, const bool faceup = true) const;

    // ストリーム出力演算子
    friend std::ostream &operator<<(std::ostream &out, const CardSet &cset) {
        return cset.printOn(out);
    }

    //////////////////////////////////////////////////////////////////////
    // 「大富豪」で追加したもの
    //////////////////////////////////////////////////////////////////////
    static bool sortedPrint;    // 印刷時にソートして表示するかどうかのフラグ

    // 自身に newcards のカード集合の各カードを入れる (false: 失敗; true: 成功)
    bool insert(const CardSet &newcards);

    // カード集合の要素を配列のようにアクセスするための演算子関数
    Card &operator[](int);
    const Card &operator[](int) const;

    // 自身の状態を標準出力に出力する
    void print(void) const;

    // 文字列化
    std::string toString() const;
    
    // 集合操作関数 等 (added by Teigo Nakamura)
    bool member(Card c) const;           // c が集合の要素かどうか判定(要素のとき true)
    bool subsetof(const CardSet &cs);    // 自身が cs の部分集合かどうか判定(部分集合のとき true)
    bool equal(const CardSet &cs);       // 自身と cs が集合として等しいかどうか判定(等しい集合のとき true)
    bool remove(const CardSet &cs);      // 自身から cs の要素を削除(true: 失敗，false: 成功)
    CardSet intersection(const CardSet &cs) const; // 自身と cs の共通集合を返す

    void sort(bool (*cmp_func)(const Card &, const Card &)); // ソート

    //////////////////////////////////////////////////////////////////////
    // for backward compatibility
    //////////////////////////////////////////////////////////////////////
    
    bool pickup(Card &card, int targetpos = -1) { return draw(card, targetpos); }

};

#endif
