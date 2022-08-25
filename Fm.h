#ifndef FM_H
#define FM_H
#include "data.h"
#include "bits/stdc++.h"
using namespace std;
struct part_Cell
{  
    int gain = 0;
    bool lock = false;
    bool part;  // 0 for part1 1 for part2
};
struct part_Net
{
    unordered_set<int> part0;
    unordered_set<int> part1;
    int size[2] = {0,0};
};

class FM
{
private:    
    long long total_area[2];
    vector<part_Cell> cells;   
    unordered_map<int,part_Net> nets_infro;
    map<int,unordered_set<int>,greater<int>> gain_map;        
public:         
    int CrossNets_N;
    FM(){
        total_area[0] = 0;
        total_area[1] = 0;
    };
    void area_update(int);    
    int area_constraint();
    void partition();
    void fmMove();  
    void gain_update(int);  
    void buckets_create();
    void ans(); 
    void testPartition(); 
    void area_adjust();
    void net();
    void read_net();
    void reload();
};
#endif

