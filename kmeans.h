#include <array>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <bitset>
#include <climits>

using namespace std;

// size_t popcount(size_t n) {
//     std::bitset<sizeof(size_t) * CHAR_BIT> b(n);
//     return b.count();
// }

class Point
{
private:
    int id_point, id_cluster;
    vector<int> values;
    int total_values;
    string name;

public:
    Point(int id_point, vector<int>& values, string name = "");
    int getID();
    void setCluster(int id_cluster);
    int getCluster();
    int getValue(int index);
    int getTotalValues();
    void addValue(int value);
    string getName();
};

class Cluster
{
private:
    int id_cluster;
    vector<int> central_values;
    vector<Point> points;

public:
    Cluster(int id_cluster, Point point);
    void addPoint(Point point);
    bool removePoint(int id_point);
    int getCentralValue(int index);
    void setCentralValue(int index, int value);
    Point getPoint(int index);
    int getTotalPoints();
    int getID();
};

class KMeans
{
private:
    int K; // number of clusters
    int total_values, total_points, max_iterations;
    vector<Cluster> clusters;

    int getIDNearestCenter(Point point);

public:
    KMeans(int K, int total_points, int total_values, int max_iterations);
    void run(vector<Point> & points);
};

// class Entries
// {
// public:
//     void clustering(int total_points, int K, int max_iterations, vector<array<int,64>>& cacheLines);
// };

class Entries
{
private:
    int K;
    int total_points, total_values, max_iterations;
    vector<array<int,64>> cacheLines;
public:
    Entries (int total_points, int total_values, int K, int max_iterations);
    void clustering(vector<array<int,64>> const &cacheLines);
};