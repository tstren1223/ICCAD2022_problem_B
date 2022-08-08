#ifndef _IO_IO_H_
#define _IO_IO_H_


#include <string>
#include <vector>
#include <map>
#include <semaphore.h>
namespace io
{
    class shared
    {
    public:
    
        static sem_t *end_of_LB, *data_ready;
        double *shared_memx;
        double *shared_memy;
        int *fixed;
        int shmid, shmid2, shmid3, shmid4, shmid5;
        shared();
        void ini(int cells);
        std::string end(pid_t p);
    };
    class IOModule
    {
    private:
        static std::string _name;

        void showOptions() const;

    public:
        static std::string Format;

        static std::string BookshelfAux;
        static std::string BookshelfPl;
        static std::string BookshelfVariety;
        static std::string BookshelfPlacement;
        static std::string DefFloorplan;
        static std::string DefCell;
        static std::string DefPlacement;

        // ICCAD2022
        static bool load_par;
        static bool load_GP;
        static bool load_place;
        static bool TOP;
        static bool ANS;
        static bool GP_check;
        static bool statics;

    public:
        const std::string &name() const { return _name; }
        static bool load();
        static bool save();
        static shared sh;
        static int top_c;
        static int bott_c;
        static pid_t pid;
        static int pins;
        static int cells;
        static int nets;
        static std::vector<std::vector<int>> netCells;
        static std::map<int, int> die_to_g;
        static std::map<int, std::pair<int, int>> g_to_die;
        static std::vector<std::vector<double>> netPinX;
        static std::vector<std::vector<double>> netPinY;
    };
}

#endif
