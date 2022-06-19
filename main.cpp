#include "bits/stdc++.h"
#include<iostream>
using namespace std;
int main(int argc, char **argv){
    if (system("chmod 755 placer") == -1)
        cerr<<"fail to execute chmod 755 placer";
    string infile=argv[1],outfile=argv[2];
    string command="./placer "+infile+" "+outfile;
    if (system((command+" -top").c_str()) == -1)
        cerr<<"fail to execute top placer";
    if (system((command+" -ans").c_str()) == -1)
        cerr<<"fail to execute bottom placer";
    return 0;
}