#include "io.h"

#include "../db/db.h"
#include "../global.h"

using namespace io;
std::string IOModule::_name = "io";

std::string IOModule::Format = "lefdef";
std::string IOModule::BookshelfAux = "";
std::string IOModule::BookshelfPl = "1";
std::string IOModule::BookshelfVariety = "ICCAD2022";
std::string IOModule::BookshelfPlacement = "";
// ICCAD2022
bool IOModule::load_place=false;
bool IOModule::GP_check=false;
bool IOModule::TOP=false;
bool IOModule::ANS=false;
double* IOModule::shared_memx=NULL;
double* IOModule::shared_memy=NULL;
int IOModule::top_c=0;
int IOModule::bott_c=0;
pid_t IOModule::pid=0;
int IOModule::pins=0;
int IOModule::cells=0;
int IOModule::nets=0;
std::string IOModule::LefTech = "";
std::string IOModule::LefCell = "";
std::string IOModule::DefFloorplan = "";
std::string IOModule::DefCell = "";
std::string IOModule::DefPlacement = "";
std::string io::IOModule::Verilog = "";
std::string io::IOModule::Liberty = "";
std::string io::IOModule::Size = "";
std::string io::IOModule::Constraints = "";
std::vector<std::vector<int>> io::IOModule::netCells;
std::map<int,int> io::IOModule::die_to_g;
std::map<int,std::pair<int,int>> io::IOModule::g_to_die;
sem_t* io::IOModule::end_of_LB=NULL;
sem_t* io::IOModule::data_ready=NULL;
void IOModule::showOptions() const
{
    printlog(LOG_INFO, "format              : %s", IOModule::Format.c_str());
    printlog(LOG_INFO, "bookshelfAux        : %s", IOModule::BookshelfAux.c_str());
    printlog(LOG_INFO, "bookshelfPl         : %s", IOModule::BookshelfPl.c_str());
    printlog(LOG_INFO, "bookshelfVariety    : %s", IOModule::BookshelfVariety.c_str());
    printlog(LOG_INFO, "lefTech             : %s", IOModule::LefTech.c_str());
    printlog(LOG_INFO, "lefCell             : %s", IOModule::LefCell.c_str());
    printlog(LOG_INFO, "defFloorplan        : %s", IOModule::DefFloorplan.c_str());
    printlog(LOG_INFO, "defCell             : %s", IOModule::DefCell.c_str());
    printlog(LOG_INFO, "defPlacement        : %s", IOModule::DefPlacement.c_str());
    printlog(LOG_INFO, "verilog             : %s", IOModule::Verilog.c_str());
    printlog(LOG_INFO, "liberty             : %s", IOModule::Liberty.c_str());
    printlog(LOG_INFO, "size                : %s", IOModule::Size.c_str());
    printlog(LOG_INFO, "constraints         : %s", IOModule::Constraints.c_str());
}

bool io::IOModule::load()
{
    if (BookshelfAux.length() > 0 && BookshelfPl.length())
    {
        Format = "bookshelf";
        database.readICCAD2022_setup(BookshelfAux);
    }
    return true;
}


bool io::IOModule::save()
{
    if (Format == "bookshelf" && BookshelfPlacement.size())
    {
        database.writeICCAD2022(BookshelfPlacement);
    }
    return true;
}
