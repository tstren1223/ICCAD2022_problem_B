#ifndef DATA_H
#define DATA_H
#include "bits/stdc++.h"
using namespace std;
struct Cell
{
    int width,height;
    long long area;
    vector<pair<int,int>> pins;
};
struct Tech
{    
    // <cell_name,Cell>
    unordered_map<string,Cell> cells_map;
};
struct Terminal
{
    int width,height,spacing;    
};
struct Net
{
    int pin_num;  
    vector<pair<int,int>> pins;  // <instance_idx,pin_idx>
};
struct Instances_infro
{
    // 0 for top Cell_type , 1 for bottom Cell_type
    long long area[2];
    int width[2];
    int height[2];
    vector<vector<pair<int,int>>> pins;
};
struct Instance
{
    string type;
    bool die; // false = top true = bottom
    int x,y;  // bottom left x,y          
};
struct Die_infro
{
    int MaxUtil;  
    int rowHeight,rowN;  
    long width,height;
    int cells_num;
    long long area;
};
class Data
{
    private: 
        void calSite();
        void Instances_infro_create();               
    public :          
        void read(string,string);
        void separate();
        void update();
    public:           
        void testInput(); 
        int Cells_N,Nets_N;
        int grid_row,grid_col;  
        string in_filename,out_filename;       
        vector<int> which_techs;
        vector<Tech> techs;
        Die_infro dies_infro[2];  // 0 for top,1 for bottom
    // net
        vector<Net> nets;  // <Net>
        unordered_map<int,unordered_set<int>> node_net;        
    // instance
        vector<Instance> instances;  // <Instance> 
        vector<Instances_infro> instances_infro;
    // terminal                   
        Terminal terminal_infro;  
        unordered_map<int,int> crossNets_pair;
        vector<int> top_nodes;
        vector<int> bottom_nodes;                                                    
    // siteWH
        int siteW[2],siteH[2];
};

extern Data database;
bool rand_bool();
double rand_01();       
#endif