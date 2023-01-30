#include <iostream>
#include <string>
#include <mutex>
#include <sstream>
#include <vector>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <algorithm>
#include <iomanip>

using namespace std;

int customerNumber; // variable to hold the total number of customers

// cumulative balance of the each company
int K = 0;
int B = 0;
int S = 0;
int O = 0;
int D = 0;

pthread_mutex_t vendingMachine[10]; // mutex locks representing shared data of vending machines
pthread_mutex_t customerWaiting[10]; // mutex locks representing customer queue
pthread_mutex_t vendingMachineWaiting[10]; // mutex lock representing availability of vending machines
pthread_mutex_t company[5]; // mutex lock for balance control of each company
pthread_mutex_t varMutex; // mutex lock for current prepayment info
pthread_mutex_t outputFile; // mutex lock for printing ouput file
ofstream MyFile; // declare outpuf file

// prepayment struct holds the required data for each prepayment operation
struct prepayment{
    int custID;
    int machinID;
    int companyID;
    int amount;
    
};

// array to hold all current prepayment operations in the process
struct prepayment prepayments[10];

// customer thread function
// it takes an integer vector as a parameter which is holding the information about the prepayment
void *sendInfo(void *param){
    std::vector<int> paymentData = *((std::vector<int> *)param);

    int sleepTime = paymentData[1];

    // customer thread sleeps for given milliseconds
    usleep(sleepTime * 1000);

    // create a prepayment struct object to append prepayments array
    struct prepayment currentPrepayment;
    
    // lock varMutex to read prepayment data seperately
    pthread_mutex_lock(&varMutex);    
    int machID = paymentData[2];
    currentPrepayment.custID = paymentData[0]+1;
    currentPrepayment.machinID = machID-1;
    currentPrepayment.companyID = paymentData[3];
    currentPrepayment.amount = paymentData[4];
    pthread_mutex_unlock(&varMutex);

    // lock current vending machine to prevent multiple operations concurrently
    pthread_mutex_lock(&vendingMachine[(currentPrepayment.machinID)]);

    // pass the prepayment information to the vending machine
    prepayments[(currentPrepayment.machinID)].custID = currentPrepayment.custID;
    prepayments[(currentPrepayment.machinID)].companyID = currentPrepayment.companyID;
    prepayments[(currentPrepayment.machinID)].amount = currentPrepayment.amount;
    
    // customer leaves the queue to allow next customer to use vending machine
    pthread_mutex_unlock(&customerWaiting[currentPrepayment.machinID]);
    
    // waiting for vending machine to do prepayment operations
    pthread_mutex_lock(&vendingMachineWaiting[currentPrepayment.machinID]);
    
    pthread_mutex_unlock(&vendingMachine[currentPrepayment.machinID]);
    
    // exit the thread
    pthread_exit(NULL);

}

// vending machine thread function
// it takes an int value as a parameter which represents the operating machine id
void *makePrepayment(void *param){

    int machineID = *((int *)param);

    while(true){
        
        pthread_mutex_lock(&customerWaiting[machineID-1]);

        // check for the company id and add prepayment amount to the balance
        string compName;
        if (prepayments[machineID-1].companyID == 1){
            pthread_mutex_lock(&company[0]);
            K += prepayments[machineID-1].amount;
            compName = "Kevin";
            pthread_mutex_unlock(&company[0]);
        }
        if (prepayments[machineID-1].companyID == 2){
            pthread_mutex_lock(&company[1]);
            B += prepayments[machineID-1].amount;
            compName = "Bob";
            pthread_mutex_unlock(&company[1]);
        }
        if (prepayments[machineID-1].companyID == 3){
            pthread_mutex_lock(&company[2]);
            S += prepayments[machineID-1].amount;
            compName = "Stuart";
            pthread_mutex_unlock(&company[2]);
        }
        if (prepayments[machineID-1].companyID == 4){
            pthread_mutex_lock(&company[3]);
            O += prepayments[machineID-1].amount;
            compName = "Otto";
            pthread_mutex_unlock(&company[3]);
        }
        if (prepayments[machineID-1].companyID == 5){
            pthread_mutex_lock(&company[4]);
            D += prepayments[machineID-1].amount;
            compName = "Dave";
            pthread_mutex_unlock(&company[4]);
        }

        // lock the log file to prevent concurrent writing
        pthread_mutex_lock(&outputFile);
        MyFile << "Customer" << prepayments[machineID-1].custID << "," << prepayments[machineID-1].amount << "TL," << compName << endl;
        pthread_mutex_unlock(&outputFile);

        // vending machine is done
        pthread_mutex_unlock(&vendingMachineWaiting[machineID-1]);
               
    }   
        // exit the thread
        pthread_exit(NULL); 
}

int main(int argc, char* argv[]){
    string fileName = argv[1];
    istringstream mys(fileName);
    string outName;
    getline(mys, outName, '.');
    MyFile.open(outName + "_log.txt"); // open the log file with input file name

    std::ifstream Read(argv[1]);
    std::string line;
    getline(Read, line);
    customerNumber = std::stoi(line); // read the first line and initialize the customer number
    vector<string> fileLines;
    while(getline(Read,line)){
        fileLines.push_back(line); // read every line and add to the file lines vector
}

    // initialize mutexes as NULL
    pthread_mutex_init(&outputFile, NULL);
    pthread_mutex_init(&varMutex, NULL);

    for (int i = 0; i < 10; i++){
        pthread_mutex_init(&vendingMachine[i], NULL);
        pthread_mutex_init(&vendingMachineWaiting[i], NULL);
        pthread_mutex_init(&customerWaiting[i], NULL);

        // initially there is no waiting customer or operating machine
        pthread_mutex_lock(&vendingMachineWaiting[i]);
        pthread_mutex_lock(&customerWaiting[i]);
    }
    for (int i = 0; i < 5; i++){
        pthread_mutex_init(&company[i],NULL);
    }

    // declare machine threads and customer threads
    pthread_t machineThreads[10];
    pthread_t customerThreads[customerNumber];

    int mThreadCounter[10]; // int array to hold machine id of current operating machine in the thread

    // create machine threads
    for (int i = 0; i < 10; i++) {
        mThreadCounter[i] = i+1;
        pthread_create(&machineThreads[i], 0, makePrepayment, &mThreadCounter[i]);
    }

    vector <int> datavector[customerNumber]; // array of vectors that are keeping prepayment data

    // read the required data of each customer from input file and keep them in datavector
    // data vector contains customer id, sleep duration, machine id, company id and prepayment amount in order
    for (int i = 0; i < customerNumber; i++){

        string line = fileLines[i];
        istringstream myss(line);

        vector<int> data;
    
        string sleep;
        getline(myss, sleep, ',');
        int sleepTime = std::stoi(sleep);
        
        string mach;
        getline(myss, mach, ',');
        int machID = std::stoi(mach);
        
        string comp;
        getline(myss, comp, ',');
        
        string amnt;
        getline(myss, amnt, ',');
        int amount = std::stoi(amnt);

        data.push_back(i);
        data.push_back(sleepTime);
        data.push_back(machID);

        if(comp =="Kevin"){
            data.push_back(1);
        }
        else if(comp =="Bob"){
            data.push_back(2);
        }
        else if(comp =="Stuart"){
            data.push_back(3);
        }
        else if(comp =="Otto"){
            data.push_back(4);
        }
        else if(comp =="Dave"){
            data.push_back(5);
        }

        data.push_back(amount);
        datavector[i] = data;

        pthread_create(&customerThreads[i], 0, sendInfo, &datavector[i]);
    }

    // create customer threads
    for (int i = 0; i < customerNumber; i++){
        pthread_join(customerThreads[i], NULL);
    }
    
    // main thread writes total amounts to end of the log file
    MyFile << "All prepayments are completed" << endl;
    MyFile << "Kevin: " << K << endl;
    MyFile << "Bob: " << B << endl;
    MyFile << "Stuart: " << S << endl;
    MyFile << "Otto: " << O << endl;
    MyFile << "Dave: " << D << endl;

    return 0;
}