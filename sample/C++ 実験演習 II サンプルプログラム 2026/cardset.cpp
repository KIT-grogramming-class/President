//
// cardset.cpp - トランプカードの集合型(C++版)
//  作者: (あなたの名前); 日付: (完成した日付)
//

#include <iostream>
#include <random>
#include <cstdlib>
#include <ctime>

#include "card.h"
#include "cardset.h"

//
// CardSet::find() - target に等しいカードの位置(添字)を返す (-1: ない)
//
int CardSet::find(const Card &target) const {
    for (int i = 0; i < count; i++) {
        if (target.equals(cards[i]))
            return i;
    }
    return -1;  // 見つからなかった
}

//
// CardSet::find() - 数字が num のカードの位置(添字)を返す (-1: ない)
//
int CardSet::find(int number) const {
    for (int i = 0; i < count; i++) {
        if (number == cards[i].rank())
            return i;
    }
    return -1;  // 見つからなかった
}

//
// CardSet::setupDeck() - 自身に全部の(maxnumcard 枚の)カードを入れる
//
void CardSet::setupDeck(void) {
    Card c;
    int suit, num;

    for (suit = Card::SUIT_SPADE; suit <= Card::SUIT_CLUB; suit++)
        for (num = 1; num <= 13; num++) {
            c.set(suit, num);
            insert(c);
        }
    c.set(Card::SUIT_JOKER, Card::RANK_JOKER);
    insert(c);
}

// 自身から targetpos 枚目 (先頭は 0 枚目) のカードを除き，
// そのカードを引数 card で返す
// targetpos が -1 またはカード数以上のときはランダムに選ぶ
// 第2引数を省略した場合は -1 が指定されたものとみなす(デフォルト引数)
// (false: 要素数に変化なし，card の値は変更されない; true: 成功)
// remove() を実装した後でコメントを外して，draw を使えるようにせよ
bool CardSet::draw(Card &card, int targetpos) {
    std::random_device rnd;

    if (count == 0)
        return false;
    if (targetpos < 0 || targetpos >= count)
        targetpos = random() % count;

    card = cards[targetpos];
    return remove(card);
}

//
// CardSet::insert() - newcard を入れる (false: 失敗; true: 成功)
// 既にあるカードを挿入しようとしたか，または，満杯状態の場合は失敗
//
bool CardSet::insert(const Card &newcard) {
    int position = find(newcard);
    if (position >= 0)          // 既にある
        return false;
    if (count >= maxnumcard)    // もう入らない
        return false;
    // 最後に追加
    position = count;
    count++;
    cards[position] = newcard;

    return true;
}

//
// CardSet::remove() - 自身から targetと同一のカードを取り除く(false: 失敗; true: 成功)
//
bool CardSet::remove(const Card &target) {
    int pos = find(target); // 除くカードの位置

    if (pos < 0) return false;    // target のカードは無い
    // 削除
    for (int i = pos; i < count - 1; i++)
        cards[i] = cards[i+1];
    count--;
    return true;
}

//
// CardSet::remove() - 自身から数字が num であるカードを1枚除く(false: 失敗; true: 成功)
//
bool CardSet::remove(int number) {
    int pos = find(number); // 除くカードの位置

    if (pos < 0) return false;    // num 番のカードは無い
    // 削除
    for (int i = pos; i < count - 1; i++)
        cards[i] = cards[i+1];
    count--;
    return true;
}

// シャフルする
void CardSet::shuffle(void) {
    std::random_device rnd;
    int t, i, j, k;
    Card swap;

    i = rnd() % count;
    swap = cards[i];
    cards[i] = cards[count-1];
    cards[count-1] = swap;

    for (t = 0; t < 100; t++) {
      i = rnd() % count;
      j = i + (rnd() % (count - i));
      for (k = 0; i + k < j; k++) {
          swap = cards[k];
          cards[k] = cards[j - k - 1];
          cards[j - k - 1] = swap;
       }
    }
}

// rank と suit で昇順にソートする
void CardSet::sort() {
    Card t;
    for(int i = 0; i < count - 1; ++i) {
        for(int j = i+1; j < count; ++j) {
            if (cards[i].rank() > cards[j].rank()
                    || (cards[i].rank() == cards[j].rank() && cards[i].suit() > cards[j].suit())) {
                t = cards[j];
                cards[j] = cards[i];
                cards[i] = t;
            }
        }
    }
}

//
// CardSet::printOn() - 自身を出力する
//
std::ostream&CardSet::printOn(std::ostream &out, const bool faceup) const {
    out << toString();
    return out;
}

// コピーコンストラクタ
CardSet::CardSet(const CardSet &orig) {
    count = orig.count;     // 現在の集合内のカード数
    for (int i = 0; i < count; i++)
        cards[i] = orig.cards[i];   // カードのデータ
}

//////////////////////////////////////////////////////////////////////
// 「大富豪」で追加したもの
//////////////////////////////////////////////////////////////////////
bool CardSet::sortedPrint = false;

bool CardSet::insert(const CardSet &cards) {
  int ret = false;
  for (int i = 0; i < cards.count; i++) {
    ret = insert(cards.cards[i]);
  }
  return ret;
}

Card &CardSet::operator[](int i) {
    return cards[i];
}

const Card &CardSet::operator[](int i) const {
    return cards[i];
}

//
// CardSet::print() - 自身の状態を標準出力に出力する
//
void CardSet::print(void) const {
    //printf("((CARDSET))\n\t");
    if (count == 0) {
        printf("no cards");
        return;
    }
    for (int i = 0; i < count; i++) {
        //printf("\t");
        cards[i].print();
        printf(", ");
    }
    printf("\n");
}

std::string CardSet::toString() const {
    std::string ret;
    char buf[80];

    sprintf(buf, "%2d, ", count);
    ret += buf;

    if (count == 0) {
        ret += "(no card)";
        return ret;
    }

    CardSet cs = *this;
    if (sortedPrint) cs.sort(Card::cmp);

    for (int i = 0; i < cs.count; i++) {
        ret += cs.cards[i].toString();
        ret += " ";
    }
    return ret;
}


////////////////////////////////////////////////////////////////////////
// added by Teigo Nakamura
////////////////////////////////////////////////////////////////////////

//
// CardSet::member(Card c) - c が集合の要素なら true
// 
bool CardSet::member(Card c) const { return find(c) >= 0; }

//
// CardSet::subsetof(CardSet &cs) - 自身が cs の部分集合なら true
// 
bool CardSet::subsetof(const CardSet &cs) {
  for (int i = 0; i < count; i++) {
    if (! cs.member(cards[i])) return false;
  }
  return true;
}

//
// CardSet::equal(CardSet &cs) - 自身と cs が集合として等しいなら true
//
bool CardSet::equal(const CardSet &cs) {
  return count == cs.count && subsetof(cs);
}

//
// CardSet::remove(CardSet &cs) - 自身から cs の集合中のカードを除く(true: 失敗; false: 成功)
//
bool CardSet::remove(const CardSet &cs) {
  bool ret = false;

  for (int i = 0; i < cs.count; i++) {
    if (remove(cs.cards[i])) ret = true;
  }
  return ret;
}

//
// CardSet::intersection(CardSet &cs) - 自身と cs の共通集合を返す
// 
CardSet CardSet::intersection(const CardSet &cs) const {
  CardSet ret;

  ret.makeEmpty();
  for (int i = 0; i < count; i++) {
    if (cs.member(cards[i]))
      ret.insert(cards[i]);
  }
  return ret;
}

//
// CardSet::sort() - 要素の並び換え．比較関数を引数で与える．
//
void CardSet::sort(bool (*cmp_func)(const Card &a, const Card &b)) {
  Card tmp;

  for (int i = count - 1; i > 0; i--) {
    for (int j = 0; j < i; j++) {
      if (cmp_func(cards[j+1], cards[j])) {
        tmp = cards[j];
        cards[j] = cards[j+1];
        cards[j+1] = tmp;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
