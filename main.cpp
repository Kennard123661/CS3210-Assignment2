#include <iostream>
#include <openmpi/mpi.h>
#include <vector>
#include <sstream>
#include <list>
#include <mpi.h>

using namespace std;

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

class StationSide {
public:
    unsigned int lastVisited;
    unsigned int numVisited;
    unsigned int leastWaitTime;
    unsigned int maxWaitTime;
    unsigned int totalWaitTime;
    float stationCountDown;
    float popularity;

    StationSide(float _popularity) {
        lastVisited = 0;
        numVisited = 0;
        leastWaitTime = UINT32_MAX;
        maxWaitTime = 0;
        totalWaitTime = 0;
        stationCountDown = 0;
        popularity = _popularity;
    };

    StationSide(const StationSide &_other) {
        lastVisited = _other.lastVisited;
        numVisited = _other.numVisited;
        leastWaitTime = _other.leastWaitTime;
        maxWaitTime = _other.maxWaitTime;
        totalWaitTime = _other.totalWaitTime;
        stationCountDown = _other.stationCountDown;
        popularity = _other.popularity;
    }

    void visited(unsigned int time) {
        unsigned int waitTime = time - lastVisited;
        numVisited++;
        leastWaitTime = min(waitTime, leastWaitTime);
        maxWaitTime = max(waitTime, maxWaitTime);
        totalWaitTime += waitTime;
        stationCountDown = (((rand() % 10) + 1) * popularity) - 1;
    }

    void left(unsigned int time) {
        lastVisited = time;
    }
};

class LineNetwork{
public:
    vector<unsigned int> stationSides;

    LineNetwork(vector<unsigned int> &stationIds, unsigned int numStations) {
        for (unsigned int i = 0; i < stationIds.size(); i++) {
            stationSides.push_back(stationIds[i]);
        }

        for (unsigned int i = 0; i < stationIds.size(); i++) {
            stationSides.push_back(stationIds[stationIds.size() - i - 1] + numStations);
        }
    }

    LineNetwork(const LineNetwork &_other) {
        for (unsigned int i = 0; i < _other.stationSides.size(); i++) {
            stationSides.push_back(_other.stationSides[i]);
        }
    }

    unsigned int getNextLineIdx(unsigned int currLineIdx) {
        return (currLineIdx + 1) % stationSides.size();
    }

    unsigned char isTerminal(unsigned int currLineIdx) {
        return (currLineIdx == ((stationSides.size() / 2) - 1))  || (currLineIdx == (stationSides.size() - 1));
    }
};

class Train {
public:
    unsigned int lineId;
    unsigned int lineIndex;
    unsigned int trainId;

    Train(unsigned int _lineId, unsigned int _trainId) {
        lineId = _lineId;
        lineIndex = 0;
        trainId = _trainId;
    }

    Train(const Train &_other) {
        lineId = _other.lineId;
        lineIndex = _other.lineIndex;
        trainId = _other.trainId;
    }
};

int main() {
    const unsigned int NUM_LINES = 3;
    srand(static_cast<unsigned int>(time(NULL)));
    const unsigned int STATUS_NOT_PROCESSING = 0;
    const unsigned int STATUS_FINISHED_PROCESSING = 1;
    const unsigned int STATUS_IS_PROCESSING = 2;


    // Read inputs
    unsigned int numStations;
    cin >> numStations;

    vector<string> stationNames;
    getStationNames(stationNames);
    map<string, unsigned int> stationNamesToStationId;
    for (unsigned int i = 0; i < numStations; i++) {
        stationNamesToStationId.insert(pair<string, unsigned int>(stationNames[i], i));
        // cout << stationNames[i] << endl;
    }

    vector<vector<unsigned int>> linksIndex(numStations);
    std::vector<pair<unsigned int, unsigned int>> links;
    vector<vector<unsigned int>> linkCosts;
    linkCosts.resize(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linkCosts[i].resize(numStations);
        linksIndex[i].resize(numStations);
        for (unsigned int j = 0; j < numStations; j++) {
            cin >> linkCosts[i][j];
            if (linkCosts[i][j] > 0) {
                linksIndex[i][j] = links.size();
                links.push_back(pair<unsigned int, unsigned int>(i,j));
            }
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

    vector<LineNetwork> lineNetworks;
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        lineNetworks.push_back(LineNetwork(line_stationIds[i], numStations));
    }

    vector<StationSide> stationSides;
    {
        for (unsigned int i = 0; i < numStations; i++) {
            stationSides.push_back(StationSide(stationPopularities[i]));
        }

        for (unsigned int i = 0; i < numStations; i++) {
            stationSides.push_back(StationSide(stationPopularities[i]));
        }
    }

    vector<vector<Train>> trains(NUM_LINES);
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        for (unsigned int j = 0; j < numTrainsPerLine[i]; j++) {
            trains[i].push_back(Train(i, j));
        }
    }

    vector<list<pair<unsigned int, unsigned int>>> linkTrainQueues(links.size());
    vector<list<pair<unsigned int, unsigned int>>> stationSideTrainQueues(numStations * 2);

    ////////////////////////////
    //// Begin the Good Stuff //
    ////////////////////////////

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    unsigned int numChildProcesses = links.size();
    int* errcodes = (int *) malloc(sizeof(int) * numChildProcesses);

    MPI_Comm intercomm;
    // Spawn link processes
    MPI_Comm_spawn("./simulation_process", MPI_ARGV_NULL, numChildProcesses, MPI_INFO_NULL, 0,
                   MPI_COMM_WORLD, &intercomm, errcodes);

    int intercommRank;
    MPI_Comm_rank(intercomm, &intercommRank);

    // Initialize link processes
    unsigned int** linkCostTuples = (unsigned int **) malloc(sizeof(unsigned int*) * links.size());
    for (unsigned int i = 0; i < links.size(); i++) {
        linkCostTuples[i] = (unsigned int*) malloc(sizeof(unsigned int) * 3);
        linkCostTuples[i][0] = links[i].first;
        linkCostTuples[i][1] = links[i].second;
        linkCostTuples[i][2] = linkCosts[links[i].first][links[i].second];
        MPI_Send(linkCostTuples[i], 3, MPI_UINT32_T, i, 0, intercomm);
    }
    MPI_Barrier(intercomm);

    MPI_Bcast(&numTicks, 1, MPI_UINT32_T, MPI_ROOT, intercomm);
    MPI_Barrier(intercomm);

    unsigned int* stationStatus = (unsigned int*) malloc(sizeof(unsigned int) * numStations * 2);
    for (unsigned int i = 0; i < numStations * 2; i++) {
        stationStatus[i] = STATUS_NOT_PROCESSING;
    }

    unsigned int* statusBuffer = (unsigned int*) malloc(sizeof(unsigned int) * numChildProcesses);
    for (unsigned int i = 0; i < numChildProcesses; i++) {
        statusBuffer[i] = 0;
    }

    // Actually processing here
    for (unsigned int t = 0; t < numTicks; t++) {
        for (unsigned int i = 0; i < NUM_LINES; i++) {
            for (unsigned int j = (t*2); (j < numTrainsPerLine[i]) && (j < ((t+1) * 2)); j++) {
                unsigned int stationSideId = lineNetworks[i].stationSides[trains[i][j].lineIndex];
                stationSideTrainQueues[stationSideId].push_back(pair<unsigned int, unsigned int>(i,j));
            }
        }

        for (unsigned int i = 0; i < numChildProcesses; i++) {
            MPI_Recv(&statusBuffer[i], 1, MPI_UINT32_T, i, 0, intercomm, NULL);
        }
        MPI_Barrier(intercomm);

        for (unsigned int i = 0; i < numStations * 2; i++) {
            if (stationStatus[i] == STATUS_FINISHED_PROCESSING) {
                stationSides[i].left(t);
                pair<unsigned int, unsigned int> trainIndices = stationSideTrainQueues[i].front();
                Train* trainPtr = &trains[trainIndices.first][trainIndices.second];
                unsigned int nextIdx = lineNetworks[trainPtr->lineId].getNextLineIdx(trainPtr->lineIndex);
                unsigned int nextStationId = lineNetworks[trainPtr->lineId].stationSides[nextIdx];

                if (lineNetworks[trainPtr->lineId].isTerminal(trainPtr->lineIndex)) {
                    stationSideTrainQueues[nextStationId].push_back(trainIndices);
                    stationSideTrainQueues[i].pop_front();
                } else {
                    unsigned int currStationId = lineNetworks[trainPtr->lineId].stationSides[trainPtr->lineIndex];
                    unsigned int linkIdx = linksIndex[currStationId][nextStationId];
                    linkTrainQueues[linkIdx].push_back(trainIndices);
                    stationSideTrainQueues[i].pop_front();
                }
                stationStatus[i] = STATUS_NOT_PROCESSING;
            }
        }

        for (unsigned int i = 0; i < numChildProcesses; i++) {
            if (statusBuffer[i] == STATUS_FINISHED_PROCESSING) {
                pair<unsigned int, unsigned int> trainIndices = stationSideTrainQueues[i].front();
                Train* trainPtr = &trains[trainIndices.first][trainIndices.second];
                unsigned int nextIdx = lineNetworks[trainPtr->lineId].getNextLineIdx(trainPtr->lineIndex);
                unsigned int nextStationId = lineNetworks[trainPtr->lineId].stationSides[nextIdx];
                stationSideTrainQueues[nextStationId].push_back(trainIndices);
                linkTrainQueues[i].pop_front();
                statusBuffer[i] = STATUS_NOT_PROCESSING;
            }
        }



        for (unsigned int i = 0; i < numChildProcesses; i++) {
            if (statusBuffer[i] == STATUS_NOT_PROCESSING) {
                if (!linkTrainQueues[i].empty()) {
                    statusBuffer[i] = STATUS_IS_PROCESSING;
                }
                MPI_Send(&statusBuffer[i], 1, MPI_UINT32_T, i, 0, intercomm);
            }
        }
        MPI_Barrier(intercomm);

        for (unsigned int i = 0; i < numStations * 2; i++) {
            if (stationStatus[i] == STATUS_NOT_PROCESSING) {
                if (!stationSideTrainQueues[i].empty()) {
                    stationSides[i].visited(t);
                    if (stationSides[i].stationCountDown <= 0) {
                        stationStatus[i] = STATUS_FINISHED_PROCESSING;
                    } else {
                        stationStatus[i] = STATUS_IS_PROCESSING;
                    }
                }
            } else if (stationStatus[i] == STATUS_IS_PROCESSING) {
                stationSides[i].stationCountDown -=  1;
                if (stationSides[i].stationCountDown <= 0) {
                    stationStatus[i] = STATUS_FINISHED_PROCESSING;
                }
            }
        }

        
    }

    MPI_Finalize();

    return 0;
}