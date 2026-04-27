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

#include <iostream>
#include <string>

#include "card.h"
#include "cardset.h"
#include "player.h"
#include "gamestatus.h"
#include "simpleplayer.h"

void SimplePlayer::ready() {
    // memory.clear();
    // trump.clear();
    hand.sort(myCardCmp);
}

// 「(弱い方から順に) 出せるなら温存せずに出す！！」という貪欲戦略
bool SimplePlayer::follow(const GameStatus &gstat, CardSet &cs) {
    CardSet pile(gstat.pile);
    Card tmp;
    int leadSize;

    hand.sort(myCardCmp);  // 手札をソート(弱い方から順に)

    leadSize = pile.size();

    if (leadSize == 0) { // 自分がリーダなら
      tmp = hand.at(0); // 一番弱いカードを1枚出す
      cs.insert(tmp);
      hand.remove(tmp);
      return true;
    }
    else if (leadSize == 1) { // リードが1枚の場合
      for (int i = 0; i < hand.size(); i++) {
        tmp = hand.at(i);
        if (tmp.isGreaterThan(pile.at(0))) {
          cs.insert(tmp);
          hand.remove(tmp);
          return true;
        }
      }
    }
    else { // 2枚以上の場合
      for (int i = 0; i < hand.size(); i++) {
        tmp = hand.at(i);
        if (tmp.isGreaterThan(pile.at(0)) || tmp.isGreaterThan(pile.at(1))) { // by Teigo Nakamura 2020/07/04
          cs.clear();
          cs.insert(tmp);
          for (int j = 1; j < leadSize && i + j < hand.size(); j++) {
            if (hand.at(i).rank() == hand.at(i + j).rank())
              cs.insert(hand.at(i + j));
          }
          if (cs.size() == leadSize) { // 出せるカードが揃った
            hand.remove(cs); // カード集合を remove 
            return true;
          }
        }
      }
    }

    // 出せるカードがないのでパス
    cs.clear();
    return true;
}

bool SimplePlayer::approve(const GameStatus &gstat) {
    //
    // 未実装
    // 
    return true;
}
