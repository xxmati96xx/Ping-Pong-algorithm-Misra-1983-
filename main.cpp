//Mateusz Adler 146941
//Ping-Pong-algorithm-Misra-1983
//compile:
//mpic++ main.cpp
//
//run:
//mpirun -np <number of process> a.out

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <time.h> 
#include <condition_variable> 
using namespace std;

int ping = 1;
int pong = -1;
int m = 0;
int world_rank;
int world_size;
int MPI_PING = 0;
int MPI_PONG = 1;
pthread_t receive_Message_Event;
int messageReceive;
int next_process;
bool criticalSection = false;
mutex block,cv_m;
condition_variable cv;
unique_lock<mutex> ul(cv_m);
bool isPing = false; //true = lost Ping. Only one true
bool isPong = true; //true = lost Pong. Only one true
int precentLost = 20; //declare percent lost ping or pong

void receivePing(int value);
void receivePong(int value);
void regenerate(int value);
void incarnate(int value);
void *recieveMessage(void *arg);
void saveStatus(int value);
void sendPing(int ping);
void sendPong(int pong);

int main(int argc, char** argv) {
    srand( time( NULL ) );
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    messageReceive = pthread_create(&receive_Message_Event,NULL,recieveMessage,NULL);
    if(messageReceive){
        cout<<"Error reciever create. Process: "<<world_rank<<endl;
        MPI_Finalize();
    }

    next_process = (world_rank+1)%world_size;

    if(world_rank==0){
        sendPing(ping);
        sendPong(pong);
    }

    while (1)
    {
        if(criticalSection){
            cout<<"Enter critical section: Process: "<<world_rank<<endl;
            usleep(1000000);
            cout<<"Exit critical section: Process: "<<world_rank<<endl;
            block.lock();
            criticalSection = false;
            if(rand() % 100<precentLost && isPing){
                saveStatus(ping);
                cout<<"Ping lost. Process: "<<world_rank<<endl;
            }else{
                sendPing(ping);
                saveStatus(ping);
            }
            block.unlock();
        }else
        {
            cv.wait(ul);
        }
        
    }  
MPI_Finalize();
}

void receivePing(int value){
    if(abs(value)<abs(m)){
        cout<<"Old PING(delete). Process:"<<world_rank<<endl;
    }
    else {
        cout<<"Receive ping ["<<value<<"]. Process: "<<world_rank<<endl;
        block.lock();
        if(m==value){
            cout<<"Regenerate PONG. Process: "<<world_rank<<endl;
            regenerate(value);
            sendPong(pong);
            saveStatus(pong);
        }
        else if(m<value)
        {
            regenerate(value);
        }
        criticalSection = true;
        block.unlock();
        cv.notify_one();
    }
}

void receivePong(int value){
    if(abs(value)<abs(m)){
        cout<<"Old PONG(delete). Process:"<<world_rank<<endl;
    }
        else {
            cout<<"Receive pong ["<<value<<"]. Process: "<<world_rank<<endl;
           block.lock();
            if(criticalSection){
                cout<<"Incarnate. Process: "<<world_rank<<endl;
                incarnate(value);
            }else if(m==value)
            {
                cout<<"Regenerate PING. Process: "<<world_rank<<endl;
                regenerate(value);
                incarnate(ping);
                criticalSection = true;
       
            }else if(m>value)
            {
                regenerate(value);
            }
            cv.notify_one();
            if(rand() % 100<precentLost && isPong){
                saveStatus(pong);
                cout<<"Pong lost. Process: "<<world_rank<<endl;
            }else
            {
                saveStatus(pong);
                sendPong(pong);
            }
            block.unlock();
        }
}

void regenerate(int value){
    ping = abs(value);
    pong = -ping;
}

void incarnate(int value){
    ping = abs(value)+1;
    pong = -ping;
}

void saveStatus(int status){
    m = status;
}

void *recieveMessage(void *arg){
    while (1)
    {
        int value;
        MPI_Status stat;
        MPI_Recv(&value,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);

        if(stat.MPI_TAG == MPI_PING){
            receivePing(value);
        }
        if(stat.MPI_TAG == MPI_PONG){
            receivePong(value);
        }
    }
}

void sendPing(int ping){
    cout<<"Send ping ["<<ping<<"] from "<<world_rank<<" to "<<next_process<<endl;
    MPI_Send(&ping,1,MPI_INT,next_process,MPI_PING,MPI_COMM_WORLD);
}

void sendPong(int pong){
    usleep(10000);
    cout<<"Send pong ["<<pong<<"] from "<<world_rank<<" to "<<next_process<<endl;
    MPI_Send(&pong,1,MPI_INT,next_process,MPI_PONG,MPI_COMM_WORLD);
}