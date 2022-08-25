#ifndef AddTerminal_H
#define AddTerminal_H

#include <bits/stdc++.h>
#include "data.h"
using namespace std;

class AddTerminal{
  public:
    void output();
    AddTerminal(){};
    void terminal_insert();
  private:
    int grid_row, grid_col;
    map<int,pair<int,int>> terminals_map;
    vector<vector<int>> terminals_grid;
    vector<vector<int>> area_vec = {{0,-1},{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1}};
    vector<int> dir_vec = {1,0,-1,0,1}; 
  private:        
    void findCenter(int,int,int&,int&);
    void terminal_BFS(int,int);
};  

//---------------------------------------------------------------------------
#endif
