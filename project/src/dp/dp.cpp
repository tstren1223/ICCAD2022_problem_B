#include "dp.h"
#include "dp_data.h"
#include "dp_global.h"
using namespace dp;

DPlacer* DPModule::dplacer = nullptr;

std::string DPModule::_name = "dp";

unsigned DPModule::CPU = 8;
unsigned DPModule::MaxDisp = INT_MAX;
double DPModule::MaxDensity = 1.0;
string DPModule::DefEval = "";
int DPModule::MaxLGIter = 300;
int DPModule::MaxGMIter = 100;
int DPModule::MaxLMIter = 100;
int DPModule::MaxMPIter = 20;
int DPModule::LGStepSize = 10;
int DPModule::LGThresholdBase = 0;
int DPModule::LGThresholdStep = 20;
unsigned DPModule::LGTrial = 1;
unsigned DPModule::LGOperRegSize = 256;
bool DPModule::GMEnable = false;
unsigned DPModule::GMThresholdBase = 10;
unsigned DPModule::GMThresholdStep = 10;
int DPModule::GMTrial = 1;
int DPModule::LMThresholdBase = 10;
int DPModule::LMThresholdStep = 10;
int DPModule::LMTrial = 1;
bool DPModule::EnableLocalVert = true;
bool DPModule::EnableLocalReorder = true;
bool DPModule::EnableLocalCompact = true;
bool DPModule::EnablePinAcc = true;
bool DPModule::EnablePinAccRpt = false;

int DPModule::MLLAccuracy = 10000;
bool DPModule::MLLDispFromInput = false;
bool DPModule::MLLPGAlign = true;
unsigned DPModule::MLLPinCost = 1000;
unsigned DPModule::MLLMaxDensity = 1000;
bool DPModule::MLLUseILP = false;
bool DPModule::MLLTotalDisp = false;


void DPModule::showOptions() const {
    printlog(LOG_INFO, "cpu                : %u", CPU);
    printlog(LOG_INFO, "maxDisp            : %u", MaxDisp);
    printlog(LOG_INFO, "maxDensity         : %.3lf", MaxDensity);
    printlog(LOG_INFO, "defEval            : %s", DefEval.c_str());
    printlog(LOG_INFO, "maxLGIter          : %d", MaxLGIter);
    printlog(LOG_INFO, "maxGMIter          : %d", MaxGMIter);
    printlog(LOG_INFO, "maxLMIter          : %d", MaxLMIter);
    printlog(LOG_INFO, "maxMPIter          : %d", MaxMPIter);
    printlog(LOG_INFO, "LGStepSize         : %d", LGStepSize);
    printlog(LOG_INFO, "LGThresholdBase    : %d", LGThresholdBase);
    printlog(LOG_INFO, "LGThresholdStep    : %d", LGThresholdStep);
    printlog(LOG_INFO, "LGTrial            : %u", LGTrial);
    printlog(LOG_INFO, "LGOperRegSize      : %u", LGOperRegSize);
    printlog(LOG_INFO, "GMEnable           : %s", GMEnable ? "true" : "false");
    printlog(LOG_INFO, "GMThresholdBase    : %u", GMThresholdBase);
    printlog(LOG_INFO, "GMThresholdStep    : %u", GMThresholdStep);
    printlog(LOG_INFO, "GMTrial            : %d", GMTrial);
    printlog(LOG_INFO, "LMThresholdBase    : %d", LMThresholdBase);
    printlog(LOG_INFO, "LMThresholdStep    : %d", LMThresholdStep);
    printlog(LOG_INFO, "LMTrial            : %d", LMTrial);
    printlog(LOG_INFO, "enableLocalVert    : %s", EnableLocalVert ? "true" : "false");
    printlog(LOG_INFO, "enableLocalReorder : %s", EnableLocalReorder ? "true" : "false");
    printlog(LOG_INFO, "enableLocalCompact : %s", EnableLocalCompact ? "true" : "false");
    printlog(LOG_INFO, "enablePinAcc       : %s", EnablePinAcc ? "true" : "false");
    printlog(LOG_INFO, "enablePinAccRpt    : %s", EnablePinAccRpt ? "true" : "false");

    printlog(LOG_INFO, "MLLAccuracy        : %d", MLLAccuracy);
    printlog(LOG_INFO, "MLLDispFromInput   : %s", MLLDispFromInput ? "true" : "false");
    printlog(LOG_INFO, "MLLPGAlign         : %s", MLLPGAlign ? "true" : "false");
    printlog(LOG_INFO, "MLLPinCost         : %u", MLLPinCost);
    printlog(LOG_INFO, "MLLMaxDensity      : %.3lf", MLLMaxDensity / 1000.0);
    printlog(LOG_INFO, "MLLUseILP          : %s", MLLUseILP ? "true" : "false");
    printlog(LOG_INFO, "MLLTotalDisp       : %s", MLLTotalDisp ? "true" : "false");
}



bool DPModule::dplace(const string& flowName) {


    dplacer = new DPlacer();
    dplacer->input();
    dplacer->place(flowName);
    dplacer->output();
    delete dplacer;
    return true;
}
