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

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank);
    MPI_Comm_size(MCW, &size);
    MPI_Status status;

    srand(rank);
    int prevDest = rank - 1;
    if(prevDest == 0){prevDest = size - 1;}
    int potentialDest = (rank + 1) % size;
    int dest = potentialDest == 0 ? potentialDest + 1 : potentialDest;
    
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

    //=================//
    //=== GAME LOOP ===//
    //=================//

    //=================//
    //=== MPI_TAGS ====// (Just Ideas here)
    // Tag = -1 ( You lost [Player specific] )
    // Tag = 1 (You grabbed a spoon [Player specific] )
    // Tag = 2 (Someone else grabbed a spoon! Request one NOW!)
    // Tag = 0 (Passed a card [Player specific], Someone requested a spoon [GM specific])

    MPI_Barrier(MCW);
    
    // for(int numOfRounds = size - 2; numOfRounds > 0; numOfRounds--){
        int numOfRounds = size - 2;
        cout << "Rank: " << rank << " Starting round!" << endl;
        // -- initialize round -- //
        int numAlive;
        // initialize deck again, so we don't run out of cards
        deck = initializeDeck(numDeck);
        if(rank == 0){
            spoons = numOfRounds;  // number of spoons in play
            numAlive = spoons + 1; // number of current players, only GM needs access to this variable.
        }
        done = false;

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

        // -- during round -- //
        MPI_Barrier(MCW);
        while(!roundFinished){  // exit condition.
            if(rank != 0){
                // cout << "Rank:" << rank << " =" << myHand[0][0] << "-" << myHand[1][0] << "-" << myHand[2][0] << "-" << myHand[3][0] << endl;
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
                MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, 1, MCW, &status); //receives the request for a spoon.
                cout << data << " wants a spoon! spoons left --> " << spoons << endl;
                int message = -1;
                if(spoons > 0){ // if there are spoons left.
                    cout << "Sending a spoon!" << endl;
                    MPI_Send(&message, 1, MPI_INT, data, 1, MCW); // player is sent a spoon.
                    if(spoons == numOfRounds){ // if first spoon was sent.
                        message = -2;
                        for(int i = 1; i < size - 1; i++){
                            cout << "Telling " << i << " that someone grabbed a spoon " << endl;
                            if(i != data) MPI_Send(&message, 1, MPI_INT, i, 1, MCW); //send message to all participants to start grabbing spoons.
                        }
                    }
                    spoons = spoons - 1; //reduce the number of spoons.
                    numAlive = numAlive - 1; // reduce the number of processors in play.
                } else {
                    message = -3;
                    MPI_Send(&message, 1, MPI_INT, data, 1, MCW);
                    break;
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
            
            } else {
                int isThereAMessage;
                bool iHaveASpoon = false;
                int newCard = -7;
                MPI_Status playerMessageStatus;
                vector<vector<int> > newHand;
                
                if(isDealer) {
                    newCard = deck[deck.size() - 1];
                }

                // get available messages
                MPI_Iprobe(0, MPI_ANY_TAG, MCW, &isThereAMessage, &status);
                while(isThereAMessage || newCard == -7) {
                    MPI_Recv(&newCard, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &playerMessageStatus);
                    if(newCard == -2) {
                        cout << "Rank: " << rank << " uh oh, someone grabbed a spoon" << endl;
                        break;
                    }
                    if(!isAlive) MPI_Send(&newCard, 1, MPI_INT, dest, 0, MCW);
                    MPI_Iprobe(0, MPI_ANY_TAG, MCW, &isThereAMessage, &status);
                }

                if(newCard != -2 && isDealer) {
                    newCard = deck[deck.size() - 1];
                    deck.pop_back();
                }

                // figure out what to do with my card
                if (newCard > 0) {
                    newHand = takeYourTurn(myHand, newCard);
                    vector<int> discardCard = newHand[newHand.size() - 1];
                    newHand.pop_back();
                    int discard = discardCard[0];
                    // cout << rank << " Discarding... " << discard << endl;
                    MPI_Send(&discard, 1, MPI_INT, dest, 0, MCW);
                }

                // grab a spoon if someone else has or if I have four of a kind
                if(newCard == -2 || newHand[0][1] == 4) {
                    if(newHand[0][1] == 4) {
                        cout << "I'm winning - " << rank << " + " << newHand[0][0] << "-" << newHand[1][0] << "-" << newHand[2][0] << "-" << newHand[3][0] <<  endl;
                    } else {
                        cout << "Someone grabbed a spoon" << endl;
                    }
                    int giveMeASpoon = rank;
                    MPI_Send(&giveMeASpoon, 1, MPI_INT, 0, 1, MCW); // ask rank 0 politely for a spoon.
                    cout << "Rank: " << rank << " asking for a spoon" << endl;
                    MPI_Recv(&giveMeASpoon, 1, MPI_INT, 0, 1, MCW, &status);
                    cout <<"Rank: " << rank <<  "Received a spoon --> " << giveMeASpoon << endl;
                    if(giveMeASpoon == -1) {
                        cout << "I, " << rank << " received a spoon" << endl;
                        iHaveASpoon = true;
                        break;
                    } else {
                        isAlive = false;
                        cout << rank << " I didn't get a spoon, I'm out!" << endl;
                        break;
                    }
                }

                myHand = newHand;
                if(iHaveASpoon) {
                    cout << "I have a spoon, breaking" << endl;
                    break;
                }
            }
        //-- round over --//
        }
    // }

    
    //=================//
    //=== GAME OVER ===//
    //=================//
    MPI_Barrier(MCW);
    if(isAlive && rank != 0){
        cout << "===========================" << endl;
        cout << "  Player #" << rank << " is the Spoons Champion!" << endl;
        cout << "===========================" << endl;
    }
    
    MPI_Finalize();
    return 0;
}
