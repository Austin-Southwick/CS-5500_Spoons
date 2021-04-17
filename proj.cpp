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

// fill decks with 4 cards per deck. cards go from 1 - 13 [Ace(1), 2, 3, ... 10, Jack(11), Queen(12), King(13)]
vector<int> initializeDeck(int numOfDecks) {
    vector<int> deck;
    for(int i = 0; i < numOfDecks * 4; i++) {
        int k = 1;
        for(int j = k; j < 14; j++) { //
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

    int rank, size, data, flag, numOfDecks, spoons, numAlive;
    vector<vector<int> > myHand;
    vector<int> deck;
    bool isDealer, roundFinished, firstDraw = false;
    bool isAlive = true;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank);
    MPI_Comm_size(MCW, &size);
    MPI_Status status;

    srand(rank);
    
    // Ring Logic
    int prevPlayer = rank - 1;
    if(prevPlayer == 0){prevPlayer = size - 1;}
    int potentialDest = (rank + 1) % size;
    int dest = potentialDest == 0 ? potentialDest + 1 : potentialDest;
   
    MPI_Barrier(MCW);
    
    for(int roundNumber = 1; roundNumber < size - 2; roundNumber++)
    {
        //Determine whos turn to deal and give everyone their starting hands.
        if(rank != 0 && rank != roundNumber){
            isDealer = false;
            for(int i = 0; i < 4; i++){
                MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MCW, MPI_STATUS_IGNORE);
                vector<int> temp2;
                temp2.push_back(data);
                temp2.push_back(1);
                myHand.push_back(temp2);
                temp2.clear();
            }
            myHand = assessHand(myHand);
        }
        if(rank == roundNumber){ // Dealer for the round.
            isDealer = true;
            deck = initializeDeck(1); // 4 more cards than players always in the deck.
            for(int j = 1; j < size; j++){
                for(int i = 0; i < 4; i++){
                    int pass = deck.back();
                    deck.pop_back();
                    if(rank != j){
                        MPI_Send(&pass, 1, MPI_INT, j, 1, MCW);
                    }
                    else{
                        vector<int> temp;
                        temp.push_back(pass);
                        temp.push_back(1);
                        myHand.push_back(temp);
                        temp.clear();
                    }
                }
            }
            myHand = assessHand(myHand);
        }
        else{ // rank 0
            numAlive = size - roundNumber; // number of current players, only GM needs access to this variable.
            spoons = numAlive - 1;         // number of spoons available at the start of the round, only GM needs access to this variable.
        }
        data = 0;
        MPI_Barrier(MCW);
        
        while(roundFinished == 0){
        
        }
        // Round over
        isDealer = false;
        myHand.clear(); //gets rid of current hand
        sleep(1);
    }// end of roundNumber for loop
    
    // Game Over
    
    MPI_Barrier(MCW);
    if(isAlive == true && rank != 0){
        cout << "-----------------------------------" << endl;
        cout << "I rank #" << rank << " am the champion!" << endl;
        cout << "-----------------------------------" << endl;
    }
    MPI_Finalize();
    return 0;
}
