#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

#define MCW MPI_COMM_WORLD 

using namespace std;

vector<int> draw(vector<int> hand, vector<int> deck){
	hand.push_back(1);
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 4; j++){
			if(i != j){
				int quantity = 0;
				if(hand[i] == hand[j]){
					quantity = quantity + 1;
				}
			}
		}
	}		
	return hand;	
}


int main(int argc, char  **argv){


	//=================//
	//=== VARIABLES ===//
	//=================//

	int rank, size, spoons, playerFinished;
	vector<int> myHand;
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
	if(rank == 1){ //rank 1 to be first dealer.
		isDealer = true;
	}
	
	int numOfRounds = size - 3;


	//=================//
	//=== GAME LOOP ===//
	//=================//

	for(numOfRounds; numOfRounds > 0; numOfRounds--){
		
		// -- initialize round -- //
		if(rank == 0){
			spoons = 
		}

		// -- during round -- //
		while(!roundFinished){ 
		
			// rank 0 is the pile of spoons and handles when a player requests a spoon and when a round ends.
			if(rank == 0){
				MPI_Iprobe(); // check if a player grabs a spoon.
				if(playerFinished > 0 && spoons > 0){
					MPI_Send(playerFinished);
					spoons = spoons - 1;	
					playerFinished = 0; // exit condition
				}
				else{	
										
					roundFinished = true;
				}
			}
			if(isDealer){
				
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
