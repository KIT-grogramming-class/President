//
// GameStatus.h
//
//  Created on: 2015/05/07
//      Author: sin
//

#ifndef _GAMESTATE_H_
#define _GAMESTATE_H_

#include <sstream>
template <typename T> std::string tostr(const T& t) {
  std::ostringstream os; os << t; return os.str();
}

#include "card.h"
#include "cardset.h"

struct GameStatus {
  static const int MAXIMUM_NUM_OF_PLAYERS = 8;

  CardSet pile;               // 場札(リード)
  int numPlayers;             // 現在プレイ中のプレイヤの数 (上がったプレイヤは除く)
  int numParticipants;        // 全プレイヤ数
  int turnIndex;              // ターンの回ってきたプレイヤの添字
  int leaderIndex;            // リーダとなっているプレイヤの添字
  int numCards[MAXIMUM_NUM_OF_PLAYERS]; // 各プレイヤの残りカード枚数
  int playerID[MAXIMUM_NUM_OF_PLAYERS]; // 各プレイヤのプレイヤID
  std::string playerName[MAXIMUM_NUM_OF_PLAYERS];
                              // 各プレイヤの(コンストラクタの引数に与えた)名前

  GameStatus(void) { numPlayers = 0; }  // Empty instance

  // ストリーム出力
  std::ostream &printOn(std::ostream &out) const {
    out << "Game status: ";
    out << "pile " << pile << ", ";
    out << numPlayers << " players, ";
    out << turnIndex << "'s turn, ";
    out << "numCards = [";
    for(int i = 0; i < numPlayers; i++) {
      out << numCards[i];
      out << " ";
    }
    out << "], ";
    out << "playerID = [";
    for(int i = 0; i < numPlayers; i++) {
      out << playerID[i];
      out << " ";
    }
    out << "], ";
    out << "leaderIndex = ";
    out << leaderIndex;

    out << std::endl;
    return out;
  }

  // ストリーム出力演算子 <<
  friend std::ostream &operator<<(std::ostream &ostr, const GameStatus &stat) {
    ostr << stat.toString();
    return ostr;
  }

  // 文字列化
  std::string toString() const {
    std::string ret;

    ret = "Game status: ";
    ret += "pile " + pile.toString() + ", ";
    ret += tostr(numPlayers) + " players, ";
    ret += tostr(turnIndex) + "'s turn, ";
    ret += "numCards = [ ";
    for(int i = 0; i < numParticipants; i++) {
      ret += tostr(numCards[i]) + " ";
    }
    ret += "], ";
    ret += "playerID = [ ";
    for(int i = 0; i < numParticipants; i++) {
      ret += tostr(playerID[i]) + " ";
    }
    ret += "], leaderIndex = ";
    ret += tostr(leaderIndex) + "\n";
    return ret;
  }

};

#endif // GAMESTATE_H_
