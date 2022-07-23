#include "../db/db.h"
#include "../dp/dp.h"
#include "../global.h"
#include "../gp/gp.h"
#include "../gr/gr.h"
#include "../io/io.h"
using namespace db;

#include "ripple.h"
#include "ripple_data.h"
using namespace ripple;

Ripple *Ripple::_instance = nullptr;

Ripple *Ripple::get()
{
    if (!Ripple::_instance)
    {
        Ripple::_instance = new Ripple;
    }
    return _instance;
}

int Ripple::_run(int argc, char **argv)
{
    init_log(LOG_ALL ^ LOG_VERBOSE);
    // init_log(LOG_ALL);

    printlog(LOG_INFO, "-----------------------------------");
    printlog(LOG_INFO, "  Ripple - placement tool          ");
    printlog(LOG_INFO, "  Chinese University of Hong Kong  ");
    printlog(LOG_INFO, "-----------------------------------");

    if (getArgs(argc, argv))
    {
        printlog(LOG_ERROR, "get command line argument fail");
    }
    else
    {
        rippleSetting.flow = RippleSetting::PlaceFlow::ICCAD2022;

        gp::GPModule::MainGRIterations = 0;
        io::IOModule *io_m=new io::IOModule;
#ifdef __GP__
        gp::GPModule *gp_m=new gp::GPModule;
#endif
        dp::DPModule *dp_m=new dp::DPModule;
        db::DBModule::EdgeSpacing = true;
        db::DBModule::EnablePG = false;
        db::DBModule::EnableIOPin = false;
        // top
        io_m->load();
        if (io::IOModule::load_place == false)
        {
            database.setup();
            database.errorCheck();
            printlog(LOG_INFO,
                     "wirelength = %.2lf (scale=%.2lf)",
                     (double)database.getHPWL() / (double)database.siteW,
                     (double)database.siteW);
            #ifdef __GP__
            io::IOModule::GP_check=true;
            gp_m->gplace();
            io_m->save();
            #endif
            printlog(LOG_INFO,
                     "wirelength = %.2lf (scale=%.2lf)",
                     (double)database.getHPWL() / (double)database.siteW,
                     (double)database.siteW);
            dp::DPModule::MaxDisp = database.maxDisp * database.siteH / database.siteW;
            dp::DPModule::MaxDensity = database.maxDensity;
            dp_m->dplace();
        }
        io_m->save();
    }

    printlog(LOG_INFO, "-----------------------------------");
    printlog(LOG_INFO, "  Ripple terminating...");
    printlog(LOG_INFO, "-----------------------------------");

    return 0;
}

int Ripple::_abort()
{
    rippleSetting.force_stop = true;
    if (currentStatus == NULL)
    {
        return 0;
    }
    return 1;
}

ProcessStatus::RunStatus Ripple::getStatus()
{
    if (currentStatus == NULL)
    {
        return ProcessStatus::RunStatus::NotStarted;
    }
    return currentStatus->status;
}

int Ripple::getArgs(int argc, char **argv)
{
    io::IOModule::BookshelfPlacement = "placed.pl";
    db::DBModule::EdgeSpacing = true;
    // setting.user_target_density=-1.0;
    for (int a = 1; a < argc; a++)
    {
        if (strcmp(argv[a], "-bookshelf") == 0)
        {
            string type(argv[++a]);
            if (type == "wu2016")
            {
                io::IOModule::BookshelfVariety = "wu2016";
            }
            else if (type == "iccad2022")
            {
                io::IOModule::BookshelfVariety = "ICCAD2022";
            }
            else
            {
                printlog(LOG_ERROR, "unknown bookshelf format %s", type.c_str());
                return 1;
            }
        }
        else if (strcmp(argv[a], "-output") == 0)
        {
            ++a;
            io::IOModule::BookshelfPlacement.assign(argv[a]);
        }
        else if (strcmp(argv[a], "-aux") == 0)
        {
            io::IOModule::BookshelfAux.assign(argv[++a]);
        }
        else if (strcmp(argv[a], "-pl") == 0)
        {
            io::IOModule::BookshelfPl.assign(argv[++a]);
        }
        else if (strcmp(argv[a], "-targetdensity") == 0)
        {
            ++a;
        }
#ifdef __GP__
        else if (!strcmp(argv[a], "-gp_num_init_iter"))
        {
            gp::GPModule::InitIterations = atoi(argv[++a]);
        }
        else if (!strcmp(argv[a], "-gp_num_wl_iter"))
        {
            gp::GPModule::MainWLIterations = atoi(argv[++a]);
        }
        else if (!strcmp(argv[a], "-gp_num_cg_iter"))
        {
            gp::GPModule::MainCongIterations = atoi(argv[++a]);
        }
        else if (!strcmp(argv[a], "-gp_num_gr_iter"))
        {
            gp::GPModule::MainGRIterations = atoi(argv[++a]);
        }
        else if (!strcmp(argv[a], "-gp_pnweight_begin"))
        {
            gp::GPModule::PseudoNetWeightBegin = atof(argv[++a]);
        }
        else if (!strcmp(argv[a], "-gp_pnweight_end"))
        {
            gp::GPModule::PseudoNetWeightEnd = atof(argv[++a]);
        }
#endif
        else if (!strcmp(argv[a], "-cpu"))
        {
            dp::DPModule::CPU = atoi(argv[++a]);
        }
        else if (!strcmp(argv[a], "-eval_def"))
        {
            dp::DPModule::DefEval.assign(argv[++a]);
        }
        else if (strcmp(argv[a], "-flow") == 0)
        {
            string flow(argv[++a]);
            if (flow == "iccad2017")
            {
                rippleSetting.flow = RippleSetting::PlaceFlow::ICCAD2017;
            }
            else if (flow == "dac2016")
            {
                rippleSetting.flow = RippleSetting::PlaceFlow::DAC2016;
            }
            else if (flow == "eval")
            {
                rippleSetting.flow = RippleSetting::PlaceFlow::Eval;
            }
            else if (flow == "default")
            {
                rippleSetting.flow = RippleSetting::PlaceFlow::Default;
            }
            else if (flow == "iccad2022")
            {
                rippleSetting.flow = RippleSetting::PlaceFlow::ICCAD2022;
            }
            else
            {
                printlog(LOG_ERROR, "unknown placement flow: %s", flow.c_str());
                return 1;
            }
        }
        else if (strcmp(argv[a], "-load_place") == 0)
        {
            io::IOModule::load_place = true;
        }
        else if (strcmp(argv[a], "-top") == 0)
        {
            io::IOModule::TOP = true;
        }
        else if (strcmp(argv[a], "-ans") == 0)
        {
            io::IOModule::ANS = true;
        }
        else if (strcmp(argv[a], "-statics") == 0)
        {
            io::IOModule::statics = true;
        }
        else if (argv[a][0] == '-')
        {
            if (!strcmp(argv[a], "-EnablePinAccRpt"))
            {
                dp::DPModule::EnablePinAccRpt = true;
            }
            else if (!strcmp(argv[a], "-IgnoreEdgeSpacing"))
            {
                db::DBModule::EdgeSpacing = false;
            }
            else if (!strcmp(argv[a], "-IgnoreFence"))
            {
                db::DBModule::EnableFence = false;
            }
            else if (!strcmp(argv[a], "-LGOperRegSize"))
            {
                dp::DPModule::LGOperRegSize = atoi(argv[++a]);
            }
            else if (!strcmp(argv[a], "-GMEnable"))
            {
                dp::DPModule::GMEnable = true;
            }
            else if (!strcmp(argv[a], "-MLLAccuracy"))
            {
                dp::DPModule::MLLAccuracy = atoi(argv[++a]);
            }
            else if (strcmp(argv[a], "-MLLDispFromInput") == 0)
            {
                dp::DPModule::MLLDispFromInput = true;
            }
            else if (!strcmp(argv[a], "-MLLMaxDensity"))
            {
                dp::DPModule::MLLMaxDensity = atoi(argv[++a]);
            }
            else if (!strcmp(argv[a], "-MLLPinCost"))
            {
                dp::DPModule::MLLPinCost = atoi(argv[++a]);
            }
            else if (!strcmp(argv[a], "-MLLUseILP"))
            {
                dp::DPModule::MLLUseILP = true;
            }
            else
            {
                printlog(LOG_ERROR, "unknown option: %s", argv[a]);
            }
        }
        else
        {
            if (a == 1)
            {
                io::IOModule::BookshelfAux.assign(argv[a]);
                continue;
            }
            else if (a == 2)
            {
                io::IOModule::BookshelfPlacement.assign(argv[a]);
                continue;
            }
            printlog(LOG_ERROR, "unknown parameter: %s", argv[a]);
            return 1;
        }
    }
    return 0;
}

ripple::RippleSetting rippleSetting;
