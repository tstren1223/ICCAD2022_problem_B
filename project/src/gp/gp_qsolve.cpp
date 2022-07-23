#include "gp_global.h"
#include "gp_data.h"

#include "gp_region.h"
#include "gp_qsolve.h"

using namespace gp;

#define FAST_INDEX_LOOKUP

double MinCellDist=0.01; //to avoid zero distance between two cells

int t_numNets;
int t_numCells;
vector<vector<int>> t_netCell;
int t_numPins;
long Cxsize;
long Cysize;
vector<SparseMatrix> Cx;
vector<SparseMatrix> Cy;
vector<double> bx;
vector<double> by;
vector<double> Mx_inverse;
vector<double> My_inverse;
vector<double> lox;
vector<double> hix;
vector<double> loy;
vector<double> hiy;
vector<double> lastVarX;
vector<double> lastVarY;
vector<double> varX;
vector<double> varY;
int numMovables = 0;
int numConnects = 0;

NetModel netModel = NetModelNone;

#ifdef FAST_INDEX_LOOKUP
vector<vector<long> > indexMapX;
vector<vector<long> > indexMapY;
#else
map<long, int> indexMapX;
map<long, int> indexMapY;
#endif

void initLowerBound(){
    netModel = NetModelNone;
}

void initCG(NetModel model,vector<double> &cellX,vector<double> &cellY){
   
    int nConnects = 2 * t_numPins + t_numCells;
    int nMovables = t_numCells;    
    
    numMovables = nMovables;
    numConnects = nConnects;
    netModel = model;
    varX.resize(numMovables, 0.0);
    varY.resize(numMovables, 0.0);
    lastVarX = cellX;
    lastVarY = cellY;
    Cx.resize(numConnects);
    Cy.resize(numConnects);
    bx.resize(numMovables);
    by.resize(numMovables);
    Mx_inverse.resize(numMovables);
    My_inverse.resize(numMovables);
    lox.resize(numMovables);
    hix.resize(numMovables);
    loy.resize(numMovables);
    hiy.resize(numMovables);
    Cxsize=0;
    Cysize=0;
#ifdef FAST_INDEX_LOOKUP
    indexMapX.resize(numMovables);
    indexMapY.resize(numMovables);
#else
    indexMapX.clear();
    indexMapY.clear();
#endif
}


void resetCG(){
    bx.assign(numMovables, 0);
    by.assign(numMovables, 0);
#ifdef FAST_INDEX_LOOKUP
    vector<vector<long> >::iterator itrx=indexMapX.begin();
    vector<vector<long> >::iterator itrxe=indexMapX.end();
    vector<vector<long> >::iterator itry=indexMapY.begin();
    vector<vector<long> >::iterator itrye=indexMapY.end();
    for(;itrx!=itrxe; ++itrx){
        itrx->clear();
    }
    for(;itry!=itrye; ++itry){
        itry->clear();
    }
#endif
    Cxsize=0;
    Cysize=0;
}
#ifdef FAST_INDEX_LOOKUP
int idxGetX(int i, int j){
    int size=indexMapX[i].size();
    for(int k=0; k<size; k++){
        if((int)(indexMapX[i][k]&0xffffffff)==j){
            return indexMapX[i][k]>>32;
        }
    }
    return -1;
}
int idxGetY(int i, int j){
    int size=indexMapY[i].size();
    for(int k=0; k<size; k++){
        if((int)(indexMapY[i][k]&0xffffffff)==j){
            return indexMapY[i][k]>>32;
        }
    }
    return -1;
}
void idxSetX(int i, int j, int x){
    long val=(((long)x)<<32)|(long)j;
    indexMapX[i].push_back(val);
}
void idxSetY(int i, int j, int x){
    long val=(((long)x)<<32)|(long)j;
    indexMapY[i].push_back(val);
}
#else
int idxGetX(int i, int j){
    long idx=(long)i*(long)numMovables+(long)j;
    map<long, int>::iterator itr=indexMapX.find(idx);
    if(itr==indexMapX.end()){
        return -1;
    }
    return itr->second;
}
int idxGetY(int i, int j){
    long idx=(long)i*(long)numMovables+(long)j;
    map<long, int>::iterator itr=indexMapY.find(idx);
    if(itr==indexMapY.end()){
        return -1;
    }
    return itr->second;
}
void idxSetX(int i, int j, int x){
    long idx=(long)i*(long)numMovables+(long)j;
    indexMapX[idx]=x;
}
void idxSetY(int i, int j, int x){
    long idx=(long)i*(long)numMovables+(long)j;
    indexMapY[idx]=x;
}
#endif

void addToC(
        vector<SparseMatrix> &C,
        int i, int j, double w,
        long *Csize,
        int (*idxGet)(int,int),
        void (*idxSet)(int,int,int)){
    //C[i][j]=C[i][j]+w
    int C_index=idxGet(i,j);
    if(C_index<0){
        //new value, add to C[Csize]
        C_index=*Csize;
        idxSet(i,j,C_index);
        
        C[C_index].row=i;
        C[C_index].col=j;
        C[C_index].num=w;

        *Csize=C_index+1;
    }else{
        //accumulate to old value
        C[C_index].num+=w;
    }
}

void B2BaddInC_b(
        int net,
        vector<SparseMatrix> &C,
        vector<double> &b,
        int idx1, int idx2,
        double x1, double x2,
        bool mov1, bool mov2,
        double w, long *Csize,
        int (*idxGet)(int,int),
        void (*idxSet)(int,int,int)){
    if(idx1 == idx2){
        return;
    }
    if(mov1 && mov2){
        if(idx1 < idx2){
            addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
            addToC(C, idx1, idx2, -w, Csize, idxGet, idxSet);
        }else{
            addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
            addToC(C, idx2, idx1, -w, Csize, idxGet, idxSet);
        }
    }else if(mov1){
        addToC(C, idx1, idx1, w, Csize, idxGet, idxSet);
        b[idx1] += w * x2;
    }else if(mov2){
        addToC(C, idx2, idx2, w, Csize, idxGet, idxSet);
        b[idx2] += w * x1;
    }
}


void B2BModel(){
    for(int net=0; net<t_numNets; net++){
        //find the net bounding box and the bounding cells
        int nPins = t_netCell[net].size();
        if(nPins < 2){
            continue;
        }
        double lx,hx,ly,hy;
        int lxp,hxp,lyp,hyp;
        int lxc,hxc,lyc,hyc;
        bool lxm,hxm,lym,hym;
        lxp = hxp = lyp = hyp = 0;
        lxc = hxc = lyc = hyc = t_netCell[net][0];
        if(lxc < t_numCells){
            lx = hx = varX[lxc];
            ly = hy = varY[lyc];
            lxm = hxm = lym = hym = true;
        }else{
            lx = hx = cellX[lxc] + netPinX[net][0];
            ly = hy = cellY[lyc] + netPinY[net][0];
            lxm = hxm = lym = hym = false;
        }
        for(int p=1; p<nPins; p++){
            int cell = t_netCell[net][p];
            double px, py;
            bool mov;
            if(cell < t_numCells){
                px = varX[cell];
                py = varY[cell];
                mov = true;
            }else{
                px = cellX[cell]+netPinX[net][p];
                py = cellY[cell]+netPinY[net][p];
                mov = false;
            }
            if(px <= lx && cell != hxc){
                lx = px;
                lxp = p;
                lxc = cell;
                lxm = mov;
            }else if(px >= hx && cell != lxc){
                hx = px;
                hxp = p;
                hxc = cell;
                hxm = mov;
            }
            if(py <= ly && cell != hyc){
                ly = py;
                lyp = p;
                lyc = cell;
                lym = mov;
            }else if(py >= hy && cell != lyc){
                hy = py;
                hyp = p;
                hyc = cell;
                hym = mov;
            }
        }

        if(lxc == hxc || lyc == hyc){
            continue;
            printlog(LOG_ERROR, "same bounding (%d) [%d,%d][%d,%d]", nPins, lxc, hxc, lyc, hyc);
            for(int i=0; i<nPins; i++){
                int cell = t_netCell[net][i];
                double px,py;
                bool movable;
                if(cell < t_numCells){
                    px = varX[cell];
                    py = varY[cell];
                    movable = true;
                }else{
                    px = cellX[cell]+netPinX[net][i];
                    py = cellY[cell]+netPinY[net][i];
                    movable = false;
                }
                if(movable){
                    printlog(LOG_ERROR, "+cell=%d:%d pin=(%lf,%lf)", cell, i, px, py);
                }else{
                    printlog(LOG_ERROR, "-cell=%d:%d pin=(%lf,%lf)", cell, i, px, py);
                }
            }
        }

        //enumerate B2B connections
        double w = 2.0  / (double) (nPins-1);
        double weightX = w / max(MinCellDist, hx-lx);
        double weightY = w / max(MinCellDist, hy-ly);
        B2BaddInC_b(net, Cx, bx, lxc, hxc, lx, hx, lxm, hxm, weightX, &Cxsize, idxGetX, idxSetX);
        B2BaddInC_b(net, Cy, by, lyc, hyc, ly, hy, lym, hym, weightY, &Cysize, idxGetY, idxSetY);
        
        for(int p=0; p<nPins; p++){
            int cell = t_netCell[net][p];
            double px, py;
            bool mov;
            if(cell < t_numCells){
                px = varX[cell];
                py = varY[cell];
                mov = true;
            }else{
                px = cellX[cell]+netPinX[net][p];
                py = cellY[cell]+netPinY[net][p];
                mov = false;
            }
            if(p != lxp && p != hxp){
                weightX = w / max(MinCellDist, px-lx);
                B2BaddInC_b(net, Cx, bx, cell, lxc, px, lx, mov, lxm, weightX, &Cxsize, idxGetX, idxSetX);
                weightX = w / max(MinCellDist, hx-px);
                B2BaddInC_b(net, Cx, bx, cell, hxc, px, hx, mov, hxm, weightX, &Cxsize, idxGetX, idxSetX);
            }
            if(p != lyp && p != hyp){
                weightY = w / max(MinCellDist, py-ly);
                B2BaddInC_b(net, Cy, by, cell, lyc, py, ly, mov, lym, weightY, &Cysize, idxGetY, idxSetY);
                weightY = w / max(MinCellDist, hy-py);
                B2BaddInC_b(net, Cy, by, cell, hyc, py, hy, mov, hym, weightY, &Cysize, idxGetY, idxSetY);
            }
        }
    }
}
void UpdateBoxConstraintRelaxedRect(double relax, double maxDisp){
    //relax: 0.0 = Sub-Fence 1.0 = Fence-BBox
    findCellRegionDist();

    vector<double> flx(numFences, coreHX);
    vector<double> fly(numFences, coreHY);
    vector<double> fhx(numFences, coreLX);
    vector<double> fhy(numFences, coreLY);
    for(int f=0; f<numFences; f++){
        int nRects = fenceRectLX[f].size();
        for(int r=0; r<nRects; r++){
            flx[f] = min(flx[f], fenceRectLX[f][r]);
            fly[f] = min(fly[f], fenceRectLY[f][r]);
            fhx[f] = max(fhx[f], fenceRectHX[f][r]);
            fhy[f] = max(fhy[f], fenceRectHY[f][r]);
        }
    }

    for(int i=0; i<t_numCells; i++){
        int f=cellFence[i];
        int r=cellFenceRect[i];
        double rlx = fenceRectLX[f][r];
        double rly = fenceRectLY[f][r];
        double rhx = fenceRectHX[f][r];
        double rhy = fenceRectHY[f][r];

        lox[i] = rlx + (flx[f] - rlx) * relax;
        hix[i] = rhx + (fhx[f] - rhx) * relax;
        loy[i] = rly + (fly[f] - rly) * relax;
        hiy[i] = rhy + (fhy[f] - rhy) * relax;

        if(maxDisp>=0){
            double dlx=cellX[i]-maxDisp;
            double dhx=cellX[i]+maxDisp;
            double dly=cellY[i]-maxDisp;
            double dhy=cellY[i]+maxDisp;
            lox[i]=max(dlx, min(dhx, lox[i]));
            hix[i]=max(dlx, min(dhx, hix[i]));
            loy[i]=max(dly, min(dhy, loy[i]));
            hiy[i]=max(dly, min(dhy, hiy[i]));
        }

        cellX[i]=max(lox[i], min(hix[i], cellX[i]));
        cellY[i]=max(loy[i], min(hiy[i], cellY[i]));
    }
}

void jacobiM_inverse(){
    for(int i=0; i<numMovables; i++){
        int idxx=idxGetX(i,i);
        if(idxx<0){
            Mx_inverse[i]=0;
        }else{
            Mx_inverse[i]=Cx[idxx].num>MinCellDist?1.0/Cx[idxx].num : 1.0/MinCellDist;
        }
        int idxy=idxGetY(i,i);
        if(idxy<0){
            My_inverse[i]=0;
        }else{
            My_inverse[i]=Cy[idxy].num>MinCellDist?1.0/Cy[idxy].num : 1.0/MinCellDist;
        }
    }
}

void pseudonetAddInC_b(double alpha){
    double weight;
    for(int i=0; i<t_numCells; i++){
        //weight = alpha / max(MinCellDist, cellDispX[i]);
        weight=alpha/max(MinCellDist, abs(lastVarX[i]-varX[i]));
        addToC(Cx, i, i, weight, &Cxsize, idxGetX, idxSetX);
        bx[i]+=(weight*varX[i]);

        //weight = alpha / max(MinCellDist, cellDispY[i]);
        weight=alpha/max(MinCellDist, abs(lastVarY[i]-varY[i]));
        addToC(Cy, i, i, weight, &Cysize, idxGetY, idxSetY);
        by[i]+=(weight*varY[i]);
    }
}

void lowerBound(
        double epsilon,
        double pseudoAlpha,
        int repeat,
        LBMode mode, NetModel model,
        double relax,
        double maxDisp,
        vector<double> &cellX,
        vector<double> &cellY,
        int pins,
        int cells,
        int nets,
        vector<vector<int>> net_Cell
        ){
    //setting
    t_numPins = pins;
    t_numCells = cells;
    t_numNets = nets;
    t_netCell = net_Cell;

    int CGi_max=500; //--The maximum iteration times of CG
    initCG(model,cellX,cellY);
    copy(cellX.begin(), cellX.begin()+t_numCells, varX.begin());
    copy(cellY.begin(), cellY.begin()+t_numCells, varY.begin());

    for(int r=0; r<repeat; r++){
        bool enableBox=false;
        resetCG();
        B2BModel();
        if(pseudoAlpha>0.0){
            pseudonetAddInC_b(pseudoAlpha);
        }
        jacobiM_inverse();
        jacobiCG(   Cx, bx, varX, lox, hix, Mx_inverse, CGi_max, epsilon, numMovables, Cxsize,
                    Cy, by, varY, loy, hiy, My_inverse, CGi_max, epsilon, numMovables, Cysize,
                    enableBox);
    }
    
    copy(varX.begin(), varX.begin()+t_numCells, cellX.begin());
    copy(varY.begin(), varY.begin()+t_numCells, cellY.begin());
    copy(varX.begin(), varX.begin()+t_numCells, lastVarX.begin());
    copy(varY.begin(), varY.begin()+t_numCells, lastVarY.begin());
}

