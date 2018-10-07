#include <iostream>
#include <openmpi/mpi.h>
#include <vector>
#include <sstream>

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
    float popularity;

    StationSide(float _popularity) {
        lastVisited = 0;
        numVisited = 0;
        leastWaitTime = UINT32_MAX;
        maxWaitTime = 0;
        totalWaitTime = 0;
        popularity = _popularity;
    };

    StationSide(const StationSide &_other) {
        lastVisited = _other.lastVisited;
        numVisited = _other.numVisited;
        leastWaitTime = _other.leastWaitTime;
        maxWaitTime = _other.maxWaitTime;
        totalWaitTime = _other.totalWaitTime;
        popularity = _other.popularity;
    }

    void visited(unsigned int time) {
        unsigned int waitTime = time - lastVisited;
        numVisited++;
        leastWaitTime = min(waitTime, leastWaitTime);
        maxWaitTime = max(waitTime, maxWaitTime);
        totalWaitTime += waitTime;
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

    std::vector<pair<unsigned int, unsigned int>> links;
    vector<vector<unsigned int>> linkCosts;
    linkCosts.resize(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linkCosts[i].resize(numStations);
        for (unsigned int j = 0; j < numStations; j++) {
            cin >> linkCosts[i][j];
            if (linkCosts[i][j] > 0) {
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

    vector<unsigned int> stationSidePopularities(numStations * 2);
    for (unsigned int i = 0; i < numStations; i++) {
        stationSidePopularities[i] = stationPopularities[i];
        stationSidePopularities[i + numStations] = stationPopularities[i];
    }

    vector<LineNetwork> lineNetworks;
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        lineNetworks.push_back(LineNetwork(line_stationIds[i], numStations));
    }


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

    for (unsigned int t = 0; t < numTicks; t++) {
        MPI_Barrier(intercomm);
    }





    MPI_Finalize();

    return 0;
}