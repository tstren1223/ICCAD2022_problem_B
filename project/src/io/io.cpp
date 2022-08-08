#include "io.h"

#include "../db/db.h"
#include "../global.h"

#include <error.h>
#include <sys/shm.h>
#include <sys/ipc.h>
using namespace io;
std::string IOModule::_name = "io";

std::string IOModule::Format = "lefdef";
std::string IOModule::BookshelfAux = "";
std::string IOModule::BookshelfPl = "1";
std::string IOModule::BookshelfVariety = "ICCAD2022";
std::string IOModule::BookshelfPlacement = "";
// ICCAD2022
bool IOModule::load_par = false;
bool IOModule::load_GP= false;
bool IOModule::load_place = false;
bool IOModule::GP_check = false;
bool IOModule::TOP = false;
bool IOModule::ANS = false;
bool IOModule::statics = false;
shared IOModule::sh;
sem_t *io::shared::data_ready;
sem_t *io::shared::end_of_LB;
int IOModule::top_c = 0;
int IOModule::bott_c = 0;
pid_t IOModule::pid = 0;
int IOModule::pins = 0;
int IOModule::cells = 0;
int IOModule::nets = 0;
std::string IOModule::DefFloorplan;
std::string IOModule::DefCell;
std::string IOModule::DefPlacement;
std::vector<std::vector<int>> io::IOModule::netCells;
std::vector<std::vector<double>> io::IOModule::netPinX;
std::vector<std::vector<double>> io::IOModule::netPinY;
std::map<int, int> io::IOModule::die_to_g;
std::map<int, std::pair<int, int>> io::IOModule::g_to_die;
void IOModule::showOptions() const
{
    printlog(LOG_INFO, "format              : %s", IOModule::Format.c_str());
    printlog(LOG_INFO, "bookshelfAux        : %s", IOModule::BookshelfAux.c_str());
    printlog(LOG_INFO, "bookshelfPl         : %s", IOModule::BookshelfPl.c_str());
    printlog(LOG_INFO, "bookshelfVariety    : %s", IOModule::BookshelfVariety.c_str());
    printlog(LOG_INFO, "defFloorplan        : %s", IOModule::DefFloorplan.c_str());
    printlog(LOG_INFO, "defCell             : %s", IOModule::DefCell.c_str());
    printlog(LOG_INFO, "defPlacement        : %s", IOModule::DefPlacement.c_str());
}
io::shared::shared()
{
    end_of_LB = NULL;
    data_ready = NULL;
    shared_memx = NULL;
    shared_memy = NULL;
    fixed = NULL;
}
void io::shared::ini(int cells)
{
    if ((shmid = shmget(0, sizeof(double) * cells, IPC_CREAT | 0600)) == -1)
    {
        cout << "shmid error" << endl;
        char buffer[256];
        strerror_r(errno, buffer, 256); // get string message from errno, XSI-compliant version
        printf("Error %s\n", buffer);
        // or
        char *errorMsg = strerror_r(errno, buffer, 256); // GNU-specific version, Linux default
        printf("Error %s\n", errorMsg);                    // return value has to be used since buffer might not be modified
                                                         // ...
    }
    if ((shmid2 = shmget(0, sizeof(double) * cells, IPC_CREAT | 0600)) == -1)
        cout << "shmid2 error" << endl;
    if ((shmid3 = shmget(0, sizeof(sem_t), IPC_CREAT | 0600)) == -1)
        cout << "shmid3 error" << endl;
    if ((shmid4 = shmget(0, sizeof(sem_t), IPC_CREAT | 0600)) == -1)
        cout << "shmid4 error" << endl;
    if ((shmid5 = shmget(0, sizeof(int) * cells, IPC_CREAT | 0600)) == -1)
        cout << "shmid5 error" << endl;

    shared_memx = (double *)shmat(shmid, NULL, 0);
    shared_memy = (double *)shmat(shmid2, NULL, 0);
    end_of_LB = (sem_t *)shmat(shmid3, NULL, 0);
    data_ready = (sem_t *)shmat(shmid4, NULL, 0);
    fixed = (int *)shmat(shmid5, NULL, 0);
}
std::string io::shared::end(pid_t p)
{
    std::string s = "";
    if (p == 0)
    {
        shmdt(end_of_LB);
        shmdt(data_ready);
        shmdt(shared_memx);
        shmdt(shared_memy);
        shmdt(fixed);
    }
    else if (p > 0)
    {
        if (sem_destroy(data_ready))
            s += "sem1 error\n";
        if (sem_destroy(end_of_LB))
            s += "sem2 error\n";
        if (shmctl(shmid, IPC_RMID, NULL))
            s += "S_mem1 error\n";
        if (shmctl(shmid2, IPC_RMID, NULL))
            s += "S_mem2 error\n";
        if (shmctl(shmid3, IPC_RMID, NULL))
            s += "S_mem3 error\n";
        if (shmctl(shmid4, IPC_RMID, NULL))
            s += "S_mem4 error\n";
        if (shmctl(shmid5, IPC_RMID, NULL))
            s += "S_mem5 error\n";
    }
    return s;
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
