#include <iostream>
#include <list>
#include <string.h>
#include "generals.h"
#include "lsh_func.h"

class Graph {
private:
    int vertices; // Number of vertices
    std::list<int>* adjacencyList; // Array of linked lists to represent adjacency list

public:
    // Constructor
    Graph(int vertices) : vertices(vertices) {
        adjacencyList = new std::list<int>[vertices];
    }

    // Destructor
    ~Graph() {
        delete[] adjacencyList;
    }

    // Add a directed edge to the graph
    void addEdge(int source, int destination) {
        adjacencyList[source].push_back(destination);
    }

    // Print the adjacency list representation of the directed graph
    void printGraph() {
        for (int i = 0; i < vertices; ++i) {
            std::cout << "Adjacency list of vertex " << i << ": ";
            for (const auto& neighbor : adjacencyList[i]) {
                std::cout << neighbor << " ";
            }
            std::cout << std::endl;
        }
    }

    std::vector<int>* get_node_nn(int node, int E){
        std::vector<int>* vect = new std::vector<int>();
        int i = 0;
        for (const auto& neighbor : adjacencyList[node]) {
            vect->push_back(neighbor);
            if (i == E - 1) break;
        }

        return vect;
    }

    std::vector<int>* get_node_nn(int node){
        std::vector<int>* vect = new std::vector<int>();

        for (const auto& neighbor : adjacencyList[node]) {
            vect->push_back(neighbor);
        }

        return vect;
    }
};

void set_insert(std::vector<int>& S, std::vector<int>* cands){
    for (auto it = cands->begin(); it != cands->end(); it++){
        S.push_back(*it);
    }
}

int min(int** pixels, int** queries, int query, std::vector<int>* cands, int DIM, double (*dist)(int*, int*, int, int)){
    if (cands->empty()) {
        // Handle the case where the candidates vector is empty
        std::cerr << "Error: Empty candidates vector." << std::endl;
        return -1;  // Return an error code or handle it appropriately
    }

    auto curr_it = cands->begin();
    if (curr_it == cands->end()) {
        // Handle the case where the candidates vector is empty
        std::cerr << "Error: Empty candidates vector." << std::endl;
        return -1;  // Return an error code or handle it appropriately
    }

    int min = *curr_it;

    double min_dist = dist(pixels[min], queries[query], 2, DIM);
    curr_it++;

    for (auto it = curr_it; it != cands->end(); it++) {
        double curr_dist = dist(pixels[*it], queries[query], 2, DIM);

        if (curr_dist < min_dist) {
            min = *it;
            min_dist = curr_dist;
        }
    }

    return min;
}

int main(int argc, char const *argv[]) {
    std::string input_file;
    std::string output_file;
    std::string query_file;
    std::string method;
    const int NO_QUERIES = 10;
    const int NO_IMAGES = 60000;
    const int DIMENSION = 784;
    const long M = 34359738363;         // 2^35 - 5 οπως λενε οι διαφανειες
    const int T = 30;                   // Οριο αριθμου βηματων
    unsigned int N = 1;                 // Πλησιεστεροι γειτονες
    unsigned int k = 50;                // Πλησιεστεροι γειτονες στον γραφο k-NN
    unsigned int R = 1;                 // Αριθμος τυχαιων επανεκκινησεων
    unsigned int E = 30;                // Αριθμος επεκτασεων
    std::size_t candidate_size = 20;    // Το πληθος υποψηφιων L που θα δεχτουμε
    unsigned int L = ceil(k / 8.0);     // L maps της LSH
    unsigned int K = floor(L *0.625);   // K παραμετρος της LSH

    for (int i = 1 ; i < argc ; i++){
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc){
            input_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc){
            query_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc){
            k = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-E") == 0 && i + 1 < argc){
            E = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc){
            output_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc){
            N = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-R") == 0 && i + 1 < argc){
            R = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc){
            candidate_size = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc){
            if (strcmp(argv[i + 1], "1") == 0){
                method = "GNNS Results";

            }
            else if (strcmp(argv[i + 1], "2") == 0){
                method = "MRNG Results";

            }
            else{
                std::cerr << "Wrong method. Enter one of the following methods next time: 1 for GNNS, 2 for MRNG ";
                return -1;
            }
        }
    }

    if (method.empty()){
        std::cerr << "Error. No method entered. Next time enter 1 for GNNS, 2 for MRNG";
        return -1;
    }

    if (method == "MRNG Results" && N > candidate_size){
        std::cerr << "Error. N cannot be bigger than L. Try again with valid arguments";
        return -1;
    }

    if (input_file.empty()){
        std::cout << "Enter input file: ";
        std::cin >> input_file;
    }

    std::cout << "Preprocessing the data..." << std::endl;

    // Pixel array
    int** pixels = readfile<int>(input_file, NO_IMAGES, DIMENSION);

    srand(time(NULL));

    int** w = new int*[L];
    double** t = new double*[L];
    int** rs = new int*[L]; // οι L πινακες που θα εχουν τα r για καθε map
    std::unordered_multimap<int, int>* mm[L]; // empty multimap container
    long* id = new long[NO_IMAGES];

    lsh_init(pixels, w, t, rs, mm, id, L, K, M, NO_IMAGES, DIMENSION);

    std::cout << "Creating the graph..." << std::endl;

    Graph* gr = new Graph(NO_IMAGES);

    for (int i = 0 ; i < NO_IMAGES ; i++){
        Neibs<int>* lsh = new Neibs<int>(pixels, pixels, DIMENSION, k, i, &dist);
        lsh_knn(pixels, mm, lsh, w, t, rs, id, i, L, K, M, NO_IMAGES, DIMENSION, true);

        for (int j = 0 ; j < lsh->give_size() ; j++){
            gr->addEdge(i, lsh->givenn(j));
        }

        delete lsh;
    }

    if (query_file.empty()){
        std::cout << "Enter query file: ";
        std::cin >> query_file;
    }
    if (output_file.empty()){
        std::cout << "Enter output file: ";
        std::cin >> output_file;
    }

    // Create Output file to write
    std::ofstream Output(output_file);

    std::vector<double> timesGNNS;
    std::vector<double> timesTrue;
    double MAF = -1;

    Output << method << std::endl;

    while (1){
        std::cout << "Processing the data..." << std::endl;
        
        // Read from query file
        int** queries = readfile<int>(query_file, NO_QUERIES, DIMENSION);

        auto startGNNS = std::chrono::high_resolution_clock::now();

        int query = rand() % NO_QUERIES;

        // Process
        std::vector<int> S;
        
        for (int r = 0 ; r < R ; r++){
            int y = rand() % NO_IMAGES;
            for (int t = 0 ; t < T ; t++){
                std::vector<int>* candidates = gr->get_node_nn(y, E);
                set_insert(S, candidates);    // S = S U N(Y, E, G)
                int y_next = min(pixels, queries, query, candidates, DIMENSION, &dist);    // Yt = arg minY∈N(Yt−1,E,G)δ(Y, Q)
                delete candidates;

                // if (dist(pixels[y], queries[query], 2, DIMENSION) < dist(pixels[y_next], queries[query], 2, DIMENSION)) break;  // Local optimal
                y = y_next;
            }
        }
        Neibs<int>* gnns = new Neibs<int>(pixels, queries, DIMENSION, N, query, &dist);
        for (auto it = S.rbegin(); it != S.rend(); it++){
            gnns->insertionsort_insert(*it);
        }

        auto stopGNNS = std::chrono::high_resolution_clock::now();

        auto startReal = std::chrono::high_resolution_clock::now();

        Neibs<int>* real_neighbs = new Neibs<int>(pixels, queries, DIMENSION, N, query, &dist);
        for (int i = 0 ; i < NO_IMAGES ; i++){
            real_neighbs->insertionsort_insert(i);
        }

        auto stopReal = std::chrono::high_resolution_clock::now();

        Output << "Query: " << query << std::endl;
        for (int i = 0 ; i < N ; i++){
            Output << "Nearest neighbor-" << i + 1 << ": " << gnns->givenn(i) << std::endl;
            Output << "distanceApproximate: " << gnns->givedist(i) << std::endl;
            Output << "distanceTrue: " << real_neighbs->givedist(i) << std::endl;
            if (i == 0){
                double AF = gnns->givedist(i) / real_neighbs->givedist(i);
                if (MAF != -1){
                    if (MAF < AF){
                        MAF = AF;
                    }
                }
                else{
                    MAF = AF;
                }
            }
        }

        // Calculating total time taken by the program.
        double durationGNNS = std::chrono::duration_cast<std::chrono::milliseconds>(stopGNNS - startGNNS).count();
        durationGNNS *= 1e-3;
        timesGNNS.push_back(durationGNNS);

        double durationReal = std::chrono::duration_cast<std::chrono::milliseconds>(stopReal - startReal).count();
        durationReal *= 1e-3;
        timesTrue.push_back(durationReal);

        // Deallocations
        for (int i = 0 ; i < NO_QUERIES ; i++){
            delete[] queries[i];
        }
        delete[] queries;
        delete real_neighbs;
        delete gnns;

        // Quit or rerun with a different file
        std::cout << "Type quit to stop, type a different query file name to rerun it with or press enter to rerun with the same query file" << std::endl;
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.fail() || input.empty()) continue;
        if (input == "quit") break;
        query_file = input;
    }

    // Calculating average time of the algorithm and average real time
    double averageGNNS = 0.0;
    double averageTrue = 0.0;
    for (auto it = timesGNNS.begin(); it != timesGNNS.end(); it++){
        averageGNNS += *it;
    }
    averageGNNS /= timesGNNS.size();

    for (auto it = timesTrue.begin(); it != timesTrue.end(); it++){
        averageTrue += *it;
    }
    averageTrue /= timesTrue.size();
    
    // Printing the average time of the algorithm and the average real time
    Output << "tAverageApproximate: " << std::fixed << std::setprecision(4) << averageGNNS << " seconds" << std::endl;
    Output << "tAverageTrue: " << std::fixed << std::setprecision(4) << averageTrue << " seconds" << std::endl;
    Output << "MAF: " << std::fixed << std::setprecision(4) <<  MAF << std::endl;
    
    // Close Output file
    Output.close();

    // Deallocations
    for (int i = 0 ; i < NO_IMAGES ; i++){
        delete[] pixels[i];
    }
    delete[] pixels;

    for (int i = 0 ; i < L ; i++){
        delete[] w[i];
        delete[] t[i];
        delete[] rs[i];
        delete mm[i];
    }
    delete[] w;
    delete[] t;
    delete[] rs;
    delete[] id;
    delete gr;

    return 0;
}