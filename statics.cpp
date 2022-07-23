#include<iostream>
#include<fstream>
#include<algorithm>
using namespace std;
int main (int argc, char *argv[]){
    string file=argv[1];
    string iter=argv[2];
    for(int i=0;i<stoi(iter);i++){
        system(("./placer "+file+" out.txt -statics > /dev/null").c_str());
    }
    long long results[stoi(iter)];
    ifstream is("statics.txt");
    for(int i=0;i<stoi(iter);i++){
        is>>results[i];
    }
    long long max=*max_element(&results[0],&results[stoi(iter)]);
    long long min=*min_element(&results[0],&results[stoi(iter)]);
    long double avg=0;
    for(int i=0;i<stoi(iter);i++){
        avg+=results[i];
    }
    avg/=stoi(iter);
    cout<<"Statics of "<<iter<<" iterations of file "<<file<<" is:"<<endl;
    cout<<"MAX:"<<max<<endl;
    cout<<"MIN:"<<min<<endl;
    cout<<"AVG:"<<avg<<endl;
    system("rm statics.txt");
    return 0;
}