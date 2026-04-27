/*
 *  ThinkTA.h
 *  PlayingCard
 *
 *  Created by TA (Ryosuke Tadano) @ 2009
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

class ThinkTA1 : public Player {

public:
    ThinkTA1(const char *);
    bool follow(const GameStatus & gstat, CardSet & cards);
    bool approve(const GameStatus & gstat);
    bool sort(CardSet&);
};
