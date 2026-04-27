//
//  Dealer.cpp
//  PlayingCard
//
//  Created by 下薗 真一 on 09/04/12.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  Modified by Teigo Nakamura in 2013.
//

#include <iostream> 
#include <iomanip> 
#include <string>

#include "card.h"
#include "cardset.h"
#include "player.h"
#include "dealer.h"

int Dealer::verbose = false;

Dealer::Dealer() {
    numberOfPlayingPlayers = 0;
    numberOfParticipants = 0;
    turn = 0;
    pauper = 0;
    for (int i = 0; i < maxNumOfPlayers; i++){
      for (int j = 0; j <= maxNumOfPlayers; j++) {
        score[i][j] = 0;
      }
      dupHands[i].makeEmpty();
    }
    noMillionaire = true;
    return;
}

bool Dealer::regist(Player * pl) {
  //  std::cerr << "### registering of " << pl->playerName() << std::endl;
  //  std::cerr << "nop=" << numberOfParticipants << ",max=" << maxNumOfPlayers << std::endl;
  
  std::cerr << "Registering Player of " << pl->playerName() << " ... ";

    if (numberOfParticipants >= maxNumOfPlayers) {
      std::cerr << "Error: player registration exceeding the limit (" 
                << maxNumOfPlayers << ")." << std::endl;
      return false;
    }
    players[numberOfParticipants] = pl;
    id2player[numberOfParticipants] = pl;
    pl->setId(numberOfParticipants);
    numberOfParticipants++;
    pauper = 0;
    noMillionaire = true;

    std::cerr << "completed successfully." << std::endl;
    return true;
}

Player & Dealer::currentLeader() {
    return * players[leaderIndex];
}

bool Dealer::playerInTurnIsLeader() {
    return leaderIndex == turn;
}

void Dealer::newGame() {
    numberOfPlayingPlayers = howManyParticipants();
    if ( !noMillionaire )
        pauper = 0;
    for (int i = 0; i < numberOfParticipants; i++){
      players[i]->clearHand();
      //      players[i]->ready();
      dupHands[i].makeEmpty();
    }
    clearDiscardPile();
    dealAll();
    for (int i = 0; i < numberOfParticipants; i++){
      players[i]->ready();
    }
    setAsLeader();
}

void Dealer::setAsLeader() {
    leaderIndex = turn;
}

void Dealer::setAsLeader(int id) {
    turn = id;
    leaderIndex = turn;
}

bool Dealer::deal(int c) {
    Card top;

    for (int p = 0; p < numberOfParticipants ; p++) {
        players[p]->clearHand();
    }
    theDeck.setupDeck();
    theDeck.shuffle();
    for (int i = 0; i < c; i++) {
        for (int p = 0; p < numberOfPlayingPlayers; p++) {
            if ( theDeck.isEmpty() )
                break;
            theDeck.draw(top, 0);
            players[ (numberOfPlayingPlayers - 1 - p) % numberOfPlayingPlayers]->takeCards(top);
            dupHands[players[(numberOfPlayingPlayers - 1 - p) % numberOfPlayingPlayers]->getId()].insert(top);
        }
    }
    //    turn = 0;
    return true;
}

bool Dealer::dealAll() {
    return deal(53);
}

void Dealer::clearDiscardPile() {
    discarded.makeEmpty();
}

const CardSet & Dealer::discardPile() {
    return discarded;
}

bool Dealer::accept(CardSet & opened) {
        Card openedRank;

    if (! opened.subsetof(dupHands[players[turn]->getId()])) {
      std::cout << "\t!!! You don't have " << opened
                << "in your hand of " << dupHands[players[turn]->getId()];
      return false;
    }

    if (discarded.isEmpty() && opened.isEmpty() )
      return false;  // regarded as "pass for empty discard pile."

    if (!discarded.isEmpty() && discarded.size() != opened.size())  // the number of cards must be match. no five cards w/ Jkr allowed.
      return false;
    
    if (!checkRankUniqueness(opened))
      return false;
    
    openedRank = getCardRank(opened);

    if (!discarded.isEmpty()) // 場にカードがでていないのであれば無条件に受理
      if (!openedRank.isGreaterThan(discardedRank)) 
        return false;
    

    // passed all the checks.

    discarded.makeEmpty();
    discarded.insert(opened);
    dupHands[players[turn]->getId()].remove(opened);
    opened.makeEmpty();
    discardedRank=openedRank;

    return true;
}

Card Dealer::getCardRank(CardSet & cs){
    for (int i = 0; i < cs.size(); i++) {
      if (!cs[i].isJoker()){return cs[i];}
    }
    return cs[0];
}

bool Dealer::checkRankUniqueness(CardSet & cs) {
    int j = 0;
    if (cs.size() == 0)
        return false;
    if (cs[j].isJoker())
        j++;
    for (int i = j+1; i < cs.size(); i++) {
      if (cs[i].isJoker()){continue;} // 追加：途中にJkrを許す
      if (cs[j].getNumber() != cs[i].getNumber() )
        return false;
    }
    return true;
}

void Dealer::showDiscardedToPlayers() {
    GameStatus gstat = gameStatus();
    for (int i = 0; i < numberOfPlayingPlayers; i++) {
        players[(turn + i) % numberOfPlayingPlayers]->approve(gstat);
    }
    return; 
}

void Dealer::withdrawPlayer(int i) {
    Player * p;
    p = players[i];
    for ( ; i+1 < numberOfPlayingPlayers; i++) {
        players[i] = players[i+1];
    }
    if (pauper == numberOfPlayingPlayers) {
        players[i] = players[i+1];
        i++;
        pauper--;
    }
    players[i] = p;
    numberOfPlayingPlayers--;
    if (numberOfPlayingPlayers > 0) {
        turn = turn % numberOfPlayingPlayers;
        leaderIndex = leaderIndex % numberOfPlayingPlayers;
    }
}

Player & Dealer::playerInTurnFinished() {
    //int i;
    bool bankrupt = false;
    
    Player * p = 0;
    if (numberOfPlayingPlayers == howManyParticipants()
        // the first finished person is not the millionaire 
            ) { 
        if (turn != howManyParticipants() - 1)
            bankrupt = true;
        withdrawPlayer(turn); // millionaire
        if ( bankrupt && !noMillionaire) {
            pauper = numberOfPlayingPlayers-1;
            withdrawPlayer(pauper);
        }
        // 次の１行をコメントアウトすると 都落ちなし
        // noMillionaire = false;
        return *players[pauper];
    }
    withdrawPlayer(turn);
    show();
    return *p;
}


Player & Dealer::player(int i) {
    return *players[i];
}


int Dealer::numberOfFinishedPlayers() {
    return howManyParticipants() - numberOfPlayingPlayers;
}

Player & Dealer::playerInTurn() {
    return * players[turn];
}

Player & Dealer::nextPlayer() {
    turn = (turn+1) % numberOfPlayingPlayers;
    return * players[turn];
}

void Dealer::show() {
  std::cout << "===========" << std::endl;
  for (int p = 0; p < numberOfParticipants ; p++) {
    if ( p==numberOfPlayingPlayers)
      std::cout << "-----------" << std::endl;
    if ( p == leaderIndex )
      std::cout << "* ";
    else 
      std::cout << "  ";
    std::cout << players[p]->toString() << std::endl;
    if (! players[p]->inHand().equal(dupHands[players[p]->getId()])) {
      std::cout << "*** error: " << players[p]->getName()
                << "'s hand is different from the real hand. ***" << std::endl;
      std::cout << "   (real) : " << dupHands[players[p]->getId()] << std::endl << std::endl;
    }
  }
  std::cout << "===========" << std::endl << std::endl;
}

void Dealer::putBackOpened(CardSet & opened) {
  CardSet cs = opened.intersection(dupHands[players[turn]->getId()]);
  players[turn]->takeCards(cs);
}

////////////////////////////////////////////////////////////////////////
void Dealer::updateScore() {
  int id, n = howManyParticipants();
  for (int i = 0; i < n; i++) {
    id = players[n - 1 - i]->getId();
    // score[id][0] += (n - i);
    // score[id][0] += (n - i) * (n - i);
    score[id][0] += (i == 0) ? n + 1 : n - i;
    score[id][i+1]++;
  }
  // score[id][0] += n;
}

void Dealer::printScore() {
  std::cout << "### Score ###" << std::endl;
  for (int i = 0; i < howManyParticipants(); i++) {
    std::cout << id2player[i]->playerName() << ": "
              << score[id2player[i]->getId()][0] << std::endl;
  }
}

void Dealer::printResult() {
  int i, j, n = howManyParticipants();
  int tbl[n];
  int id, tmp;

  for (i = 0; i < n; i++) tbl[i] = i;

  for (i = n - 1; i > 0; i--) {
    for (j = 0; j < i; j++) {
      if (score[id2player[tbl[j]]->getId()][0] < score[id2player[tbl[j+1]]->getId()][0]) {
        tmp = tbl[j];
        tbl[j] = tbl[j+1];
        tbl[j+1] = tmp;
      }
    }
  }

  std::cerr << "### Final Result ###" << std::endl;
  std::cerr << std::setw(18) << " ";
  std::cerr << std::setw(10) << "Score  ";
  for (i = 0; i < n; i++) {
    std::string s;
    s = tostr(i+1);
    s += ((i == 0) ? "st" : ((i == 1) ? "nd" : ((i == 2) ? "rd" : "th")));
    std::cerr << std::setw(5) << s;
  }
  std::cerr << std::endl;
  std::cerr << "----------------------------";
  for (i = 0; i < n; i++) {
    std::cerr << "-----";
  }
  std::cerr << std::endl;

  for (i = 0; i < n; i++) {
    id = id2player[tbl[i]]->getId();
    std::cerr << (i+1)
              << ((i == 0) ? "st" : ((i == 1) ? "nd" : ((i == 2) ? "rd" : "th")))
              << " place: " << std::setw(8) << id2player[tbl[i]]->playerName() << ": "
              << std::setw(5) << score[id][0] << " : ";
    for (j = 0; j < n; j++) {
      std::cerr << std::setw(4) << score[id][j+1] << " ";
    }
    std::cerr << std::endl;
  }
}

bool Dealer::isEmptyHanded(int playerId) {
  return dupHands[playerId].isEmpty();
}

////////////////////////////////////////////////////////////////////////
GameStatus Dealer::gameStatus(void) const {
    GameStatus gstat;
    gstat.pile = discarded;
    gstat.turnIndex = turn;
    gstat.numPlayers = numberOfPlayingPlayers;
    gstat.numParticipants = numberOfParticipants;
    for (int i = 0; i < howManyParticipants(); i++) {
        gstat.numCards[i] = players[i]->size();
        gstat.playerID[i] = players[i]->id;
        gstat.playerName[i] = players[i]->playerName();
    }
    gstat.leaderIndex = leaderIndex;
    return gstat;
}
