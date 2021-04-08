#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

#define MCW MPI_COMM_WORLD 

using namespace std;

// morphed this function into the assess hand function
// vector<int> draw(vector<int> hand, vector<int> deck){
// 	hand.push_back(1);
// 	for(int i = 0; i < 3; i++){
// 		for(int j = 0; j < 4; j++){
// 			if(i != j){
// 				int quantity = 0;
// 				if(hand[i] == hand[j]){
// 					quantity = quantity + 1;
// 				}
// 			}
// 		}
// 	}		
// 	return hand;	
}

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
	vector<int> newCard = (newCard, newCardQuantity)
	hand.push_back(newCard);
	sort(hand.begin(), hand.end(), sortVectorFunc);
	return hand;
}

vector<vector<int> > assessHand(vector<vector<int> > myHand) {
	for(int i = 0; i < myHand.size() - 1; i++) {
		for(int j = 0; j < h=myHand.size(); j++) {
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

int main(int argc, char  **argv){
	srand(time(NULL));

	//=================//
	//=== VARIABLES ===//
	//=================//

	int rank, size, spoons, playerFinished;
	vector<vector<int> > myHand;
	vector<int> deck;
	bool isDealer, isTurn, roundFinished = false;
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
	if(rank == 1){ // rank 1 to be first dealer.
		isDealer = true;
	}
	
	int numOfRounds = size - 3;

	// initialize deck
	for(int i = 0; i < 52; i++) {
		deck.push_back(rand() % 13);
	}
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
	// Tag = -1 (Someone grabbed a spoon)
	// Tag = 1 (Someone has won the game just quit)
	//

	for(numOfRounds; numOfRounds > 0; numOfRounds--){
		
		// -- initialize round -- //
		if(rank == 0){
			spoons = numOfRounds;
		}
		playerFinished = numOfRounds + 1;
		// -- during round -- //
		while(!roundFinished){ 
		
			// rank 0 is the pile of spoons and handles when a player requests a spoon and when a round ends.
			if(rank == 0){
				MPI_Iprobe(); // check if a player grabs a spoon.
				if(playerFinished > 0 && spoons > 0){
					MPI_Send(playerFinished); // this needs to be sent to all players so they can grab a spoon too.
					if(playerFinished == numOfRounds) {
						MPI_Scatter('Grab a spoon ya slowPokes');
					}
					spoons = spoons - 1;	
					playerFinished = playerFinished - 1; // exit condition
				}	else {				
					roundFinished = true;
				}
			}

			int grabASpoon = 0;
			MPI_IProbe(grabASpoon); // check if the other players grabbed a spoon.
			if(grabASpoon) {
				int giveMeASpoon = 0;
				MPI_Send(&giveMeASpoon, 1, MPI_INT, 0, 0, MCW); // ask rank 0 politely for a spoon.
				MPI_Recv(&giveMeASpoon, 1, MPI_INT, 0, MPI_ANY_TAG, MCW);
				if(!giveMeASpoon) {
					isALive = false;
				}
			}

			int potentialDest = (rank + 1) % size
			int dest = potentialDest == 0 ? potentialDest + 1 : potentialDest;
			int newCard;
			if(isDealer){
				// draw a card from the deck
				newCard = deck.pop_back();
			} else {
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
