#include <iostream>
#include <openmpi/mpi.h>
#include <vector>
#include <sstream>
#include <list>
#include <mpi.h>
#include <iomanip>

using namespace std;

const unsigned int NUM_LINES = 3;

void getStationNames(vector<string> &stationNames) {
    string rawStationNames;
    cin >> rawStationNames;

    char delimiter = ',';
    stringstream ss(rawStationNames);
    string stationName;
    while (getline(ss, stationName, delimiter)) {
        stationNames.push_back(stationName);
    }
}

void splitStringUsingDelimiter(string &rawString, char delimiter, vector<string> &result)
{
    stringstream ss(rawString);
    string token;
    while (getline(ss, token, delimiter)) {
        result.push_back(token);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));
    const string finalLinePrefixes[3] = {"green", "yellow", "blue"};
    const char linePrefixes[3] = {'g', 'y', 'b'};
    const char* linkFormat = "%c%u-s%u->s%u";
    const char* stationFormat = "%c%u-s%u";

    ///////////////////
    // Read Inputs ///
    /////////////////
    unsigned int numStations;
    cin >> numStations;

    vector<string> stationNames;
    getStationNames(stationNames);
    map<string, unsigned int> stationNamesToStationId;
    for (unsigned int i = 0; i < numStations; i++) {
        stationNamesToStationId.insert(pair<string, unsigned int>(stationNames[i], i));
    }

    unsigned int** linkCosts = (unsigned int**) malloc(sizeof(unsigned int*) * numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linkCosts[i] = (unsigned int*) calloc(numStations, sizeof(unsigned int));
        for (unsigned int j = 0; j < numStations; j++) {
            cin >> linkCosts[i][j];
        }
    }

    vector<float> stationPopularities(numStations);
    {
        string rawStationPopularity;
        cin >> rawStationPopularity;
        vector<string> stationPopularityStrings;
        splitStringUsingDelimiter(rawStationPopularity, ',',stationPopularityStrings);
        for (unsigned int i = 0; i < numStations; i++) {
            stationPopularities[i] = stof(stationPopularityStrings[i]);
        }
    }

    vector<vector<unsigned int>> line_stationIds;
    line_stationIds.resize(NUM_LINES);
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        vector<string> stationsInLine;
        getStationNames(stationsInLine);
        line_stationIds[i].resize(stationsInLine.size());
        for (unsigned int j = 0; j < stationsInLine.size(); j++) {
            unsigned int stationId = stationNamesToStationId[stationsInLine[j]];
            line_stationIds[i][j] = stationId;
        }
    }

    unsigned int numTicks;
    cin >> numTicks;

    unsigned int numTrainsPerLine[NUM_LINES];
    {
        string rawTrainsPerLineString;
        cin >> rawTrainsPerLineString;
        vector<string> trainsPerLineString;
        splitStringUsingDelimiter(rawTrainsPerLineString, ',', trainsPerLineString);
        for (unsigned int i = 0; i < NUM_LINES; i++) {
            numTrainsPerLine[i] = stoi(trainsPerLineString[i]);
        }
    }

    /////////////////////////////////
    // Initialize child processes //
    ///////////////////////////////
    unsigned int numStationSides = numStations * 2;

    // Encode the next node that the station side should pass the train to, depending on the line that the
    // train belongs to
    int** stationSideNextNode = (int **)malloc(sizeof(int*) * numStationSides);
    for (unsigned int i = 0; i < numStationSides; i++) {
        stationSideNextNode[i] = (int*) malloc(sizeof(int) * NUM_LINES);
        for (unsigned int j = 0; j < NUM_LINES; j++) {
            stationSideNextNode[i][j] = -1;
        }
    }

    int** beforeNodeInfo = (int **) malloc(sizeof(int*) * numStationSides);
    for (unsigned int i = 0; i < numStationSides; i++) {
        beforeNodeInfo[i] = (int*) malloc(sizeof(int) * NUM_LINES);
        for (unsigned int j = 0; j < NUM_LINES; j++) {
            beforeNodeInfo[i][j] = -1;
        }
    }

    vector<vector<unsigned int>> linksIndex(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linksIndex[i].resize(numStations);
    }

    std::vector<pair<unsigned int, unsigned int>> links;
    {
        unsigned int count(0);
        for (unsigned int i = 0; i < NUM_LINES; i++) {
            // Set next links for trains station
            for (unsigned int j = 0; j < (line_stationIds[i].size() - 1); j++) {
                unsigned int curr = line_stationIds[i][j];
                unsigned int next = line_stationIds[i][j + 1];

                bool previouslyEnter = false;
                for (unsigned int k = 0; k < links.size(); k++) {
                    if ((links[k].first == curr) && links[k].second == next) {
                        previouslyEnter = true;
                    }
                }

                if (!previouslyEnter) {
                    linksIndex[curr][next] = links.size();
                    links.push_back(pair<unsigned int, unsigned int>(curr, next));
                    linksIndex[next][curr] = links.size();
                    links.push_back(pair<unsigned int, unsigned int>(next + numStations, curr + numStations));

                    stationSideNextNode[curr][i] = linksIndex[curr][next] + numStationSides;
                    stationSideNextNode[next + numStations][i] = linksIndex[next][curr] + numStationSides;

                    beforeNodeInfo[next][i] = linksIndex[curr][next] + numStationSides;
                    beforeNodeInfo[curr + numStations][i] = linksIndex[next][curr] + numStationSides;
                }
            }

            // Set for terminal stations
            unsigned int startStationId = line_stationIds[i][0];
            unsigned int endStationId = line_stationIds[i][line_stationIds[i].size() - 1];

            beforeNodeInfo[endStationId + numStations][i] = endStationId;
            beforeNodeInfo[startStationId][i] = startStationId + numStations;

            stationSideNextNode[startStationId + numStations][i] = startStationId;
            stationSideNextNode[endStationId][i] = endStationId + numStations;
        }
    }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    unsigned int numChildProcesses = numStationSides + links.size();
    int* errcodes = (int *) malloc(sizeof(int) * numChildProcesses);

    // Spawn link processes
    MPI_Comm centralComm;
    MPI_Comm_spawn("./simulation_process", MPI_ARGV_NULL, numChildProcesses, MPI_INFO_NULL, 0,
                   MPI_COMM_WORLD, &centralComm, errcodes);

    MPI_Bcast(&numStationSides, 1, MPI_UINT32_T, MPI_ROOT, centralComm);

    // Send the node info for the child processes
    int** nodeInfo = (int **) malloc(sizeof(int*) * numChildProcesses);
    for (unsigned int i = 0; i < numChildProcesses; i++) {
        bool isLink = i >= numStationSides;
        nodeInfo[i] = (int*) malloc(sizeof(int) * NUM_LINES);

        if (isLink) {
            nodeInfo[i][0] = links[i - numStationSides].first;
            nodeInfo[i][1] = links[i - numStationSides].second;

            unsigned int startId = nodeInfo[i][0] - ((nodeInfo[i][0] >= numStations) ? numStations : 0);
            unsigned int endId = nodeInfo[i][1] - ((nodeInfo[i][1] >= numStations) ? numStations : 0);

            nodeInfo[i][2] = linkCosts[startId][endId];
        } else {
            for (unsigned int j = 0; j < NUM_LINES; j++) {
                nodeInfo[i][j] = stationSideNextNode[i][j];
            }
        }
        MPI_Send(nodeInfo[i], 3, MPI_INT, i, 0, centralComm);
    }
    for (unsigned int i = 0; i < numStationSides; i++) {
        MPI_Send(beforeNodeInfo[i], 3, MPI_INT, i, 0, centralComm);
    }

    // Send the number  of ticks to the child process
    MPI_Bcast(&numTicks, 1, MPI_UINT32_T, MPI_ROOT, centralComm);
    MPI_Barrier(centralComm);
    for (unsigned int i = 0; i < numChildProcesses; i++) {
        free(nodeInfo[i]);
    }
    free(nodeInfo);

    // Send station popularities
    {
        float* stationSidePopularities = (float *) malloc(sizeof(float) * numStationSides);
        for (unsigned int i = 0; i < numStationSides; i++) {
            unsigned int stationId = i - (i >= numStations) ? numStations : 0;
            stationSidePopularities[i] = stationPopularities[stationId];
        }

        for (unsigned int i = 0; i < numStationSides; i++) {
            MPI_Send(&stationSidePopularities[i], 1, MPI_FLOAT, i, 0, centralComm);
        }
        free(stationSidePopularities);
    }

    // Send the initial trains
    {
        // Initialize the number of trains for each Line for each station. If not initial line, then the number of
        // trains is zero
        unsigned int** initialStationSideNumTrains = (unsigned int**) malloc(numStationSides * sizeof(unsigned int*));
        for (unsigned int i = 0; i < numStationSides; i++) {
            initialStationSideNumTrains[i] = (unsigned int*) calloc(NUM_LINES, sizeof(unsigned int));
        }

        for (unsigned int i = 0; i < NUM_LINES; i++) {
            unsigned int startSideId = line_stationIds[i][0];
            unsigned int endSideId = line_stationIds[i][line_stationIds[i].size() - 1] + numStations;

            // Allocate one more to the starting side if odd.
            initialStationSideNumTrains[startSideId][i] += (numTrainsPerLine[i] / 2) + (numTrainsPerLine[i] % 2);
            initialStationSideNumTrains[endSideId][i] += (numTrainsPerLine[i] / 2);
        }

        unsigned int* totalNumInitialTrains = (unsigned int*) calloc(numStationSides, sizeof(unsigned int));
        for (unsigned int i = 0; i < numStationSides; i++) {
            for (unsigned int j = 0; j < NUM_LINES; j++) {
                totalNumInitialTrains[i] += initialStationSideNumTrains[i][j];
            }
        }

        // Send the total total number of trains the child node should receive
        for (unsigned int i = 0; i < numStationSides; i++) {
            MPI_Send(&totalNumInitialTrains[i], 1, MPI_UINT32_T, i, 0, centralComm);
        }

        // The trains that the initial stations should receive represented as (line_id, train_id)
        vector<vector<unsigned int>> initialTrains(numStationSides);
        for (unsigned int i = 0; i < numStationSides; i++) {

            // Find the max number of trains for each line.
            unsigned int maxTrains = 0;
            for (unsigned int j  = 0; j < NUM_LINES; j++) {
                maxTrains = max(maxTrains, initialStationSideNumTrains[i][j]);
            }

            for (unsigned int j = 0; j < maxTrains; j++) {
                for (unsigned int k = 0; k < NUM_LINES; k++) {
                    if (initialStationSideNumTrains[i][k] > j) {
                        initialTrains[i].push_back(k);
                        if (i < numStations) {
                            // Starting side takes even numbered trains and zero
                            initialTrains[i].push_back(j * 2);
                        } else {
                            // Reverse side takes odd numbered trains
                            initialTrains[i].push_back(j * 2  + 1);
                        }
                    }
                }
            }
        }

        // Set the initial trains to send
        unsigned int** initialTrainsToSend = (unsigned int**) malloc(sizeof(unsigned int) * numStationSides);
        for (unsigned int i = 0; i < numStationSides; i++) {

            if (totalNumInitialTrains[i] <= 0) {
                continue;
            }

            initialTrainsToSend[i] = (unsigned int*) malloc(sizeof(unsigned int) * totalNumInitialTrains[i] * 2);
            for (unsigned j = 0; j < initialTrains[i].size(); j++) {
                initialTrainsToSend[i][j] = initialTrains[i][j];
            }
        }
        for (unsigned int i = 0; i < numStationSides; i++) {
            if (totalNumInitialTrains[i] <= 0) {
                continue;
            }
        }

        for (unsigned int i = 0; i < numStationSides; i++) {
            if (totalNumInitialTrains[i] <= 0) {
                continue;
            }

            MPI_Send(initialTrainsToSend[i], initialTrains[i].size(), MPI_UINT32_T, i, 0, centralComm);
        }
        MPI_Barrier(centralComm);


        //////////////////////
        // Clean Up Memory //
        ////////////////////
        for (unsigned int i = 0; i < numStationSides; i++) {
            if (totalNumInitialTrains[i] <= 0) {
                continue;
            }
            free(initialTrainsToSend[i]);
        }
        free(initialTrainsToSend);
        initialTrainsToSend = NULL;

        free(initialStationSideNumTrains);
        initialStationSideNumTrains = NULL;
    }


    ///////////////////////////
    // Receive Updates here //
    /////////////////////////
    unsigned int** trainLocations = (unsigned int **) malloc(sizeof(unsigned int*) * NUM_LINES);
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        trainLocations[i] = (unsigned int*) malloc(sizeof(unsigned int) * numTrainsPerLine[i]);
    }
    unsigned int** receivedTrains = (unsigned int **) malloc(sizeof(unsigned int*) * numChildProcesses);

    for (unsigned int t = 0; t < numTicks; t++) {
        unsigned int* numUpdatesToReceive = (unsigned int*) malloc(sizeof(unsigned int) * numChildProcesses);
        for (unsigned int i = 0; i < numChildProcesses; i++) {
            MPI_Recv(&numUpdatesToReceive[i], 1, MPI_UINT32_T, i, 0, centralComm, NULL);
        }

        for (unsigned int i = 0; i < numChildProcesses; i++) {
            if (numUpdatesToReceive[i] > 0) {
                receivedTrains[i] = (unsigned int *) malloc(sizeof(unsigned int) * numUpdatesToReceive[i] * 2);
                MPI_Recv(receivedTrains[i], numUpdatesToReceive[i] * 2, MPI_UINT32_T, i, 0, centralComm, NULL);
            }

            for (unsigned int j = 0; j < numUpdatesToReceive[i]; j++) {
                unsigned int lineId = receivedTrains[i][j * 2];
                unsigned int trainId = receivedTrains[i][j * 2 + 1];

                if (i >= numStationSides && j > 0) {
                    unsigned int linkIdx = i - numStationSides;
                    pair<unsigned int, unsigned int> link = links[linkIdx];
                    trainLocations[lineId][trainId] = link.first;
                } else {
                    trainLocations[lineId][trainId] = i;
                }
            }
        }

        // Print tick updates
        cout << t << ": ";
        for (unsigned int i = 0; i < NUM_LINES; i++) {
            for (unsigned int j = 0; (j < numTrainsPerLine[i]) && (j < ((t+1) * 2)); j++) {
                bool atLink = trainLocations[i][j] >= numStationSides;
                if (atLink) {
                    unsigned int linkId = trainLocations[i][j] - numStationSides;
                    unsigned int startStationId = links[linkId].first;
                    unsigned int endStationId = links[linkId].second;

                    printf(linkFormat, linePrefixes[i], j, startStationId, endStationId);
                } else {
                    unsigned int stationId = trainLocations[i][j];
                    if (stationId >= numStations) {
                        stationId -= numStations;
                    }
                    printf(stationFormat, linePrefixes[i], j, stationId);
                }
                if (!((i == (NUM_LINES - 1)) && ((j == (numTrainsPerLine[j] - 1)) || (j == ((t+1) * 2 - 1))))) {
                    cout << ", ";
                }
            }
        }
        cout << endl;
        MPI_Barrier(centralComm);
    }



    ////////////////////////
    // Final Update here //
    //////////////////////
    const unsigned int NUM_WAIT_TIME_INFO = 4;

    const unsigned int TOTAL_WAIT_TIME_IDX = 0;
    const unsigned int MIN_WAIT_TIME_IDX = 1;
    const unsigned int MAX_WAIT_TIME_IDX = 2;
    const unsigned int NUM_TRAINS_VISITED_IDX = 3;

    unsigned int offset = NUM_WAIT_TIME_INFO * NUM_LINES;

    unsigned int** waitTimes = (unsigned int**) malloc(sizeof(unsigned int*) * numStationSides);
    for (unsigned int i = 0; i < numStationSides; i++) {
        waitTimes[i] = (unsigned int *) malloc(sizeof(unsigned int) * offset);
    }

    for (unsigned int i = 0; i < numStationSides; i++) {
       MPI_Recv(waitTimes[i], offset, MPI_UINT32_T, i, 0, centralComm, NULL);
    }
    MPI_Barrier(centralComm);

    // free(errcodes);
    cout.setf(ios::fixed);
    cout << "\nAverage Wait Times:" << endl;

    for (unsigned int i = 0; i < NUM_LINES; i++) {
        float avgMin(0), avgMax(0), avgAvg(0), count(0), numTrains(0);

        for (unsigned int j = 0; j < line_stationIds[i].size(); j++) {
            unsigned int forwardId = line_stationIds[i][j];
            unsigned int reverseId = forwardId + numStations;
            unsigned int dataOffset = i * NUM_WAIT_TIME_INFO;

            if (waitTimes[forwardId][dataOffset + NUM_TRAINS_VISITED_IDX] > 0) {
                avgMin += waitTimes[forwardId][dataOffset + MIN_WAIT_TIME_IDX];
                avgMax += waitTimes[forwardId][dataOffset + MAX_WAIT_TIME_IDX];
                avgAvg += waitTimes[forwardId][dataOffset + TOTAL_WAIT_TIME_IDX];
                count += 1;
                numTrains += waitTimes[forwardId][i * NUM_WAIT_TIME_INFO + NUM_TRAINS_VISITED_IDX];
            }

            if (waitTimes[reverseId][dataOffset + NUM_TRAINS_VISITED_IDX] > 0) {
                avgMin += waitTimes[reverseId][dataOffset + MIN_WAIT_TIME_IDX];
                avgMax += waitTimes[reverseId][dataOffset + MAX_WAIT_TIME_IDX];
                avgAvg += waitTimes[reverseId][dataOffset + TOTAL_WAIT_TIME_IDX];
                count += 1;
                numTrains += waitTimes[reverseId][dataOffset + NUM_TRAINS_VISITED_IDX];
            }
        }
        avgMin /= count;
        avgMax /= count;
        avgAvg /= numTrains;
        cout << finalLinePrefixes[i] << ": " << numTrainsPerLine[i] << " trains -> "
             << setprecision(1) << avgAvg << ", " << setprecision(1) << avgMin << ", "
             << setprecision(1) << avgMax << endl;
    }
    MPI_Finalize();

    return EXIT_SUCCESS;
}