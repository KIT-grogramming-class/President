//
//  Modified by Teigo Nakamura in 2013, 2014.
//

#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "card.h"
#include "cardset.h"
#include "player.h"
#include "simpleplayer.h"
#include "defaultplayer.h"
#include "thinkTA1.h"
#include "dealer.h"

void registerPlayers(Dealer & d) {
    d.regist(new DefaultPlayer("Default1")); 
    d.regist(new DefaultPlayer("Default2")); 
    d.regist(new  SimplePlayer("Simple1 ")); 
    d.regist(new  SimplePlayer("Simple2 ")); 
    d.regist(new      ThinkTA1("ThinkTA1")); 
}

int main (int argc, char * const argv[]) {
    int opt;
    bool autoPlay = false;
    int numOfGames = 5;

    while ((opt = getopt(argc, argv, "n:ahs")) != -1) {
      switch(opt) {
      case 'n':
        numOfGames = atoi(optarg);
        break;
      case 'a':
        autoPlay = true;
        break;
//      case 'v':
//        Dealer::verbose = true;
//        break;
      case 's':
        CardSet::sortedPrint = true;
        break;
      case 'h':
      default:
        std::cerr << "Usage: " << argv[0] << " [-n number_of_games] [-a] [-s] [-h]" << std::endl;
        std::cerr << "       -n number_of_games ; 5 games (default)" << std::endl;
        std::cerr << "       -a                 ; autoplay mode" << std::endl;
        std::cerr << "       -s                 ; sorted print of cardset" << std::endl;
//        std::cerr << "       -v                 ; verbose mode" << std::endl;
        std::cerr << "       -h                 ; show this message" << std::endl;
        exit(1);
      }
    }

    std::cout << "Hello, Daifugou World!\n";
    
    Dealer dealer;
    CardSet opened;
    GameStatus state;
    char dummy[32];
    
    std::cout << "Registering players." << std::endl;

    registerPlayers(dealer);

    for (int g = 1; g <= numOfGames; g++) {
        std::cout << std::endl << "### Game " << g << std::endl;
        dealer.newGame();
        dealer.showDiscardedToPlayers();

        bool passed = true;
        int npass = 0;

        while (true) {
            while (true) {
                if (npass >= dealer.howManyPlayers() * 5) {
                    std::cout << "### Passes are repeated many times, so stop the game." << std::endl;
                    break;
                }
                if (passed && dealer.playerInTurnIsLeader() ) {
                    dealer.clearDiscardPile();
                    if (! autoPlay) {
                      std::cout << "flushed. hit Return ";
                      std::cin.getline(dummy,31);
                    }
                    std::cout << "########################################################################" << std::endl;
                    dealer.show();
                }
                std::cout << "Now " << dealer.playerInTurn().playerName() << "'s Turn: ";
                state = dealer.gameStatus();
                opened.clear();
                dealer.playerInTurn().follow(state, opened);
                std::cout << opened;
                if (opened.isEmpty() || !dealer.accept(opened)) {
                    if (!opened.isEmpty()) {
                        dealer.putBackOpened(opened);
                        std::cout << "--> Rejected ";
                    }
                    std::cout << "--> Pass." << std::endl;
                    passed = true;
                    npass++;
                } else {
                    passed = false;
                }
                if (dealer.playerInTurn().isEmptyHanded() && dealer.isEmptyHanded(dealer.playerInTurn().getId()))
                    break;
                if (!passed) {
                    dealer.setAsLeader();
                    npass = 0;
                    std::cout << "--> Accepted." << std::endl;
                    // std::cout << dealer.discardPile().toString()
                    // << " lead by " << dealer.currentLeader().playerName() << ". " << std::endl;
                }

                dealer.showDiscardedToPlayers();
                dealer.nextPlayer();
            }
            if (npass >= dealer.howManyPlayers() * 5) {
              npass = 0;
              break;
            }
            std::cout << "==> " << dealer.playerInTurn().playerName() << " finished. " << std::endl << std::endl;
            //dealer.show();
            dealer.showDiscardedToPlayers();
            dealer.playerInTurnFinished();
            npass = 0;
            if (dealer.howManyPlayers() == 1) {
                dealer.playerInTurnFinished();
                break;
            }
        }
        std::cout << std::endl 
                  << "### Result of game " << g << ": " << std::endl;
        for (int i = 0; i < dealer.howManyParticipants() ; i++) {
            std::cout << i+1;
            switch (i+1) {
                    case 1:
                        std::cout << "st ";
                    break;
                    case 2:
                        std::cout << "nd ";
                    break;
                    case 3:
                        std::cout << "rd ";
                    break;
                    default:
                        std::cout << "th ";
                    break;
            }
            std::cout << "place: " << dealer.player(dealer.howManyParticipants() - i - 1).playerName() << std::endl;
        }
        dealer.updateScore();
        dealer.printScore();
    }
    dealer.printResult();
    return 0;
}
