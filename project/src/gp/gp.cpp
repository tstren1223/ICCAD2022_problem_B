#include "gp.h"
#include "../global.h"
//#include "../tcl/register.h"
#include "gp_data.h"
#include "gp_global.h"
#include "gp_main.h"

using namespace gp;

std::string GPModule::_name = "gp";

int GPModule::NumThreads = 8;
int GPModule::InitIterations = 6; // xbar: 0
int GPModule::MainWLIterations = 40;
int GPModule::MainCongIterations = 0;//40
int GPModule::MainGRIterations = 4;
int GPModule::LowerBoundIterations = 6;
int GPModule::UpperBoundIterations = 5;
int GPModule::FinalIterations = 1;

double GPModule::PseudoNetWeightBegin = 0.1;
double GPModule::PseudoNetWeightEnd = 2.0;
double GPModule::TargetDensityBegin = 1;
double GPModule::TargetDensityEnd = 1;
std::string GPModule::InitNetModel = "B2B";
std::string GPModule::MainNetModel = "B2B";

bool GPModule::EnableFence = true;
bool GPModule::EnableKeepInflate = false;
bool GPModule::Experimental = false;

void GPModule::showOptions() const {
    printlog(LOG_INFO, "numThreads           : %d", NumThreads);
    printlog(LOG_INFO, "initIter             : %d", InitIterations);
    printlog(LOG_INFO, "mainWireIter         : %d", MainWLIterations);
    printlog(LOG_INFO, "mainCongIter         : %d", MainCongIterations);
    printlog(LOG_INFO, "mainGRIter           : %d", MainGRIterations);
    printlog(LOG_INFO, "lbIter               : %d", LowerBoundIterations);
    printlog(LOG_INFO, "ubIter               : %d", UpperBoundIterations);
    printlog(LOG_INFO, "finalIter            : %d", FinalIterations);
    printlog(LOG_INFO, "pseudoNetWeightBegin : %.2lf", PseudoNetWeightBegin);
    printlog(LOG_INFO, "pseudoNetWeightEnd   : %.2lf", PseudoNetWeightEnd);
    printlog(LOG_INFO, "targetDensityBegin   : %.2lf", TargetDensityBegin);
    printlog(LOG_INFO, "targetDensityEnd     : %.2lf", TargetDensityEnd);
    printlog(LOG_INFO, "initNetModel         : %s", InitNetModel.c_str());
    printlog(LOG_INFO, "mainNetModel         : %s", MainNetModel.c_str());
    printlog(LOG_INFO, "enableFence          : %s", EnableFence ? "true" : "false");
    printlog(LOG_INFO, "enableKeepInflate    : %s", EnableKeepInflate ? "true" : "false");
    printlog(LOG_INFO, "experimental         : %s", Experimental ? "true" : "false");
}


bool GPModule::gplace() {
    gp_copy_in(true);
    gp_main();
    gp_copy_out();
    return true;
}
