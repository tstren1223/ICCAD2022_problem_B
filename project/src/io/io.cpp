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
bool IOModule::TOP=false;
bool IOModule::ANS=false;

std::string IOModule::LefTech = "";
std::string IOModule::LefCell = "";
std::string IOModule::DefFloorplan = "";
std::string IOModule::DefCell = "";
std::string IOModule::DefPlacement = "";
std::string io::IOModule::Verilog = "";
std::string io::IOModule::Liberty = "";
std::string io::IOModule::Size = "";
std::string io::IOModule::Constraints = "";

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
