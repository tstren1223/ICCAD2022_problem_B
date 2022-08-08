#include "gp_main.h"

#include "../ut/utils.h"
#include "gp_congest.h"
#include "gp_data.h"
#include "gp_global.h"
#include "gp_inflate.h"
#include "gp_qsolve.h"
#include "gp_region.h"
#include "gp_spread.h"
#include "../io/io.h"
#ifdef LIBGD
#include "gp_draw.h"
#endif

using namespace gp;

void placement();
void upperBound(const int iter, const int binSize, const int legalTimes, const int inflateTimes, const double td);

bool enable_keepinflate = false;
// bool enable_fencemode=false;

void gp_main()
{
    unsigned timer_id = utils::Timer::start();

    printlog(LOG_INFO, "#nodes : %d", numNodes);
    printlog(LOG_INFO, "#nets  : %d", numNets);
    printlog(LOG_INFO, "#pins  : %d", numPins);
    printlog(LOG_INFO, "#cells : %d", numCells);
    printlog(LOG_INFO, "#macros: %d", numMacros);
    printlog(LOG_INFO, "#pads  : %d", numPads);
    printlog(LOG_VERBOSE, "#InitIter: %d", GPModule::InitIterations);
    printlog(LOG_VERBOSE, "#WLIter: %d", GPModule::MainWLIterations);
    printlog(LOG_VERBOSE, "#CGIter: %d", GPModule::MainCongIterations);
    printlog(LOG_VERBOSE, "#GRIter: %d", GPModule::MainGRIterations);
    printlog(LOG_INFO, "input wirelength = %ld", (long)hpwl());
    initLowerBound();
    placement();

    printlog(LOG_INFO, "output wirelength = %ld", (long)hpwl());
    printlog(LOG_INFO, "global place runtime = %.2lf", utils::Timer::stop(timer_id));
    utils::Timer::reset(timer_id);
    utils::Timer::stop(timer_id);
}
void fix_cells(int cell_n,vector<double> dummyX_fix,vector<double>dummyY_fix)
{
    double XQ1, XQ3, YQ1, YQ3;
    int debug_n = 0;
    int division = 16;
    dummyX_fix.resize(cell_n);
    dummyY_fix.resize(cell_n);
    nth_element(dummyX_fix.begin(), dummyX_fix.begin() + cell_n / 4, dummyX_fix.end());
    XQ1 = *(dummyX_fix.begin() + cell_n / 4);
    nth_element(dummyX_fix.begin(), dummyX_fix.begin() + cell_n / 4 * 3, dummyX_fix.end());
    XQ3 = *(dummyX_fix.begin() + cell_n / 4 * 3);
    nth_element(dummyY_fix.begin(), dummyY_fix.begin() + cell_n / 4, dummyY_fix.end());
    YQ1 = *(dummyY_fix.begin() + cell_n / 4);
    nth_element(dummyY_fix.begin(), dummyY_fix.begin() + cell_n / 4 * 3, dummyY_fix.end());
    YQ3 = *(dummyY_fix.begin() + cell_n / 4 * 3);
    for (int i = 0; i < cell_n; i++)
    {
        if (cellX[i] <= XQ1 && !io::IOModule::sh.fixed[io::IOModule::die_to_g[i]])
        {
            io::IOModule::sh.fixed[io::IOModule::die_to_g[i]] = 1;
            debug_n++;
            if (debug_n >= cell_n / division)
            {
                break;
            }
        }
    }
    debug_n = 0;
    for (int i = 0; i < cell_n; i++)
    {
        if (cellX[i] >= XQ3 && !io::IOModule::sh.fixed[io::IOModule::die_to_g[i]])
        {
            io::IOModule::sh.fixed[io::IOModule::die_to_g[i]] = 1;
            debug_n++;
            if (debug_n >= cell_n / division)
            {
                break;
            }
        }
    }
    debug_n = 0;
    for (int i = 0; i < cell_n; i++)
    {
        if (cellY[i] >= YQ3 && !io::IOModule::sh.fixed[io::IOModule::die_to_g[i]])
        {
            io::IOModule::sh.fixed[io::IOModule::die_to_g[i]] = 1;
            debug_n++;
            if (debug_n >= cell_n / division)
            {
                break;
            }
        }
    }
    debug_n = 0;
    for (int i = 0; i < cell_n; i++)
    {
        if (cellY[i] <= YQ1 && !io::IOModule::sh.fixed[io::IOModule::die_to_g[i]])
        {
            io::IOModule::sh.fixed[io::IOModule::die_to_g[i]] = 1;
            debug_n++;
            if (debug_n >= cell_n / division)
            {
                break;
            }
        }
    }
}
void merge_lower_bound(int &initIter, NetModel &initNetModel, NetModel &mainNetModel, bool init, double pnWeightBegin = 0, double pnWeightEnd = 0, int iter = 0, int mainIter = 0, int LBIter = 0)
{
    bool sep = false;

    if (!sep)
    {
        int pins = io::IOModule::pins;
        int cells = io::IOModule::cells;
        int nets = io::IOModule::nets;
        if (io::IOModule::pid > 0) // bottom
        {
            for (int i = 0; i < io::IOModule::bott_c; i++)
            {
                io::IOModule::sh.shared_memx[io::IOModule::die_to_g[i]] = cellX[i];
                io::IOModule::sh.shared_memy[io::IOModule::die_to_g[i]] = cellY[i];
                // cout<<"(GB)Put"<<i<<"to "<<io::IOModule::die_to_g[i]<<endl;
            }
            sem_wait(io::IOModule::sh.data_ready);

            vector<double> cellX2, cellY2;
            for (int i = 0; i < cells; i++)
            {
                cellX2.push_back(io::IOModule::sh.shared_memx[i]);
                cellY2.push_back(io::IOModule::sh.shared_memy[i]);
            }
            fix_cells(cells,cellX2,cellY2);
            if (init)
            {
                for (int i = 0; i < cells; i++)
                {
                    io::IOModule::sh.fixed[i] = 0;
                }
                lowerBound(0.001, 0.0, initIter, LBModeFenceBBox, initNetModel, 0.0, -1, cellX2, cellY2, pins, cells, nets, io::IOModule::netCells, io::IOModule::netPinX, io::IOModule::netPinY, io::IOModule::sh.fixed);
            }
            else
            {
                lowerBound(0.001, pnWeightBegin + (pnWeightEnd - pnWeightBegin) * (iter - 1) / (mainIter - 1), LBIter, LBModeFenceRelax, mainNetModel, 0.0, -1, cellX2, cellY2, pins, cells, nets, io::IOModule::netCells, io::IOModule::netPinX, io::IOModule::netPinY, io::IOModule::sh.fixed);
                for (int i = 0; i < cells; i++)
                {
                    io::IOModule::sh.fixed[i] = 0;
                }
            }
            for (int i = 0; i < cells; i++)
            {
                io::IOModule::sh.shared_memx[i] = cellX2[i];
                io::IOModule::sh.shared_memy[i] = cellY2[i];
            }
            sem_post(io::IOModule::sh.end_of_LB);
            for (int i = 0; i < cells; i++)
            {
                if (io::IOModule::g_to_die[i].first == 1)
                {
                    cellX[io::IOModule::g_to_die[i].second] = io::IOModule::sh.shared_memx[i];
                    cellY[io::IOModule::g_to_die[i].second] = io::IOModule::sh.shared_memy[i];
                    // cout<<"(PB)Put"<<i<<"to "<<io::IOModule::g_to_die[i].second<<endl;
                }
            }
            cellX2.clear();
            cellY2.clear();
        }
        else if (io::IOModule::pid == 0) // top
        {
            for (int i = 0; i < io::IOModule::top_c; i++)
            {
                io::IOModule::sh.shared_memx[io::IOModule::die_to_g[i]] = cellX[i];
                io::IOModule::sh.shared_memy[io::IOModule::die_to_g[i]] = cellY[i];
                // cout<<"(GT)Put"<<i<<"to "<<io::IOModule::die_to_g[i]<<endl;
            }
            // cout<<"TOP ready"<<endl;
            sem_post(io::IOModule::sh.data_ready);
            // cout<<"TOP wait"<<endl;
            sem_wait(io::IOModule::sh.end_of_LB);
            // cout<<"GOT!!"<<endl;
            for (int i = 0; i < cells; i++)
            {
                if (io::IOModule::g_to_die[i].first == 0)
                {
                    cellX[io::IOModule::g_to_die[i].second] = io::IOModule::sh.shared_memx[i];
                    cellY[io::IOModule::g_to_die[i].second] = io::IOModule::sh.shared_memy[i];
                    // cout<<"(PT)Put"<<i<<"to "<<io::IOModule::g_to_die[i].second<<endl;
                }
            }
        }
    }
    else
    {
        if (init)
            lowerBound(0.001, 0.0, initIter, LBModeFenceBBox, initNetModel, 0.0, -1, cellX, cellY, numPins, numCells, numNets, netCell, netPinX, netPinY, io::IOModule::sh.fixed);
        else
            lowerBound(0.001, pnWeightBegin + (pnWeightEnd - pnWeightBegin) * (iter - 1) / (mainIter - 1), LBIter, LBModeFenceRelax, mainNetModel, 0.0, -1, cellX, cellY, numPins, numCells, numNets, netCell, netPinX, netPinY, io::IOModule::sh.fixed);
    }
}
void placement()
{
    int initIter = GPModule::InitIterations; // initial CG iteration
    // int initIter     = gpSetting.initIter;    //initial CG iteration
    int mainWLIter = GPModule::MainWLIterations; // old version: 3
    // int mainWLIter   = gpSetting.mainWLIter;    // old version: 3
    int mainCongIter = GPModule::MainCongIterations; // old version: 34
    int mainGRIter = GPModule::MainGRIterations;     // heavy global router iteration
    int LBIter = GPModule::LowerBoundIterations;
    int UBIter = GPModule::UpperBoundIterations;
    int mainIter = mainWLIter + mainCongIter + mainGRIter; // total GP iteration
    int finalIter = GPModule::FinalIterations;
    // int finalIter    = gpSetting.finalIter;
    double pnWeightBegin = GPModule::PseudoNetWeightBegin;
    // double pnWeightBegin = gpSetting.pseudoNetWeightBegin;
    double pnWeightEnd = GPModule::PseudoNetWeightEnd;
    // double pnWeightEnd   = gpSetting.pseudoNetWeightEnd;
    NetModel initNetModel = NetModelNone;
    if (GPModule::InitNetModel == "Clique")
    {
        initNetModel = NetModelClique;
    }
    else if (GPModule::InitNetModel == "Star")
    {
        initNetModel = NetModelStar;
    }
    else if (GPModule::InitNetModel == "B2B")
    {
        initNetModel = NetModelB2B;
    }
    else if (GPModule::InitNetModel == "Hybrid")
    {
        initNetModel = NetModelHybrid;
    }
    else if (GPModule::InitNetModel == "MST")
    {
        initNetModel = NetModelMST;
    }
    NetModel mainNetModel = NetModelNone;
    if (GPModule::MainNetModel == "Clique")
    {
        mainNetModel = NetModelClique;
    }
    else if (GPModule::MainNetModel == "Star")
    {
        mainNetModel = NetModelStar;
    }
    else if (GPModule::MainNetModel == "B2B")
    {
        mainNetModel = NetModelB2B;
    }
    else if (GPModule::MainNetModel == "Hybrid")
    {
        mainNetModel = NetModelHybrid;
    }
    else if (GPModule::MainNetModel == "MST")
    {
        mainNetModel = NetModelMST;
    }

    createInflation();

    if (rippleSetting.force_stop)
    {
        rippleSetting.force_stop = false;
        return;
    }
    if (initIter > 0)
    {
        //--Random placement
        for (int i = 0; i < numCells; ++i)
        {
            cellX[i] = getrand((double)(coreLX + 0.5 * cellW[i]), (double)(coreHX - 0.5 * cellW[i]));
            cellY[i] = getrand((double)(coreLY + 0.5 * cellH[i]), (double)(coreHY - 0.5 * cellH[i]));
        }
        printlog(LOG_INFO, "random wirelength = %lld", (long long)hpwl());
        //--Initial placement (CG)
        merge_lower_bound(initIter, initNetModel, mainNetModel, true);
        printlog(LOG_INFO, "initial wirelength = %lld", (long long)hpwl());
    }
#ifdef LIBGD
    drawcell("gp_initial");
#endif

    //--Global Placement Iteration
    for (int iter = 1; iter <= mainIter; iter++)
    {
        if (rippleSetting.force_stop)
        {
            rippleSetting.force_stop = false;
            break;
        }
        findCellRegionDist();
        legalizeRegion();

        //--Upper Bound
        const double td{GPModule::TargetDensityBegin + (GPModule::TargetDensityEnd - GPModule::TargetDensityBegin) * (iter - 1) / (mainIter - 1)};
        if (iter <= mainWLIter)
        {
            upperBound(iter, 8, UBIter, 0, td);
        }
        else if (iter <= mainWLIter + mainCongIter)
        {
            grSetting.router = GRSetting::GRRouter::RippleGR;
            upperBound(iter, 4, UBIter, 1, td);
        }
        else
        {
            grSetting.router = GRSetting::GRRouter::CUGR;
            upperBound(iter, 4, UBIter, 1, td);
        }
        long upHPWL = hpwl();

#ifdef LIBGD
        drawcell("gp_up_", iter);
#endif

        //--Lower Bound
        merge_lower_bound(initIter, initNetModel, mainNetModel, false, pnWeightBegin, pnWeightEnd, iter, mainIter, LBIter);
        long loHPWL = hpwl();

#ifdef LIBGD
        drawcell("gp_lo_", iter);
#endif

        printlog(LOG_INFO, "[%2d] up=%ld lo=%ld", iter, upHPWL, loHPWL);
    }

    upperBound(mainIter + 1, 4, finalIter, 0, GPModule::TargetDensityEnd);
    printlog(LOG_INFO, "final=%ld", (long)hpwl());
#ifdef LIBGD
    drawcell("gp_final");
#endif

    // copy(cellAW.begin(), cellAW.end(), cellW.begin());
    // copy(cellAH.begin(), cellAH.end(), cellH.begin());
    // updateCongMap();
}

void upperBound(const int iter, const int binSize, const int legalTimes, const int inflateTimes, const double td)
{
    spreadCells(binSize, legalTimes, td);

    double restoreRatio = 1.0;
    if (iter < 10)
    {
        restoreRatio = 0.6;
    }
    else
    {
        restoreRatio = 0.3;
    }

    bool inflated = true;
    for (int i = 0; i < inflateTimes && inflated; i++)
    {
        updateCongMap();
        if (enable_keepinflate)
        {
            inflated = cellInflation(1, restoreRatio);
        }
        else
        {
            inflated = cellInflation(1, -1.0);
        }
        if (inflated)
        {
            spreadCells(binSize, 1, td);
        }
    }
    // updateCongMap();

    if (!enable_keepinflate)
    {
        copy(cellAW.begin(), cellAW.end(), cellW.begin());
        copy(cellAH.begin(), cellAH.end(), cellH.begin());
    }
}
