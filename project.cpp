#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <math.h>
#include <random>

#define MCW MPI_COMM_WORLD

using namespace std;


// sort 2d vector by second(1th) element
bool sortVectorFunc(vector<int> x, vector<int> y) {
    return x[1] > y[1];
}

vector<vector<int> > assessHand(vector<vector<int> > myHand) {
    for(int i = 0; i < myHand.size(); i++) {
        myHand[i][1] = 1;
        for(int j = 0; j < myHand.size(); j++) {
            if(i != j) {
                if(myHand[i][0] == myHand[j][0]) {
                    myHand[i][1] += 1;
                }
            }
        }
    }
    sort(myHand.begin(), myHand.end(), sortVectorFunc);
    return myHand;
}

// this takes a hand and a new card and returns the new hand sorted by quantity of each card
// this assumes hand is 2d vector
// returns the hand, assumption is that you would discard the last element (send it to the next player/process)
vector<vector<int> > takeYourTurn(vector<vector<int> > hand, int newCard) {
    int goingFor = hand[0][0];
    int quantity = hand[0][1];
    int newCardQuantity = 0;
    vector<int> newCardVec;
    newCardVec.push_back(newCard); // I don't want to mess with weird constructors
    newCardVec.push_back(newCardQuantity);
    hand.push_back(newCardVec);
    hand = assessHand(hand);
    sort(hand.begin(), hand.end(), sortVectorFunc);
    return hand;
}


// thank you kind c++ documentation for this code
int randomFunction(int i) {
	random_device rd;  //Will be used to obtain a seed for the random number engine
	mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	uniform_int_distribution<> distrib(0, i);
	int num = distrib(gen);
	return num;
}

vector<int> initializeDeck(int numOfDecks) {
    vector<int> deck;
    for(int i = 0; i < numOfDecks * 4; i++) {
        for(int j = 1; j < 14; j++) {
            deck.push_back(j);
        }
    }
    random_shuffle(deck.begin(), deck.end(), randomFunction);
    return deck;
}


int main(int argc, char  **argv){

    //=================//
    //=== VARIABLES ===//
    //=================//

    int rank, size, spoons, numDeck, tag, data;
    vector<vector<int> > myHand;
    bool isDealer, isTurn, done, roundFinished = false;
    bool isAlive = true;

    srand(time(NULL) * rank);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank);
    MPI_Comm_size(MCW, &size);
    MPI_Status status;

    //===================//
    //=== SET UP GAME ===//
    //===================//

    if(size < 2){
        cout << "Need at least 2 players to play the game...(Minimum 3  processors required: 2 players, 1 to control spoon pile)" << endl;
        return 0;
    }

    if(size > 13){ // how many decks in play at the start.
            numDeck = ceil((size-1)/13);
    } else {
        numDeck = 1;
    }

  vector<int> deck = initializeDeck(numDeck);

    if(rank == 1){ // rank 1 to be first dealer.
        cout << "I've become the dealer!" << endl;
        isDealer = true;
    }

    
    // each process picks their first 4 cards from their own deck (because it's simple)
    vector<vector<int> > initialHand;
    for(int i = 0; i < 4; i++) {
        vector<int> newCard;
        int card = deck[deck.size() - 1];
        deck.pop_back();
        newCard.push_back(card);
        newCard.push_back(1);
        initialHand.push_back(newCard);
    }

    // assess the deck
    myHand = assessHand(initialHand);

    //=================//
    //=== GAME LOOP ===//
    //=================//


    //=================//
    //=== MPI_TAGS ====// (Just Ideas here)
    // Tag = -1 ( You lost [Player specific] )
    // Tag = 1 (You grabbed a spoon [Player specific] )
  // Tag = 2 (Someone else grabbed a spoon! Request one NOW!)
    // Tag = 0 (Passed a card [Player specific], Someone requested a spoon [GM specific])

    for(int numOfRounds = size - 2; numOfRounds > 0; numOfRounds--){
        cout << "Starting game!" << endl;
        // -- initialize round -- //
        int numAlive;
        // initialize deck again, so we don't run out of cards
        deck = initializeDeck(numDeck);
        if(rank == 0){
            spoons = numOfRounds;  // number of spoons in play
            numAlive = spoons + 1; // number of current players, only GM needs access to this variable.
        }
        done = false;
        // -- during round -- //
        while(!roundFinished){  // exit condition.
            if(rank != 0){
            cout << "Rank:" << rank << " =" << myHand[0][0] << "-" << myHand[1][0] << "-" << myHand[2][0] << "-" << myHand[3][0] << endl;
            }
            //------------------//
            //---GAME MANAGER---//
            //------------------//
            // Responsibilities:
            // - GM gives spoons upon request
            // - GM keeps track of how many spoons are in play and when to quit the round
            // - GM broadcasts when a spoon has been taken
            // - Tells a player when they have lost
            if(rank == 0){
                // tag = -100;
                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &tag, MPI_STATUS_IGNORE); // check if a player wants to grab a spoon.
                if(tag == 1){ // someone wants a spoon;
                        cout << "Someone wants a spoon!" << endl;
                        MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &status); //receives the request for a spoon
                        if(data > 0 && spoons > 0){ // if there are spoons left.
                                MPI_Send(&data, 1, MPI_INT, 1, 1, MCW); // player is sent a spoon.
                                int grabASpoon = -1;
                                MPI_Bcast(&grabASpoon, 1, MPI_INT, 0, MCW);
                                spoons = spoons - 1;     // reduce the number of spoons
                                numAlive = numAlive - 1; // exit condition
                        }    else {
                                roundFinished = true;
                                int youLose = -1;
                                MPI_Send(&youLose, 1, MPI_INT, data, 1, MCW); // sends you lost message to player without a spoon.
                        }
                }
                data = 0; // resets data
      }
            
            //------------------//
            //------PLAYERS-----//
            //------------------//
            // Responsibilities:
            // - players are to keep track of cards/quantity
            // - players are to ask for a spoon if 4 of a kind is achieved
            // - players are to pass cards to the next player in line
            // - players are to do nothing if not alive
            // - players are to check if another player has picked up a spoon
            
            if(rank != 0){ // if not rank zero.
                    if(isAlive){ // if a player is still in the game either playing or waiting for the next round to start.
                            if(!done){ // if a player does not yet have a spoon, but is still playing the game.
                                    MPI_Iprobe(0, MPI_ANY_TAG, MCW, &tag, MPI_STATUS_IGNORE); // check if the other players grabbed a spoon.
                                    if(tag) { // if message is waiting
                                            cout << "There is a message waiting, we need to grab a spoon " << endl;
                                            MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, MPI_STATUS_IGNORE);
                                            if(data == -2){
                                                    cout << "Someone grabbed a spoon" << endl;
                                                    int giveMeASpoon = 0;
                                                    MPI_Send(&giveMeASpoon, 1, MPI_INT, 0, 0, MCW); // ask rank 0 politely for a spoon.
                                                    cout << "Rank: " << rank << " wants a spoon!" << endl;
                                                    MPI_Recv(&giveMeASpoon, 1, MPI_INT, 0, MPI_ANY_TAG, MCW, &status);
                                                    if(!giveMeASpoon) {
                                                            isAlive = false;
                                                            done = true;
                                                            }
                                                    }
                                            }
                                            tag = 0;  // reset the tag to be zero;
                                            data = 0; // reset the data to be zero;
                                            int potentialDest = (rank + 1) % size;
                                            int dest = potentialDest == 0 ? potentialDest + 1 : potentialDest;
                                            int newCard;
                                            if(isDealer){
                                                    // draw a card from the deck
                                                    newCard = deck[deck.size() - 1];
                                                    deck.pop_back();
                                            }
                                            else {
                                                // get new card from previous player
                                                MPI_Recv(&newCard, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &status);
                                                if(!isAlive) {
                                                // automatically send card along
                                                        MPI_Send(&newCard, 1, MPI_INT, dest, 0, MCW);
                                                        continue; // process is not alive, don't do anything else
                                                }
                                            }
                                            vector<vector<int> > newHand = takeYourTurn(myHand, newCard);
                                            vector<int> discardCard = newHand[newHand.size() - 1];
                                            newHand.pop_back();
                                            int discard = discardCard[0];
                                            cout << rank << " Discarding... " << discard << endl;
                                            if(newHand[0][1] == 4 ){
                                                cout << " I'm Winning! --- Rank:" << rank << " =" << newHand[0][0] << "-" << newHand[1][0] << "-" << newHand[2][0] << "-" << newHand[3][0] <<  endl;
                                                    int giveMeASpoon = 0;
                                                    MPI_Send(&giveMeASpoon, 1, MPI_INT, 0, 1, MCW); // ask rank 0 politely for a spoon.
                                                    MPI_Recv(&giveMeASpoon, 1, MPI_INT, 0, MPI_ANY_TAG, MCW, &status);
                                                    if(giveMeASpoon == -1) {
                                                        isAlive = false; // you lose!
                                                    }
                                            }
                                            myHand = newHand;
                                            // send discarded card along to next process
                                            MPI_Send(&discard, 1, MPI_INT, dest, 0, MCW);
                                    }
                            }
                    }
        //-- round over --//
        MPI_Barrier(MCW);
        }
    }

    
    //=================//
    //=== GAME OVER ===//
    //=================//

    if(isAlive && rank != 0){
        cout << "===========================" << endl;
        cout << "  Player #" << rank << " is the Spoons Champion!" << endl;
        cout << "===========================" << endl;
    }
    
    MPI_Finalize();
    return 0;
}
