//
// Created by kennard on 07/10/18.
//
#include <openmpi/mpi.h>
#include <queue>
#include <list>
#include <mpi.h>

using namespace std;

int main(void) {
    const unsigned int STATUS_NOT_PROCESSING = 0;
    const unsigned int STATUS_IS_PROCESSING = 2;
    const unsigned int NUM_LINES = 3;

    srand(static_cast<unsigned int>(time(NULL)));
    MPI_Comm centralComm;

    int info[3] = {-1,-1,-1};

    MPI_Init(NULL, NULL);
    MPI_Comm_get_parent(&centralComm);
    int worldRank, childRank;

    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_rank(centralComm, &childRank);

    // Receive the number of station sides
    unsigned int numStationSides(0);
    MPI_Bcast(&numStationSides, 1, MPI_UINT32_T, 0, centralComm);
    // cout << childRank << " " << numStationSides << endl;
    // Get the node info. Different semantic meaning depending on whether it is a link or a stationSide. If it is
    // a link, then it represents (startStationSide, endStationSide, cost). Otherwise, it represents (link[green],
    // link[yellow], link[blue]), link[green] represents the rank of the link in the intracomm that will green
    // trains passing the station will go to.
    unsigned char isLink(worldRank >= numStationSides);
    MPI_Recv(info, 3, MPI_INT, 0, 0, centralComm, NULL);

    int prevNodes[3] = {-1,-1,-1};
    if (!isLink) {
        MPI_Recv(prevNodes, 3, MPI_INT, 0, 0, centralComm, NULL);
    }

    // Receive the number of ticks
    unsigned int numTicks;
    MPI_Bcast(&numTicks, 1, MPI_UINT32_T, 0, centralComm);
    MPI_Barrier(centralComm);

    /*
    bool isTerminal(false);
    float stationPopularity(0);
    list<pair<unsigned int, unsigned int>> beforeProcessing;

    if (!isLink) {
        // the station is a terminal if at least one of its nodes is another station side.
        for (unsigned int i = 0; i < 3; i++) {
            isTerminal |= info[i] < numStationSides;
        }

        MPI_Recv(&stationPopularity, 1, MPI_FLOAT, 0, 0, centralComm, NULL);
        // Populate if it is a starting station
        unsigned int numTrainsFromCentral(0);
        MPI_Recv(&numTrainsFromCentral, 1, MPI_UINT32_T, 0, 0, centralComm, NULL);


        if (numTrainsFromCentral > 0) {
            unsigned numTrainDataFromCentral = numTrainsFromCentral * 2;
            unsigned int *trainsFromCentral = (unsigned int *) malloc(sizeof(unsigned int) * numTrainDataFromCentral);
            // MPI_Recv(&trainsFromCentral, numTrainsFromCentral * 2, MPI_UINT32_T, 0, 0, centralComm, NULL);

            for (unsigned int i = 0; i < numTrainsFromCentral; i++) {
                beforeProcessing.push_back(
                        pair<unsigned int, unsigned int>(trainsFromCentral[i*2], trainsFromCentral[i*2 + 1]));
            }

            free(trainsFromCentral);
            trainsFromCentral = NULL;
        }
    }*/
    //MPI_Barrier(centralComm);

    /*
    float countDown(0);
    unsigned int processedTrainInfo[2];
    unsigned int receivedTrainInfo[6];
    unsigned int isProcessing = STATUS_NOT_PROCESSING;

    unsigned int totalWaitTime[NUM_LINES] = {0};
    unsigned int minWaitTime[NUM_LINES] = {0};
    unsigned int maxWaitTime[NUM_LINES] = {0};
    unsigned int numTrainsVisited[NUM_LINES] = {0};
    unsigned int prevTimeStamp[NUM_LINES] = {0};

    for (unsigned int t = 0; t < numTicks; t++) {
        bool hasTrainToSend(false);
        unsigned char hasTrainToReceive(0);

        cout << t << endl;
        // if has previously finished processing
        if (isProcessing == STATUS_IS_PROCESSING && countDown <= 0) {
            isProcessing = STATUS_NOT_PROCESSING;
            pair<unsigned int, unsigned int> processedTrain = beforeProcessing.front();
            processedTrainInfo[0] = processedTrain.first;
            processedTrainInfo[1] = processedTrain.second;

            // If station, update wait times
            if (!isLink) {
                prevTimeStamp[processedTrain.first] = t;
            }

            beforeProcessing.pop_front();
            hasTrainToSend = true;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        cout << t << " te" << endl;

        if (isLink) {
            MPI_Send(&hasTrainToSend, 1, MPI_UNSIGNED_CHAR, isLink ? info[1] : info[processedTrainInfo[0]],
                     0, MPI_COMM_WORLD);
            if (hasTrainToSend) {
                MPI_Send(processedTrainInfo, 2, MPI_UINT32_T, info[1], 0, MPI_COMM_WORLD);
            }
        } else {
            if (isTerminal) {
                for (unsigned int i = 0; i < NUM_LINES; i++) {
                    if ((i == processedTrainInfo[i]) && hasTrainToSend) {
                        MPI_Send(processedTrainInfo, 2, MPI_UINT32_T, info[1], 0, MPI_COMM_WORLD);
                    }
                }
            }

            // Notice that a station can have a maximum in-degree of 3 links. Hence, can receive up to 3 trains
            // for each link
            MPI_Recv(&hasTrainToReceive, 1, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
            if (hasTrainToReceive) {
                MPI_Recv(receivedTrainInfo, 2, MPI_INT32_T, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
                beforeProcessing.push_back(pair<unsigned int, unsigned int>(receivedTrainInfo[0],receivedTrainInfo[1]));
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);

        cout << t << "fe" << endl;


        if (isLink || isTerminal) {
            MPI_Recv(&hasTrainToReceive, 1, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
            if (hasTrainToReceive) {
                MPI_Recv(receivedTrainInfo, 2, MPI_INT32_T, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
                beforeProcessing.push_back(pair<unsigned int, unsigned int>(receivedTrainInfo[0],receivedTrainInfo[1]));
            }
        } else {
            MPI_Send(&hasTrainToSend, 1, MPI_UNSIGNED_CHAR, info[processedTrainInfo[0]], 0, MPI_COMM_WORLD);
            if (hasTrainToSend) {
                MPI_Send(processedTrainInfo, 2, MPI_UINT32_T, info[1], 0, MPI_COMM_WORLD);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);


        cout << t << " we" << endl;

        // Actual processing
        if ((isProcessing == false) && !beforeProcessing.empty()) {
            if (isLink) {
                countDown = info[2];
            } else {
                countDown = stationPopularity * ((rand() % 10) + 1);
                unsigned int processedTrainLineIdx = beforeProcessing.front().first;
                unsigned int waitTime = t - prevTimeStamp[processedTrainLineIdx];
                totalWaitTime[processedTrainLineIdx] += waitTime;
                minWaitTime[processedTrainLineIdx] = min(minWaitTime[processedTrainLineIdx], waitTime);
                maxWaitTime[processedTrainLineIdx] = max(maxWaitTime[processedTrainLineIdx], waitTime);
                numTrainsVisited[processedTrainLineIdx]++;
            }

            isProcessing = STATUS_IS_PROCESSING;
        } else if (isProcessing) {
            countDown -= 1;
        }
        MPI_Barrier(MPI_COMM_WORLD);

        // Update master process.
        unsigned int numTrains(beforeProcessing.size());
        MPI_Send(&numTrains, 1, MPI_UINT32_T, 0, 0, centralComm);
        MPI_Barrier(centralComm);

        unsigned int *holdingTrains = static_cast<unsigned int *>(malloc(sizeof(unsigned int) * numTrains * 2));
        unsigned int idx(0);
        for (list<pair<unsigned int, unsigned int>>::iterator it = beforeProcessing.begin();
             it != beforeProcessing.end(); it++) {
            holdingTrains[idx++] = it->first;
            holdingTrains[idx++] = it->second;
        }

        if (numTrains > 0) {
            MPI_Send(holdingTrains, numTrains * 2, MPI_UINT32_T, 0, 0, centralComm);
        }
        MPI_Barrier(centralComm);
        cout << 1 << endl;
        free(holdingTrains);
        holdingTrains = NULL;
    }


    // update master process with values
    if (!isLink) {
        const unsigned int NUM_DATA_TO_SEND_PER_LINE = 4;
        unsigned int waitData[NUM_DATA_TO_SEND_PER_LINE * NUM_LINES];
        for (unsigned int i = 0; i < NUM_LINES; i++) {
            waitData[i * 4] = totalWaitTime[i];
            waitData[i * 4 + 1] = minWaitTime[i];
            waitData[i * 4 + 2] = maxWaitTime[i];
            waitData[i * 4 + 3] = numTrainsVisited[i];
        }

        MPI_Send(&waitData, NUM_DATA_TO_SEND_PER_LINE * NUM_LINES, MPI_UINT32_T, 0, 0, centralComm);
    }
    MPI_Barrier(centralComm);*/
    MPI_Finalize();

    return EXIT_SUCCESS;
}
