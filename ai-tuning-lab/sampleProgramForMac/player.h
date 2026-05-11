//
//  Player.h
//  PlayingCard
//
//  Created by 下薗 真一 on 09/04/12.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  Modified by Teigo Nakamura
//

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "gamestatus.h"

//
// プレイヤークラスの基底クラス．
// Player を public の基底クラスとし，グループのプレイヤーを拡張クラスとして実装する．
// 他のグループのプレイヤークラス，大会プログラムと互換性がなくなりコンパイルできなく
// なる可能性があるので，Player クラスには，いっさい手を加えないこと．
//
class Player {
    friend class Dealer;                // フレンドクラスの設定
private:
    int id;                             // プレイヤID
    std::string name;                   // プレイヤ名

protected:
    CardSet hand;                       // 手札

public:
    bool takeCards(Card &c);            // カード c を自分の手札(hand)に加える
    bool takeCards(CardSet &cs);        // カード集合 cs のすべてのカードを自分の手札に加える
                                        // その結果，cs は空集合になる

    Player(const char *s = "noname");   // (プレイヤ名を与える)コンストラクタ
    virtual ~Player() { }               // デストラクタ

    void setId(int);                    // プレイヤID(id)の設定
    int getId() const;                  // プレイヤID(id)のアクセサ
    void clearHand();                   // 手札を空にする
    bool isEmptyHanded() const;         // 手札が空集合かどうかの判定関数 
    CardSet &inHand() { return hand; }  // 手札(hand)のアクセサ
    std::string toString(void) const;   // プレイヤの内部状態(name, hand)の文字列化
    int size() const { return hand.size(); }            // 手札の枚数のアクセサ
    std::string playerName() const { return name; }     // プレイヤ名(name)のアクセサ
    std::string getName() const { return name; }        // プレイヤ名(name)のアクセサ

    // 派生クラスでオーバーライドする関数
    virtual void ready(void) { }
    virtual bool follow(const GameStatus &gstat, CardSet &cards);
    virtual bool approve(const GameStatus &gstat);
};

#endif
