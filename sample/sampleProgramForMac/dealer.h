//
//  Dealer.h
//  PlayingCard
//
//  Created by 下薗 真一 on 09/04/12.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  Modified by Teigo Nakamura in 2013.
//

#ifndef _DEALER_H_
#define _DEALER_H_

#include "player.h"
#include "gamestatus.h"

class Dealer {
 public:
  static const int NUM_OF_ALL_CARDS = 53;
  static const int maxNumOfPlayers  = 10;       // 最大プレーヤ数
  static int verbose;

 private:
  CardSet theDeck;                    // カードデッキ
  CardSet discarded;                  // 現在の場のカード
  Card discardedRank;                 // 現在の場のカードの中の Joker 以外のカードのランク
                                      // ペアしか考えていないので全部同じ数字
    
  Player *players[maxNumOfPlayers];   // プレイヤへのポインタの配列(10人まで)
                                      // 先頭から，大貧民，... ，大富豪の順
  int numberOfPlayingPlayers;         // 現在プレイ中のプレイヤの数 (上がったプレイヤは除く)
  int numberOfParticipants;           // 全プレイヤ数
  int turn;                           // ターンの回ってきたプレイヤの添字
  int pauper;                         // 貧民
  int leaderIndex;                    // リーダとなっているプレイヤの添字
  bool noMillionaire;                 // 大富豪の搾取を行なわないかどうかのフラグ

  Player *id2player[maxNumOfPlayers]; // プレイヤid からプレイヤを取得するためのテーブル
  int score[maxNumOfPlayers][maxNumOfPlayers+1];  // プレイヤのスコア
  CardSet dupHands[maxNumOfPlayers];  // 各プレイヤの手札のコピー(チェック用)

public:
  Dealer();

  void newGame();
  bool regist(Player *);              // プレイヤの登録
  bool deal(int);                     // 引数で与えた枚数のカードを配り(*)，turn をリセット
  bool dealAll();                     // 各プレイヤに手札を配布
  void show();                        // 各プレイヤの状況の表示
    
  int howManyPlayers() const { return numberOfPlayingPlayers; }    // numberOfPlayers のアクセサ
  int howManyParticipants() const { return numberOfParticipants; } // numberOfParticipantes のアクセサ
        
  void showDiscardedToPlayers();      // 各プレイヤに approve
    
  bool accept(CardSet &opened);       // プレイヤが出したカード(opend)が，場のカードに対し
                                      // て合法かどうかの判定

  bool checkRankUniqueness(CardSet &);  // 引数のカードセットが同じ数字のペアになっているかど
                                        // うかのチェック(Jokerは任意の数だとみなす)
                                        // true: OK，false: ダメ
  Card getCardRank(CardSet &);          // 引数のカードセットの中で，最初にみつかった Joker
                                        // 以外のカードを返す．Joker しかない場合やカードセッ
                                        // トが空集合のときは，cs[0] を返す．CardSet内で
                                        // operator[] が配列のように定義されている

  const CardSet &discardPile();         // 現在の場(discarded) のアクセサ
  void clearDiscardPile();              // 現在の場(discarded) を空にする
  bool playerInTurnIsLeader();          // 現在の手番のプレイヤがリーダかどうかの判定
  void setAsLeader();                   // 現在の手番のプレイヤをリーダとする
  void setAsLeader(int);                // 引数のプレイヤをリーダとする
  Player &player(int);                  // 引数のプレイヤのアクセサ
  int numberOfFinishedPlayers();        // 上がったプレイヤ数
  void withdrawPlayer(int);             // 引数のプレイヤをゲームから除く(上がったので)

  Player &playerInTurnFinished();       // 現在の手番のプレイヤの上がり処理
    
  Player &playerInTurn();               // 現在の手番のプレイヤのアクセサ
  Player &nextPlayer();                 // 次の手番のプレイヤのアクセサ＋手番の更新
  Player &currentLeader();              // 現在のリーダの取得
    
  void putBackOpened(CardSet &);        // カードの返却(合法手でなかった場合に使用)

  GameStatus gameStatus(void) const;    // gamestatus のアクセサ

  ///////////////
  void updateScore();                   // スコアの更新
  void printScore();                    // 各プレイヤのスコアの表示
  void printResult();                   // 最終結果の表示

  bool isEmptyHanded(int);              // 引数のプレイヤの手札が空集合かどうかの判定

};

#endif
