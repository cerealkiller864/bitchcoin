#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <openssl/sha.h>
#include "timeutils.hpp"
#include "coinutils.hpp"
#include "./Node.hpp"
#include "./Blockchain.hpp"



#define NODE_COUNT 15
#define MAX_TRANSACTION_SIZE_IN_BYTES 128 // 1KB of transactions data
#define REWARD 6.25
#define TOTAL = 21000000 
#define clrscr() std::cout << "\e[1;1H\e[2J"



// global variables
const uint16_t DIFFICULTY = 1000;
const uint16_t INITIAL_AMOUNT = 0;
float TRANSACTION_FEE;
float VALUE;
std::ifstream inputS;
std::ofstream outputS;
std::ofstream logS("./.log");
bool FINISHED = 0;



// determined threads to update bitcoin transaction fee and values every 30 seconds
std::thread subsystem(update, &TRANSACTION_FEE, &VALUE, &FINISHED);
// std::thread subSystem2(startNodejsProgram);



// judge
std::string whoisthewinner(std::string& correctHash)
{
    std::ifstream infile("./arena");
    std::string line, tmpHash, winner;

    while (std::getline(infile, line))
    {
        std::istringstream ss(line);
        ss >> tmpHash;

        if (tmpHash == correctHash)
        {
            FINISHED = 1;
            ss >> winner;
            break;
        }
    }

    infile.close();
    return winner;
}




/* IMPORTANT | DEFINE MEMBER FUNCTIONS FROM HEADER */

// constructor
Block::Block(std::string& td)
: transactionsData(td)
{
    correctHash = calcHash(transactionsData, DIFFICULTY);
    logS << "Block created successfully!" << std::endl;
    
}




// empty constructor
Block::Block()
{

    logS << "Block created successfully!" << std::endl;

}


// destructor
Block::~Block()
{             

    logS << "Block freed from memory successfully!" << std::endl;

}

//constructor
Blockchain::Blockchain()
{
    
    syncDatabase(); // read values from database
            
    logS << "Blockchain crated successfully!" << std::endl;

}

//destructor
Blockchain::~Blockchain()
{
    
    this->chain.clear();
    logS << "Blockchain deleted successfully!" << std::endl;

}

//methods
void Blockchain::openCompetition(std::string& leftOut, std::vector<Node*>& nl)
{
    ++last_index; // first and foremost

    std::unique_ptr<Block> dummyPtr(new Block(tmpTransactionsData));
    chain.push_back(std::move(dummyPtr));
    tmpTransactionsData.clear(); //reset tmpTransactionsData
    tmpTransactionsData += leftOut;

    // signal nodes to start competing in order to complete the creation of the newly initialized block
    for (auto& n : nl)
    {
        n->startMining(chain[last_index]->transactionsData);
    }

    std::cout << "\n --> All nodes have started mining\nWaiting for the correct hash to be generated" << std::endl;
    std::string winner;
    while (FINISHED == 0)
    {
        winner = whoisthewinner(chain[last_index]->correctHash);
    } 
        
    std::cout << "\n --> The correct hash has been generated by node : '" << winner << "' " << " | Winner will be rewarded with " << REWARD << " bitcoins" << std::endl;

    // sending reward
    for (auto& n : nl)
    {
        if (n->name == winner)
        {
            n->balance += REWARD;
        }
    }
    
    std::cout << "\n*Signaling miners to stop\n" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // waiting for miners to realize the competition is over 
    FINISHED = 0; // reset the boolean state for later use

    syncDatabase(1); // write changes to database
}



void Blockchain::syncDatabase(const uint16_t& mode)
{
    // mode 0 | read from database
    if (mode == 0)
    {
        std::ifstream filein("./database");
        std::string line;
        uint64_t linecount = 1;
        
        while (std::getline(filein, line))
        {
            ++last_index;

            std::unique_ptr<Block> dummyPtr(new Block());
            this->chain.push_back(std::move(dummyPtr));
            
            if (linecount % 2 != 0)
            {
                this->chain[last_index]->correctHash = line;    
            }

            else
            {
                this->chain[last_index]->transactionsData = line;    
            }

            ++linecount;
        }

        filein.close();
    }

    // mode 1 | write to database
    else if (mode == 1)
    {
        std::ofstream fileout("./database", std::ios::app);
        
        if (!fileout.is_open())
        {
            std::cout << "\nERROR! DO YOU HAVE THE RIGHT READ / WRITE PERMISSION FOR DATABASE" << std::endl;
            return;
        }

        fileout << this->chain[last_index]->correctHash << std::endl << this->chain[last_index]->transactionsData << std::endl;
        
        fileout.close();
    }

}



void Blockchain::showAllBlocks()
{
    
    std::cout << "\n----------------------------------------------------------------\n";
    
    for (uint64_t i = 0; i < this->chain.size(); i++)
    {

        std::cout << "Block number " << i+1 << ":" << std::endl;
        std::cout << "\t--> Hash of block : " << this->chain[i]->correctHash << std::endl;
        std::cout << "\t--> Transactions data :\n" << this->chain[i]->transactionsData << std::endl;
        std::cout << "----------------------------------------------------------------\n";

    }

}



//constructor
Node::Node(std::string& n, const float& b)
: name(n), balance(b)
{
    logS << "Node created successfully!" << std::endl;
}

// destructor
Node::~Node()
{
    logS << "Node deleted successfully!" << std::endl;
}

void Node::transferTo(Blockchain& blc, std::vector<Node*>& nl, std::string& receiver, float& amount, const uint32_t& timestamp)
{
    if (receiver == this->name)
    {
        std::cout << "\n->> Can't transfer money to yourserlf you fishing prick!" << std::endl;
        return;
    }

    else if ( (this->balance - amount) < 0 )
    {
        std::cout << "\n ->> Your total account balance isn't sufficient enough for this transfer : " << this->balance << " / " << amount << std::endl;
        return;
    }


    bool found = 0;
    // transfer money
    for (auto& n : nl)
    {
        if (n->name == receiver)
        {
            found = 1;
            n->balance += amount;
            this->balance -= amount;
        }
    }

    // if not found
    if (found == 0) 
    {
        std::cout << "Node doesn't exist!" << std::endl;
        return;
    }

    // write to public ledger
    std::string tmp = this->name + ' ' + receiver + ' ' + std::to_string(amount) + ' ' + std::to_string(timestamp) + '\n';

    if ( (blc.tmpTransactionsData.length() + tmp.length()) >= MAX_TRANSACTION_SIZE_IN_BYTES)
    {
        blc.openCompetition(tmp, nl);
    }
    // 
    else
    {
        blc.tmpTransactionsData += tmp;
    }
}

void Node::startMining(std::string& td)
{
    std::thread worker(guessHash, td, this->name, DIFFICULTY, &FINISHED);
    worker.detach();
}



void whereOurCoinsAt(std::vector<Node*>& nl)
{
    std::ifstream infile("./yourCoinsAreHere");
    if (!infile.is_open())
    {
        std::cout << "\n ->> ERROR! CAN'T GET ACCESS TO COINS BUCKET!" << std::endl;
        return;
    }

    std::string line;
    float tmp;
    uint64_t i = 0;

    while (std::getline(infile, line))
    {
        std::istringstream ss(line);
        ss >> tmp;

        nl[i]->balance = tmp;
        ++i;
    }

    infile.close();
    
}



void storeOurCoins(std::vector<Node*> &nl)
{
    std::ofstream outfile("./.tmpYourCoinsAreHere");

    if (!outfile.is_open())
    {
        std::cout << "\n ->> ERROR! DO YOU HAVE PROPER READ / WRITE PERMISSION FOR THIS DIRECTORY" << std::endl;
        return;
    }

    for (Node* &n : nl)    
    {
        outfile << n->balance << std::endl;
    }

    std::remove("./yourCoinsAreHere");
    std::rename("./.tmpYourCoinsAreHere", "./yourCoinsAreHere");

    outfile.close();
}



int main()
{
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // pause the main thread for 1s to wait for values to be updated;

    
    std::vector<Node*> nodesList; // a vector of NODES(ptr) as a method for the nodes to interact with one another
    Blockchain mychain;
    std::string tmpName;

    for (uint64_t i = 0; i < NODE_COUNT; i++)
    {
        tmpName = "node" + std::to_string(i+1);
        Node* dummyPtr = new Node(tmpName, INITIAL_AMOUNT);
        nodesList.push_back(dummyPtr);
    }
    
    Node* whoami = nodesList[random2(0, NODE_COUNT - 1)];
    std::cout << "You are now controlling " << whoami->name << " | You can change if you wish to\nPress enter to continue to program...";
    whoami->balance = 100; // give player 100 bitcons to start transferring

    std::cin.get();
    clrscr();

    char option;
    char option2;
    bool flag = 1;

    while (flag)
    {
        std::cout << "Queries | (t)ransfer, (n)ew node, (s)witch node, (q)uit" << std::endl;
        scanf(" %c", &option);

        switch (option)
        {
            case 'p':
                std::cout << "Print mode selected" << std::endl << "\nWhat to print? | (n)ame, (b)alance, transaction (f)ee, (v)alues, (t)emp TD, (a)ll blocks\n>>> ";
                scanf(" %c", &option2);
                //
                switch (option2)
                {
                    case 'a':
                        mychain.showAllBlocks();
                        break;

                        break;

                    case 'n':
                        std::cout << "\n --> Current node's name : " << whoami->name << std::endl;
                        break;

                    case 'b':
                        std::cout << "\n --> Current node's balance : " << whoami->balance << std::endl;
                        break;

                    case 'f':
                        std::cout << "\n --> Current transaction fee of bitcoin is : " << TRANSACTION_FEE << " [USD]" << std::endl;
                        break;
    
                    case 'v':
                        std::cout << "\n --> Current bitcoin's value is : " << std::endl;
                        printAllValues();
                        break;

                    case 't':
                        std::cout << "\n --> Current temporary ledger data : \n" << mychain.tmpTransactionsData << std::endl;
                        break;

                    default:
                        std::cout << "\n --> Inavlid option : " << option2 << std::endl;
                        break;
                } //

                break;

            case 't':
                float tmpAmount;
                std::cout << "Enter the name of the receiver:\n>>> ";
                std::cin >> tmpName; std::cin.ignore();
                std::cout << "Enter amount of bitcoin to transfer:\n>>> ";
                std::cin >> tmpAmount; std::cin.ignore();
                whoami->transferTo(mychain, nodesList, tmpName, tmpAmount, time(0));

                //debug
                // std::cout << "\nTemporary transaction data : " << chain.tmpTransactionsData << std::endl;
                break;

            case 'n':
                std::cout << "Enter name of new node :\n>>> ";
                std::cin >> tmpName; std::cin.ignore();
                break;

            case 's':
                std::cout << "Enter the name of the node you want to control:\n>>> ";
                std::cin >> tmpName; std::cin.ignore();

                if (tmpName == whoami->name)
                {
                    std::cout << " ->> You are already controlling '" <<tmpName << "' | Sigh... I know ADHD is hard for you.\n" << std::endl;
                    break;
                }

                for (auto n : nodesList)
                {

                    if (tmpName == n->name)
                    {
                        whoami = n;
                    } 
                }                
                break;

            case 'q':
                std::cout << "\n--> Terminating!" << std::endl; // break out of switch - case statement
                flag = 0;
                break;
            
            case 'c':
                clrscr();
                break;

            default:
                std::cout << "Invalid option : " << option << std::endl;
                break;
        }
    }

    // memory cleaning
    for (Node* &nPtr : nodesList)
    {
        delete nPtr;
    } nodesList.clear();

    // kill nodejs fetcher
    FINISHED = 1;
    subsystem.join();

    // save changes to databases
    storeOurCoins(nodesList);   

    return 0;
}



