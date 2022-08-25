#include "ripple.h"
void fun(bool die)
{
    if(die==false)
        system("./placer top top"); 
    else
        system("./placer bot bot"); 
}
void Ripple :: ripple(string s)
{   
    system("chmod 755 placer");
    system(s.c_str());
    /*      
    thread t1(fun,false);
    thread t2(fun,true);
    t1.join();
    t2.join();*/

}
