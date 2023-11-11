#include <iostream>
#include <fstream>
#include <random>
#include <unordered_map>
#include <bits/stdc++.h>
#include "generals.h"
#include "lsh_func.h"

void lsh_knn(int** pixels, std::unordered_multimap<int, int>** mm, Neibs<int>* lsh, int** w, double** t, int** rs, long* id, int query, int L, int K, long M, int NO_IMAGES, int DIMENSION){
    int countLSH = 0;
    for(int l = 0; l < L; l++){
        int key = g(pixels, w[l], t[l], rs[l], id, query, K, M, NO_IMAGES/8, DIMENSION, l);
        auto itr = mm[l]->equal_range(key);
        for (auto it = itr.first; it != itr.second; it++) {
            lsh->insertionsort_insert(it->second);
            countLSH++;
            if( countLSH > 10*L ){
                break;
            }
        }
        
        if( countLSH > 10*L ){
            break;
        }
    }
}

void lsh_rangeSearch(int** pixels, int** queries, std::unordered_multimap<int, int>** mm, Neibs<int>* rangeSearch, int** w, double** t, int** rs, long* id, int query, int L, int K, long M, int NO_IMAGES, int DIMENSION, float R){
    int countRange = 0;
    for(int l = 0; l < L; l++){
        int key = g(pixels, w[l], t[l], rs[l], id, query, K, M, NO_IMAGES/8, DIMENSION, l);
        auto itr = mm[l]->equal_range(key);
        auto it = itr.first;
        for (int i = 0; i != mm[l]->count(key) - 1; i++) {
            countRange++;
            if (it->second == query){
                if (++it == itr.second) break;
            }
            if ( dist(pixels[it->second],queries[query],2,DIMENSION) >= R ) continue;
                rangeSearch->insertionsort_insert(it->second);
            if( countRange > 20*L ){
                break;
            }
            it++;
        }
        
        if( countRange > 20*L ){
            break;
        }
    }
}

int main(int argc, char const *argv[]){
    std::string input_file;
    std::string output_file;
    std::string query_file;
    const int NO_QUERIES = 10;
    const int NO_IMAGES = 60000;
    const int DIMENSION = 784;
    const long M = 34359738363;     // 2^35 - 5 οπως λενε οι διαφανειες
    int L = 5;
    int K = 4;    
    int N = 1;
    float R = 10000.0;

    for (int i = 1 ; i < argc ; i++){
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc){
            input_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc){
            query_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc){
            K = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc){
            L = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc){
            output_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-N") == 0 && i + 1 < argc){
            N = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-R") == 0 && i + 1 < argc){
            R = atof(argv[i + 1]);
        }
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

    for (int i = 0 ; i < L ; i++){
        w[i] = new int[K];
        t[i] = new double[K];
        rs[i] = new int[K];
        for (int j = 0 ; j < K ; j++){
            w[i][j] = rand()%5 + 2; // τυχαιο για καθε μαπ απο 2 εως 6
            t[i][j] = ( rand()%(w[i][j]*1000) )/1000.0; // τυχαιο για καθε μαπ στο [0,w)
            rs[i][j] = rand();  // τα r ειναι τυχαια
        }
    }

    std::unordered_multimap<int, int>* mm[L]; // empty multimap container
    long* id = new long[NO_IMAGES];

    for (int l = 0 ; l < L ; l++){
        mm[l] = new std::unordered_multimap<int, int>();
        for (int j = 0 ; j < NO_IMAGES ; j++){
            int key = g(pixels, w[l], t[l], rs[l], id, j, K, M, NO_IMAGES/8, DIMENSION, l);
            mm[l]->insert({key, j});
        }
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

    while (1){
        std::cout << "Processing the data..." << std::endl;

        // Read from query file
        int** queries = readfile<int>(query_file, NO_QUERIES, DIMENSION);

        auto startLSH = std::chrono::high_resolution_clock::now();
        
        int query = rand() % NO_QUERIES;
        Neibs<int>* lsh = new Neibs<int>(pixels, queries, DIMENSION, N, query, &dist);

        lsh_knn(pixels, mm, lsh, w, t, rs, id, query, L, K, M, NO_IMAGES, DIMENSION);

        Neibs<int>* rangeSearch = new Neibs<int>(pixels, queries, DIMENSION, NO_IMAGES, query, &dist);

        lsh_rangeSearch(pixels, queries, mm, rangeSearch, w, t, rs, id, query, L, K, M, NO_IMAGES, DIMENSION, R);

        auto stopLSH = std::chrono::high_resolution_clock::now();

        auto startReal = std::chrono::high_resolution_clock::now();

        Neibs<int>* real_neighbs = new Neibs<int>(pixels, queries, DIMENSION, N, query, &dist);
        for (int i = 0 ; i < NO_IMAGES ; i++){
            real_neighbs->insertionsort_insert(i);
        }

        auto stopReal = std::chrono::high_resolution_clock::now();

        Output << "Query: " << query << std::endl;
        for (int i = 0 ; i < N ; i++){
            Output << "Nearest neighbor-" << i + 1 << ": " << lsh->givenn(i) << std::endl;
            Output << "distanceLSH: " << lsh->givedist(i) << std::endl;
            Output << "distanceTrue: " << real_neighbs->givedist(i) << std::endl;
        }

        auto durationLSH = std::chrono::duration_cast<std::chrono::milliseconds>(stopLSH - startLSH);
        auto durationReal = std::chrono::duration_cast<std::chrono::milliseconds>(stopReal - startReal);

        Output << "tLSH: " << durationLSH.count() << " milliseconds" << std::endl;
        Output << "tTrue: " << durationReal.count() << " milliseconds" << std::endl;

        int rangecount = rangeSearch->give_size();
        Output << "R-near Neighbors: " << rangecount << std::endl;
        for (int i = 0 ; i < rangecount ; i++){
            Output << rangeSearch->givenn(i) << std::endl;
        }

        delete lsh;
        delete rangeSearch;
        delete real_neighbs;

        std::cout << "Type quit to stop or a different query file name to rerun it with" << std::endl;
        std::cin >> query_file;
        if (query_file == "quit") break;
    }
    
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
        // delete mm[i];
    }
    delete[] w;
    delete[] t;
    delete[] rs;

    delete[] id;

    return 0;
}