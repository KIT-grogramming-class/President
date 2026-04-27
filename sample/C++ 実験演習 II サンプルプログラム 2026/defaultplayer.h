//
//  randomplayer.h
//
//  Created by 下薗 真一 on 09/04/12.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  Modified by Teigo Nakamura
//

#ifndef _DEFAULTPLAYER_H_
#define _DEFAULTPLAYER_H_

#include "player.h"
#include "gamestatus.h"

//
// Player クラスを継承しただけのクラス
//
class DefaultPlayer : public Player {
public:
    DefaultPlayer(const char *name = "Default") : Player(name) { }
    ~DefaultPlayer() { }

    // 思考処理の関数
    // サブクラスで実装しない場合は，基底クラス(Player)で作成されたものが使われる
    // void ready(void);
    // bool follow(const GameStatus &gstat, CardSet &cards);
    // bool approve(const GameStatus &gstat);
};

#endif
