#ifndef _GP_QSOLVE_H_
#define _GP_QSOLVE_H_

#include "gp_cg.h"

void lowerBound(
        double epsilon,
        double pseudoAlpha,
        int repeat,
        LBMode mode,
        NetModel model,
        double relax,
        double maxDisp,
        vector<double> &cellX,
        vector<double> &cellY,
        int pins,
        int cells,
        int nets,
        vector<vector<int>>net_Cell);
void initLowerBound();

#endif

