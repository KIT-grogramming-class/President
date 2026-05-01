//
//  Player.cpp
//  PlayingCard
//
//  Created by 下薗 真一 on 09/04/12.
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

Player::Player(char const *given) {
    hand.makeEmpty(); //clear();
    id = -1;
    name = given;
    return;
}

std::string Player::toString() const {
    std::string ret;
    ret = name;
    ret.resize(8, ' ');
    ret += ": " + hand.toString();
    return ret;
}

void Player::setId(int temp) {
    id = temp;
}

int Player::getId() const {
    return id;
}

void Player::clearHand() {
    hand.clear();
}

bool Player::isEmptyHanded() const {
    return hand.isEmpty();
}

bool Player::takeCards(Card &c) {
    return hand.insert(c);
}

bool Player::takeCards(CardSet &cs) {
    Card tmp;
    while (!cs.isEmpty()) {
        cs.draw(tmp, 0);
        hand.insert(tmp);
    }
    return true;
}

bool Player::approve(const GameStatus &gstat) {
    // 何もしない
    return true;
}

bool Player::follow(const GameStatus &gstat, CardSet &cards) {
  // (gstat.pile は無視して)手札からランダムに1枚引いて出す
  // 通ればラッキー，通らなければパス扱い

    CardSet pile(gstat.pile);  // 作業用にコピー
    Card tmp;

    hand.draw(tmp, -1);   // hand からランダムに1枚引いて (tmp)
    cards.insert(tmp);    // (tmp を) cards に入れる

    return true;
}
