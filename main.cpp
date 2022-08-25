#include<bits/stdc++.h>
#include "data.h"
#include "Fm.h"
#include "ripple.h"
#include "terminal.h"
#include "testing.h"
using namespace std;
int main(int argc,char *argv[])
{         
    //-------------------------------
    cout<<"read input data & setting database"<<endl;
    //Data data;
    database.read(string(argv[1]),string(argv[2]));   
    //database.testInput();
    //------------------------------
    
    //-------------------------------
    cout<<"hmetis partition"<<endl;
    FM fm;
    if(argc<=3||strcmp(argv[3],"-load_par")){
        fm.partition();
    }else{
        fm.reload();
    }
    //fm.testPartition();
    database.separate();
    //------------------------------

    //------------------------------
    // ripple placer for top and bottom
    //-------------------------------
    cout<<"ripple placer for top and bottom"<<endl;
    Ripple ripple;
    ripple.ripple("./placer "+string(argv[1])+" "+string(argv[2])+" -dep");
    
    database.update();
    //-----------------------------
    cout<<"terminals_insert"<<endl;
    AddTerminal terminal;
    terminal.terminal_insert();
    terminal.output();
    //-----------------------------    
    cout<<"complete"<<endl;
    Testing test;
    test.testing(string(argv[1]),string(argv[2]));
    return 0;
}