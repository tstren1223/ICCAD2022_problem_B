#ifndef _IO_IO_H_
#define _IO_IO_H_



#include <string>
#include<vector>
#include<map>
#include <semaphore.h>
namespace io {
class IOModule{
private:
    static std::string _name;

    void showOptions() const;

public:
    static std::string Format;

    static std::string BookshelfAux;
    static std::string BookshelfPl;
    static std::string BookshelfVariety;
    static std::string BookshelfPlacement;
    static std::string LefTech;
    static std::string LefCell;
    static std::string DefFloorplan;
    static std::string DefCell;
    static std::string DefPlacement;

    static std::string Verilog;
    static std::string Liberty;
    static std::string Size;
    static std::string Constraints;
    
    //ICCAD2022
    static bool load_place;
    static bool TOP;
    static bool ANS;
    static bool GP_check;
public:
    const std::string& name() const { return _name; }
    static bool load();
    static bool save();
    static double* shared_memx; 
    static double* shared_memy;
    static int top_c;
    static int bott_c;
    static pid_t pid;
    static sem_t* end_of_LB, *data_ready;
    static int pins;
    static int cells;
    static int nets;
    static std::vector<std::vector<int>> netCells;
    static std::map<int,int> die_to_g;
    static std::map<int,std::pair<int,int>> g_to_die;
};
}

#endif
