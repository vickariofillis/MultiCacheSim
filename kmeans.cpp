#include "kmeans.h"

using namespace std;

Point::Point(int id_point, vector<int>& values, string name)
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

    return id_cluster_center;
}

void KMeans::run(vector<Point> & points)
{

    ofstream table;
    table.open("precomp_entries.out");

    cout << "Kmeans #0\n";

    if(K > total_points)
        return;

    vector<int> prohibited_indexes;

    // choose K distinct values for the centers of the clusters
    for(int i = 0; i < K; i++)
    {
        cout << "Kmeans #0.1\n";
        while(true)
        {
            cout << "Kmeans #0.2\n";
            int index_point = rand() % total_points;

            if(find(prohibited_indexes.begin(), prohibited_indexes.end(),
                    index_point) == prohibited_indexes.end())
            {
                cout << "Kmeans #0.3\n";
                prohibited_indexes.push_back(index_point);
                cout << "Kmeans #0.3.1\n";
                points[index_point].setCluster(i);
                cout << "Kmeans #0.3.2\n";
                Cluster cluster(i, points[index_point]);
                cout << "Kmeans #0.3.3\n";
                clusters.push_back(cluster);
                break;
            }
        }
    }

    cout << "Kmeans #1\n";
    int iter = 1;

    while(true)
    {
        cout << "Kmeans #1.1\n";
        bool done = true;

        // associates each point to the nearest center
        for(int i = 0; i < total_points; i++)
        {
            cout << "Kmeans #1.2\n";
            int id_old_cluster = points[i].getCluster();
            int id_nearest_center = getIDNearestCenter(points[i]);

            if(id_old_cluster != id_nearest_center)
            {
                cout << "Kmeans #1.3\n";
                if(id_old_cluster != -1)
                    clusters[id_old_cluster].removePoint(points[i].getID());

                points[i].setCluster(id_nearest_center);
                clusters[id_nearest_center].addPoint(points[i]);
                done = false;
            }
        }

        cout << "Kmeans #1.4\n";
        // recalculating the center of each cluster
        for(int i = 0; i < K; i++)
        {
            cout << "Kmeans #1.5\n";
            for(int j = 0; j < total_values; j++)
            {
                cout << "Kmeans #1.6\n";
                int total_points_cluster = clusters[i].getTotalPoints();
                double sum = 0.0;

                if(total_points_cluster > 0)
                {
                    cout << "Kmeans #1.7\n";
                    for(int p = 0; p < total_points_cluster; p++)
                        sum += clusters[i].getPoint(p).getValue(j);
                    cout << "Kmeans #1.8\n";
                    clusters[i].setCentralValue(j, sum / total_points_cluster);
                }
            }
        }

        cout << "Kmeans #1.9\n";
        if(done == true || iter >= max_iterations)
        {
            cout << "Kmeans #1.10\n";
            cout << "Break in iteration " << iter << "\n\n";
            table << "Break in iteration " << iter << "\n\n";
            break;
        }

        iter++;
        cout << "Kmeans #1.11\n";
    }

    cout << "Kmeans #2\n";
    // shows elements of clusters
    for(int i = 0; i < K; i++)
    {
        cout << "Kmeans #2.1\n";
        int total_points_cluster =  clusters[i].getTotalPoints();

        cout << "Cluster " << dec << clusters[i].getID() + 1 << endl;
        table << "Cluster " << dec << clusters[i].getID() + 1 << endl;
        for(int j = 0; j < total_points_cluster; j++)
        {
            cout << "Point " << dec << clusters[i].getPoint(j).getID() + 1 << ": ";
            table << "Point " << dec << clusters[i].getPoint(j).getID() + 1 << ": ";
            for(int p = 0; p < total_values; p++) {
                cout << hex << clusters[i].getPoint(j).getValue(p) << " ";
                table << hex << clusters[i].getPoint(j).getValue(p) << " ";
            }
            string point_name = clusters[i].getPoint(j).getName();

            if(point_name != "") {
                cout << "- " << point_name;
                table << "- " << point_name;
            }

            cout << endl;
            table << endl;
        }

        cout << "Cluster values: ";
        table << "Cluster values: ";

        for(int j = 0; j < total_values; j++) {
            cout << hex << clusters[i].getCentralValue(j) << " ";
            table << hex << clusters[i].getCentralValue(j) << " ";
        }

        cout << "\n\n";
        table << "\n\n";
    }

    table.close();
}

Entries::Entries(int total_points, int total_values, int K, int max_iterations)
{
    this->total_points = total_points;
    this->total_values = total_values;
    this->K = K;
    this->max_iterations = max_iterations;
}

// void Entries::clustering(int total_points, int K, int max_iterations, vector<array<int,64>>& cacheLines)
void Entries::clustering(vector<array<int,64>> const &cacheLines)
{
    srand (time(NULL));

    // Total points = # cache lines (vector size), Total values = 64, K = Precompression table entries, Max iterations = depends, Has name = 0
    // int total_points, total_values = 64, K, max_iterations, has_name = 0;

    vector<Point> points;
    string point_name;

    for (uint i=0; i<cacheLines.size(); i++) {
    // for (auto it = cacheLines.begin(); it != cacheLines.end(); ++it) {

        vector<int> values;

        for (int j=0; j<64; j++) {
            int value = cacheLines[i][j];
            values.push_back(value);
        }

        Point p(i, values);
        points.push_back(p);
    }

    KMeans kmeans(K, total_points, total_values, max_iterations);
    kmeans.run(points);
}