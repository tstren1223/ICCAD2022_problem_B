#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;
void check_evaluator(string s, int &error)
{
    ifstream ifile(s);
    int done = 0;
    string sss;
    bool sum = false;
    while (ifile)
    {
        ifile >> sss;
        if (sss == "Done")
        {
            done++;
        }
        if (sss == "Summery")
            sum = true;
    }
    if (done != 6 || !sum)
    {
        error += 1;
        system(("cp " + s + " error" + std::to_string(error) + ".txt").c_str());
    }
}
void stat(string s1,string s2,string iter,int error,string file){
    long long results[stoi(iter)];
    ifstream is(s1);
    for (int i = 0; i < stoi(iter); i++)
    {
        is >> results[i];
    }
    long long max = *max_element(&results[0], &results[stoi(iter)]);
    long long min = *min_element(&results[0], &results[stoi(iter)]);
    long double avg = 0;
    for (int i = 0; i < stoi(iter); i++)
    {
        avg += results[i];
    }
    avg /= stoi(iter);
    cout << "Statics of " << iter << " iterations of file " << file << " is:" << endl;
    cout << "MAX:" << max << endl;
    cout << "MIN:" << min << endl;
    cout << "AVG:" << avg << endl;
    cout << "err:" << error << endl;
    system(("rm "+s1).c_str());
    system(("rm "+s2).c_str());
}
int main(int argc, char *argv[])
{
    string file = argv[1];
    string iter = argv[2];
    int error = 0;
    string following;
    bool two = false;
    if (argc>3&&string(argv[3]) == "-debug")
    {
        two = true;
    }
    for (int i = 3; i < argc; i++)
    {
        if (two && i == 3)
            continue;
        following += string(argv[i]) + " ";
    }
    for (int i = 0; i < stoi(iter); i++)
    {
        system(("./placer " + file + " out.txt -statics " + following + "> /dev/null").c_str());
        if (two)
            system(("./placer " + file + " out_1.txt -statics -debug -load_par" + following + "> /dev/null").c_str());
        system(("./evaluator " + file + " out.txt > temp.txt").c_str());
        check_evaluator("temp.txt",error);
        if (two){
            system(("./evaluator " + file + " out_1.txt > temp2.txt").c_str());
            check_evaluator("temp2.txt",error);
        }
    }
    stat("out.txt_statics.txt","temp.txt",iter,error,file);
    if(two)
        stat("out_1.txt_statics.txt","temp2.txt",iter,error,file);
    return 0;
}