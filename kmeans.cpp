#include "kmeans.h"

using namespace std;

Point::Point(int id_point, vector<int>& values, string name = "")
{
    this->id_point = id_point;
    total_values = values.size();

    for(int i = 0; i < total_values; i++)
        this->values.push_back(values[i]);

    this->name = name;
    id_cluster = -1;
}

int Point::getID()
{
    return id_point;
}

void Point::setCluster(int id_cluster)
{
    this->id_cluster = id_cluster;
}

int Point::getCluster()
{
    return id_cluster;
}

int Point::getValue(int index)
{
    return values[index];
}

int Point::getTotalValues()
{
    return total_values;
}

void Point::addValue(int value)
{
    values.push_back(value);
}

string Point::getName()
{
    return name;
}

Cluster::Cluster(int id_cluster, Point point)
{
    this->id_cluster = id_cluster;

    int total_values = point.getTotalValues();

    for(int i = 0; i < total_values; i++)
        central_values.push_back(point.getValue(i));

    points.push_back(point);
}

void Cluster::addPoint(Point point)
{
    points.push_back(point);
}

bool Cluster::removePoint(int id_point)
{
    int total_points = points.size();

    for(int i = 0; i < total_points; i++)
    {
        if(points[i].getID() == id_point)
        {
            points.erase(points.begin() + i);
            return true;
        }
    }
    return false;
}

int Cluster::getCentralValue(int index)
{
    return central_values[index];
}

void Cluster::setCentralValue(int index, int value)
{
    central_values[index] = value;
}

Point Cluster::getPoint(int index)
{
    return points[index];
}

int Cluster::getTotalPoints()
{
    return points.size();
}

int Cluster::getID()
{
    return id_cluster;
}

KMeans::KMeans(int K, int total_points, int total_values, int max_iterations)
    {
        this->K = K;
        this->total_points = total_points;
        this->total_values = total_values;
        this->max_iterations = max_iterations;
    }

int KMeans::getIDNearestCenter(Point point)
{
    int sum = 0, min_dist;
    int id_cluster_center = 0;

    // Check distance to cluster #0 
    for(int i = 0; i < total_values; i++) {
        sum += abs(clusters[0].getCentralValue(i) - point.getValue(i));
    }

    min_dist = sum;

    // Check distance to the rest of the clusters and see which one is the smallest
    for(int i = 1; i < K; i++) {
        int dist;
        sum = 0;

        for(int j = 0; j < total_values; j++) {
            sum += abs(clusters[i].getCentralValue(j) - point.getValue(j));
        }

        dist = sum;

        if(dist < min_dist)
        {
            min_dist = dist;
            id_cluster_center = i;
        }

    }
}

void KMeans::run(vector<Point> & points)
{
    if(K > total_points)
        return;

    vector<int> prohibited_indexes;

    // choose K distinct values for the centers of the clusters
    for(int i = 0; i < K; i++)
    {
        while(true)
        {
            int index_point = rand() % total_points;

            if(find(prohibited_indexes.begin(), prohibited_indexes.end(),
                    index_point) == prohibited_indexes.end())
            {
                prohibited_indexes.push_back(index_point);
                points[index_point].setCluster(i);
                Cluster cluster(i, points[index_point]);
                clusters.push_back(cluster);
                break;
            }
        }
    }

    int iter = 1;

    while(true)
    {
        bool done = true;

        // associates each point to the nearest center
        for(int i = 0; i < total_points; i++)
        {
            int id_old_cluster = points[i].getCluster();
            int id_nearest_center = getIDNearestCenter(points[i]);

            if(id_old_cluster != id_nearest_center)
            {
                if(id_old_cluster != -1)
                    clusters[id_old_cluster].removePoint(points[i].getID());

                points[i].setCluster(id_nearest_center);
                clusters[id_nearest_center].addPoint(points[i]);
                done = false;
            }
        }

        // recalculating the center of each cluster
        for(int i = 0; i < K; i++)
        {
            for(int j = 0; j < total_values; j++)
            {
                int total_points_cluster = clusters[i].getTotalPoints();
                double sum = 0.0;

                if(total_points_cluster > 0)
                {
                    for(int p = 0; p < total_points_cluster; p++)
                        sum += clusters[i].getPoint(p).getValue(j);
                    clusters[i].setCentralValue(j, sum / total_points_cluster);
                }
            }
        }

        if(done == true || iter >= max_iterations)
        {
            cout << "Break in iteration " << iter << "\n\n";
            break;
        }

        iter++;
    }

    // shows elements of clusters
    for(int i = 0; i < K; i++)
    {
        int total_points_cluster =  clusters[i].getTotalPoints();

        cout << "Cluster " << dec << clusters[i].getID() + 1 << endl;
        for(int j = 0; j < total_points_cluster; j++)
        {
            cout << "Point " << dec << clusters[i].getPoint(j).getID() + 1 << ": ";
            for(int p = 0; p < total_values; p++)
                cout << hex << clusters[i].getPoint(j).getValue(p) << " ";

            string point_name = clusters[i].getPoint(j).getName();

            if(point_name != "")
                cout << "- " << point_name;

            cout << endl;
        }

        cout << "Cluster values: ";

        for(int j = 0; j < total_values; j++)
            cout << hex << clusters[i].getCentralValue(j) << " ";

        cout << "\n\n";
    }
}

// void Entries::clustering(int total_points, int K, int max_iterations, vector<array<int,64>>& cacheLines)
void Entries::clustering(int total_points, int K, int max_iterations, vector<array<int,64>>& cacheLines)
{
    srand (time(NULL));

    // Total points = # cache lines (vector size), Total values = 64, K = Precompression table entries, Max iterations = depends, Has name = 0
    int total_points, total_values = 64, K, max_iterations, has_name = 0;

    vector<Point> points;
    string point_name;

    for (uint i=0; i<cacheLines.size(); i++) {

        vector<int> values;
        for (auto it = cacheLines[i].begin(); it != cacheLines[i].end(); ++it) {
            int value;
            value = it->data;
            values.push_back(value);
        }

        Point p(i, values);
        points.push_back(p);
    }

    KMeans kmeans(K, total_points, total_values, max_iterations);
    kmeans.run(points);

    return 0;
}