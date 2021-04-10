#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <math.h>

#define MCW MPI_COMM_WORLD 

using namespace std;


// sort 2d vector by second(1th) element
bool sortVectorFunc(vector<int> x, vector<int> y) {
	return x[1] > y[1]; 
}

// this takes a hand and a new card and returns the new hand sorted by quantity of each card
// this assumes hand is 2d vector
// returns the hand, assumption is that you would discard the last element (send it to the next player/process)
vector<vector<int> > takeYourTurn(vector<vector<int> > hand, int newCard) {
	int goingFor = hand[0][0];
	int quantity = hand[0][1];
	int newCardQuantity = 1;
	for(int i = 0; i < hand.size(); i++){
		if(hand[i][0] == newCard) {
			newCardQuantity += 1;
			if(newCardQuantity > quantity) {
				goingFor = newCard;
				quantity = newCardQuantity;
				break;
			}
		}
	}
    vector<int> newCardVec = (newCard, newCardQuantity);
	hand.push_back(newCardVec);
	sort(hand.begin(), hand.end(), sortVectorFunc);
	return hand;
}

vector<vector<int> > assessHand(vector<vector<int> > myHand) {
	for(int i = 0; i < myHand.size() - 1; i++) {
		for(int j = 0; j < myHand.size(); j++) {
			if(i != j) {
				if(myHand[i][0] == myHand[j][0]) {
					myHand[i][1] += 1;
					myHand[j][1] += 1;
				}
			}
		}
	}
	sort(myHand.begin(), myHand.end(), sortVectorFunc);
	return myHand;
}

// randomizes a vector of integers and returns a vector of the same size, but different order.
vector<int> shuffle(vector<int> deck){
    vector<int> ret;
    int start = rand(deck.size());
    for(int i = 0; i < deck.size(); i++){
        int j = start + i;
        if(j > deck.size()-1){
            j = i - start;
        }
        ret[i] = deck[j];
    }
    return ret;
}

int main(int argc, char  **argv){
	srand(time(NULL));

	//=================//
	//=== VARIABLES ===//
	//=================//

	int rank, size, spoons, numDeck, tag, data;
	vector<vector<int> > myHand;
    vector<int> deck;
	bool isDealer, isTurn, done, roundFinished = false;
	bool isAlive = true;	

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MCW, &rank);
	MPI_Comm_size(MCW, &size);

	//===================//
	//=== SET UP GAME ===//
	//===================//

	if(size < 2){ 
		cout << "Need at least 2 players to play the game...(Minimum 3  processors required: 2 players, 1 to control spoon pile)" << endl;
		return 0;
	}
    if(size > 13){ // how many decks in play at the start.
        numDeck = ceil((size-1)/13);
    }
    else{numDeck = 1;}
	if(rank == 1){ // rank 1 to be first dealer.
		isDealer = true;
	}
	int numOfRounds = size - 2; // initial number of rounds [min 1 round for 2 player processors and the GM processor]
    
	// each process picks their first 4 cards from their own deck (because it's simple)
	vector<vector<int> > initialHand;
	for(int i = 0; i < 4; i++) {
		vector<int> newCard = (deck.pop_back(), 1);
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

	for(numOfRounds; numOfRounds > 0; numOfRounds--){
		
		// -- initialize round -- //
		if(rank == 0){
			spoons = numOfRounds;  // number of spoons in play
            int numAlive = spoons + 1; // number of current players, only GM needs access to this variable.
		}
        
		// -- during round -- //
		while(!roundFinished){  // exit condition.
            
            //------------------//
            //---GAME MANAGER---//
            //------------------//
			// Responsibilities:
            // - GM gives spoons upon request
            // - GM keeps track of how many spoons are in play and when to quit the round
            // - GM broadcasts when a spoon has been taken
            // - Tells a player when they have lost
			if(rank == 0){
				MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &tag, MPI_STATUS_IGNORE); // check if a player wants to grab a spoon.
                if(tag == 0){ // someone wants a spoon;
                    MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW); //receives the request for a spoon
                    if(data > 0 && spoons > 0){ // if there are spoons left.
                        MPI_Send(1, 1, MPI_INT, data, 1, MCW) // player is sent a spoon.
                        if(){
                            MPI_Bcast(-2, 1, MPI_INT, 0, MCW)
                            }
                        spoons = spoons - 1;	 // reduce the number of spoons
                        numAlive = numAlive - 1; // exit condition
                    }
                    else {
                        roundFinished = true;
                        MPI_Send(-1, 1, MPI_INT, data, 1, MCW) // sends you lost message to player without a spoon.
                    }
                    tag = 0;  // reset tag
                    data = 0; // resets data
                }
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
            
            if(!rank){ // if not rank zero.
                if(isAlive){ // if a player is still in the game either playing or waiting for the next round to start.
                    if(!done){ // if a player does not yet have a spoon, but is still playing the game.
                        MPI_IProbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, &tag, MPI_STATUS_IGNORE); // check if the other players grabbed a spoon.
                        if(tag) { // if message is waiting
                            MPI_Recv(&data, 1, MPI_INT, MCW, MPI_ANY_TAG, MPI_STATUS_IGNORE);
                            if(data == -2){
                                int giveMeASpoon = 0;
                                MPI_Send(&giveMeASpoon, 1, MPI_INT, 0, 0, MCW); // ask rank 0 politely for a spoon.
                                MPI_Recv(&giveMeASpoon, 1, MPI_INT, 0, MPI_ANY_TAG, MCW);
                                if(!giveMeASpoon) {
                                    isALive = false;
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
                                newCard = deck.pop_back();
                            }
                            else {
                            // get new card from previous player
                                MPI_Recv(&newCard)
                                if(!isAlive) {
                                // automatically send card along
                                    MPI_Send(&newCard, 1, MPI_INT, dest, 0, MCW);
                                    continue; // process is not alive, don't do anything else
                                }
                            }
                            vector<vector<int> > newHand = takeYourTurn(hand, newCard);
                            vector<int> discardCard = newHand.pop_back();
                            int discard = discardCard[0];
                            if(newHand[0][1] >= 4 ){
                                // grab a spoon
                            }
                            // send discarded card along to next process
                            MPI_Send(&discard, 1, MPI_INT, dest, 0, MCW);
                        }
                    }
                }
		//-- round over --//
		MPI_Barrier(MCW);
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
