#include "testing.h"
void Testing :: testing(string in,string out)
{
    system("chmod 755 evaluator");
    string str = "./evaluator "+in+" "+out;
    system(str.c_str());
}