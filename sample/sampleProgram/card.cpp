//
// card.cpp - トランプカード型(C++版)
//  作者: (あなたの名前); 日付: (完成した日付)
//

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

#include "card.h"

// クラス変数（定数）の初期化
//const char *Card::suitnames[] = { "Blank", "Spade", "Diamond", "Heart", "Club", "Joker", };
const char *Card::suitnames[] = { "__", "S", "D", "H", "C", "Jkr", };
const char *Card::ranknames[] = {
    " ?", " A", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10", " J", " Q", " K", "_", "Jkr",
};

//
// Card::isValid() - まともなカードか？
//
bool Card::isValid(void) const {
    if (((SUIT_SPADE <= suitval) && (suitval <= SUIT_CLUB))
         && (1 <= rankval && (rankval <= 13)) )
        return true;
    else if (suitval == SUIT_JOKER)
        return true;
    return false;
}

//
// Card::readFrom() - 文字列から値を得る (戻り値：残りの文字列)
//
const char *Card::readFrom(const char *str) {
    char *ptr;
    int s;

    suitval = SUIT_BLANK;
    rankval = 0;

  // 4組のいずれ？
  for (s = SUIT_SPADE; s <= SUIT_CLUB; s++) {
      if (tolower(*str) == tolower(Card::suitnames[s][0])) { // 一文字目だけで判定
          suitval = s;
          do { ++str; } while (isalpha(*str));
          break;
      }
  }
  if (SUIT_SPADE <= s && s <= SUIT_CLUB) {
      // なら番号も得る
      while (isspace(*str)) { ++str; }
      rankval = strtod(str, &ptr);
      if (0 < rankval && rankval <= 13) {
          return ptr;
      } else {
          if (rankval == 0) {
              switch (tolower(*str)) {
              case 'a':
              case 'A':
                  rankval = 1;
                  break;
              case 'j':
              case 'J':
                  rankval = 11;
                  break;
              case 'q':
              case 'Q':
                  rankval = 12;
                  break;
              case 'k':
              case 'K':
                  rankval = 13;
                  break;
              }
          }
          return str;
      }
  } else if (tolower(*str) == 'j') {
    suitval = SUIT_JOKER;
    rankval = RANK_JOKER; // joker は number を 15 としておく．
    do { ++str; } while (isalpha(*str));
    return str;
  }
  return str;
}

//
// Card::readFrom() - ストリームから値を得る
//
std::istream & Card::readFrom(std::istream & in) {
    std::string buf;
    std::getline(in, buf);
    readFrom(buf.c_str());
    return in;
}

//
// Card::printOn() - ファイルストリームに出力する
// 第二引数に false を与えると，カードを伏せて表示
//
std::ostream & Card::printOn(std::ostream& ostr, const bool faceup) const {
  ostr << toString();
  return ostr;
}

//////////////////////////////////////////////////////////////////////
// 「大富豪」で追加したもの
//////////////////////////////////////////////////////////////////////
// 大富豪的な意味で強いカードか？
bool Card::isGreaterThan(const Card & another) const {
    if (suitval == SUIT_JOKER)
        return true;
    if (another.suitval == SUIT_JOKER)
        return false;
    return (rankval + 10) % 13 > (another.rankval + 10) % 13;
}

// 大富豪的な意味での強さ
// isGreaterThan のやり方に合わせる
int Card::strength(void) const {
    if (suitval == SUIT_JOKER)
      return 15; // Card::RANK_JOKER
    else
      return (rankval + 10) % 13 + 1;
}

// 大富豪的な意味での強さ
//int Card::strength(void) const {
//    switch(rankval) {
//    case 0:
//    case Card::RANK_JOKER:
//        return rankval;
//    case 1:
//    case 2:
//        return rankval + 11;
//    default:
//        return rankval - 2;
//    }
//}

// 文字列化
std::string Card::toString() const {
    std::string ret;
    char str[100];
    ret = "[";
    sprintf(str, "(%2d,%2d)", suitval, rankval);
    //  ret += str;
    if(suitval < SUIT_JOKER)
        ret = ret + suitnames[suitval] + ranknames[rankval];
    else if(suitval == SUIT_JOKER)
        ret += "Jkr";
    else if(suitval == SUIT_BLANK)
        ret += "BLANK";
    ret += "]";
    return ret;
}

//
// Card::scan() - 標準出力から値を得て自身に設定する(true: 取得成功; false: 取得失敗)
//
bool Card::scan(void) {
  char buf[BUFSIZ];
  int s;

  suitval = SUIT_BLANK;
  // 4組のいずれかなら番号も入力する
  if(scanf("%s", buf) < 1)
    return false;
  for(s = SUIT_SPADE; s <= SUIT_CLUB; s++)
    if(!strcmp(buf, suitnames[s])) {
      suitval = s;
      if(scanf("%d", &rankval) < 1)
        return false;
      if(0 < rankval && rankval <= 13)
        return true;
      return false;
    }
  // joker はそのまま(rankval は RANK_JOKER とする)
  if(!strcmp(buf, "joker")) {
    suitval = SUIT_JOKER;
    rankval = RANK_JOKER;
    return true;
  }
  
  return false;    // エラー
}
