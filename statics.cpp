#include<iostream>
#include<fstream>
#include<algorithm>
using namespace std;
int main (int argc, char *argv[]){
    string file=argv[1];
    string iter=argv[2];
    int error=0;
    for(int i=0;i<stoi(iter);i++){
        system(("./placer "+file+" out.txt -statics > /dev/null").c_str());
        system(("./evaluator "+file+" out.txt -statics > temp.txt").c_str());
        ifstream ifile("temp.txt");
        int done=0;
        string sss;
        bool sum=false;
        while(ifile){
            ifile>>sss;
            if(sss=="Done"){
                done++;
            }
            if(sss=="Summery")
                sum=true;
        }
        if(done!=6||!sum){
            error+=1;
            system(("cp "+string("temp.txt")+" error"+std::to_string(error)+".txt").c_str());
        }
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
    cout<<"err:"<<error<<endl;
    system("rm statics.txt");
    system("rm temp.txt");
    return 0;
}