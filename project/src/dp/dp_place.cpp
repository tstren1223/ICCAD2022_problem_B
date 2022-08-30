#include "dp_data.h"
#include "dp_global.h"
#include "../io/io.h"
using namespace dp;
using namespace moodycamel;
vector<Cell *>::iterator cellsIter;
list<pair<Cell *, Region>> originalRegions;
unordered_map<Cell *, Region> operatingRegions;
// 2022 modified
void check_share(DPlacer &D)
{
    for (int i = 0; i < io::IOModule::cells; i++)
    {
        cout << i << " " << io::IOModule::sh.shared_memx[i] << " " << io::IOModule::sh.shared_memy[i] << endl;
    }
    if (io::IOModule::pid == 0)
    {
        cout << "top:" << endl;
    }
    else
    {
        cout << "bot:" << endl;
    }
    for (unsigned int i = 0; i < D.cells.size(); i++)
    {
        cout << i << " " << D.cells[i]->lx() * D.siteW << " " << D.cells[i]->ly() * D.siteH << endl;
    }
}
void DPlacer::update_share(bool wait)
{
    if (io::IOModule::pid == 0)
    {
        if (wait)
            sem_post(io::IOModule::sh.data_ready);
        if ((int)cells.size() != io::IOModule::top_c)
            cerr << "Error::top size problem in DP!" << endl;
        for (int i = 0; i < io::IOModule::top_c; i++)
        {
            db::Cell *d = database.cells[i];
            io::IOModule::sh.shared_memx[io::IOModule::die_to_g[i]] = dbCellMap[d]->lx() * siteW + dbLX;
            io::IOModule::sh.shared_memy[io::IOModule::die_to_g[i]] = dbCellMap[d]->ly() * siteH + dbLY;
            // cout<<"(DT)Put"<<i<<"to "<<io::IOModule::die_to_g[i]<<endl;
        }
        if (wait)
            sem_wait(io::IOModule::sh.end_of_LB);
    }
    else
    {
        if (wait)
            sem_post(io::IOModule::sh.end_of_LB);
        if ((int)cells.size() != io::IOModule::bott_c)
            cerr << "Error::bot size problem in DP!" << endl;
        for (int i = 0; i < io::IOModule::bott_c; i++)
        {
            db::Cell *d = database.cells[i];
            io::IOModule::sh.shared_memx[io::IOModule::die_to_g[i]] = dbCellMap[d]->lx() * siteW + dbLX;
            io::IOModule::sh.shared_memy[io::IOModule::die_to_g[i]] = dbCellMap[d]->ly() * siteH + dbLY;
            // cout<<"(DB)Put"<<i<<"to "<<io::IOModule::die_to_g[i]<<endl;
        }
        if (wait)
            sem_wait(io::IOModule::sh.data_ready);
    }
}

void net_GR(Net *net, Region &r)
{
    int g_n = io::IOModule::net_to_gnet[net->i];
    int num = io::IOModule::netPinX[g_n].size();
    for (int j = 0; j < num; j++)
    {
        if (io::IOModule::gnet_to_localset[g_n].count(j))
        {
            continue;
        }
        int px, py;
        int cell = io::IOModule::gnet_to_gcell[make_pair(g_n, j)];
        px = io::IOModule::netPinX[g_n][j] + io::IOModule::sh.shared_memx[cell];
        py = io::IOModule::netPinY[g_n][j] + io::IOModule::sh.shared_memy[cell];
        r.cover(px, py);
    }
}
void Cell::cell_optimal_2dies(Region &region)
{

    int nPins = pins.size();
    if (nPins == 0)
    {
        region.lx = DPModule::dplacer->coreLX;
        region.ly = DPModule::dplacer->coreLY;
        region.hx = DPModule::dplacer->coreHX - w;
        region.hy = DPModule::dplacer->coreHY - h;
        return;
    }
    vector<int> boundx(pins.size() * 2);
    vector<int> boundy(pins.size() * 2);
    vector<Pin *>::const_iterator cpi = pins.cbegin();
    vector<Pin *>::const_iterator cpe = pins.cend();
    for (int i = 0; cpi != cpe; ++cpi, i++)
    {
        const Pin *cellpin = *cpi;
        set<pair<int, int>> f;
        Region netbound = cellpin->net->box;
        int cpx = cellpin->pinX();
        int cpy = cellpin->pinY();
        net_GR(cellpin->net, netbound);
        int lx = DPModule::dplacer->coreHX * DPModule::dplacer->siteW;
        int ly = DPModule::dplacer->coreHY * DPModule::dplacer->siteH;
        int hx = DPModule::dplacer->coreLX * DPModule::dplacer->siteW;
        int hy = DPModule::dplacer->coreLY * DPModule::dplacer->siteH;
        if (nPins > 2 && netbound.lx != cpx && netbound.hx != cpx && netbound.ly != cpy && netbound.hy != cpy)
        {
            lx = netbound.lx;
            ly = netbound.ly;
            hx = netbound.hx;
            hy = netbound.hy;
        }
        else
        {
            vector<Pin *>::const_iterator npi = cellpin->net->pins.cbegin();
            vector<Pin *>::const_iterator npe = cellpin->net->pins.cend();
            for (; npi != npe; ++npi)
            {
                Pin *netpin = *npi;
                if (netpin->cell == this)
                {
                    continue;
                }
                int px = netpin->pinX();
                int py = netpin->pinY();
                lx = min(px, lx);
                ly = min(py, ly);
                hx = max(px, hx);
                hy = max(py, hy);
            }
            int g_n = io::IOModule::net_to_gnet[cellpin->net->i];
            int num = io::IOModule::netPinX[g_n].size();
            for (int j = 0; j < num; j++)
            {
                if (io::IOModule::gnet_to_localset[g_n].count(j))
                {
                    continue;
                }
                int px, py;
                int cell = io::IOModule::gnet_to_gcell[make_pair(g_n, j)];
                px = io::IOModule::netPinX[g_n][j] + io::IOModule::sh.shared_memx[cell];
                py = io::IOModule::netPinY[g_n][j] + io::IOModule::sh.shared_memy[cell];
                lx = min(px, lx);
                ly = min(py, ly);
                hx = max(px, hx);
                hy = max(py, hy);
            }
        }

        boundx[i * 2] = lx - cellpin->x;
        boundy[i * 2] = ly - cellpin->y;
        boundx[i * 2 + 1] = hx - cellpin->x;
        boundy[i * 2 + 1] = hy - cellpin->y;
    }

    nth_element(boundx.begin(), boundx.begin() + (nPins - 1), boundx.end());
    nth_element(boundy.begin(), boundy.begin() + (nPins - 1), boundy.end());
    region.lx = (int)floor((double)boundx[nPins - 1] / DPModule::dplacer->siteW);
    region.ly = (int)floor((double)boundy[nPins - 1] / DPModule::dplacer->siteH);
    /*
    int rx = *min_element(boundx.begin() + nPins, boundx.end());
    int ry = *min_element(boundy.begin() + nPins, boundy.end());
    region.hx = (int)ceil((double)rx / DPModule::dplacer.siteW);
    region.hy = (int)ceil((double)ry / DPModule::dplacer.siteH);
    */
    nth_element(boundx.begin(), boundx.begin() + nPins, boundx.end());
    nth_element(boundy.begin(), boundy.begin() + nPins, boundy.end());
    region.hx = (int)ceil((double)boundx[nPins] / DPModule::dplacer->siteW);
    region.hy = (int)ceil((double)boundy[nPins] / DPModule::dplacer->siteH);

    region.lx = max(DPModule::dplacer->coreLX, min(DPModule::dplacer->coreHX - (int)w, region.lx));
    region.ly = max(DPModule::dplacer->coreLY, min(DPModule::dplacer->coreHY - (int)h, region.ly));
    region.hx = max(DPModule::dplacer->coreLX, min(DPModule::dplacer->coreHX - (int)w, region.hx));
    region.hy = max(DPModule::dplacer->coreLY, min(DPModule::dplacer->coreHY - (int)h, region.hy));
}
void Cell::cell_ImproveRegion_2die(Region& region, Region& optimalRegion) {
    if (optimalRegion.empty()) {
        cell_optimal_2dies(optimalRegion);
    }
    region = optimalRegion;
    region.cover(_x.load(memory_order_acquire), _y.load(memory_order_acquire));
}
long long getHPWLDiff_2die(Move &move)
{
    long long wl_diff = 0;
    for (const Pin *cellpin : move.target.cell->pins)
    {
        int n = cellpin->net->i;
        Net *net = cellpin->net;
        set<pair<int, int>> f;
        Region region = net->box;
        net_GR(net, region);
        const int ox = cellpin->pinX();
        const int oy = cellpin->pinY();
        const int nx = move.target.xLong() * database.siteW + cellpin->x;
        const int ny = move.target.yLong() * database.siteH + cellpin->y;
        bool need_update = false;
        if (io::IOModule::netPinX[n].size() <= 3 || region.lx == ox || region.hx == ox || region.ly == oy || region.hy == oy)
        {
            need_update = true;
        }

        long long o = region.height() + region.width();
        if (need_update)
        {
            region.clear();
            vector<Pin *>::iterator npi = net->pins.begin();
            vector<Pin *>::iterator npe = net->pins.end();
            for (; npi != npe; ++npi)
            {
                Pin *pin = *npi;
                double px, py;
                if (pin == cellpin)
                {
                    px = nx;
                    py = ny;
                }
                else
                {
                    px = pin->x;
                    py = pin->y;
                    if (pin->cell)
                    {
                        px += pin->cell->lx() * database.siteW;
                        py += pin->cell->ly() * database.siteH;
                    }
                }
                region.cover(px, py);
            }
            int g_n = io::IOModule::net_to_gnet[n];
            int num = io::IOModule::netPinX[g_n].size();
            for (int j = 0; j < num; j++)
            {
                if (io::IOModule::gnet_to_localset[g_n].count(j))
                {
                    continue;
                }
                int px, py;
                int cell = io::IOModule::gnet_to_gcell[make_pair(g_n, j)];
                px = io::IOModule::netPinX[g_n][j] + io::IOModule::sh.shared_memx[cell];
                py = io::IOModule::netPinY[g_n][j] + io::IOModule::sh.shared_memy[cell];
                region.cover(px, py);
            }
        }
        else
            region.cover(nx, ny);

        wl_diff = wl_diff + region.width() + region.height() - o;
    }
    return wl_diff;
}
//----------------------------------------------------------------------------
void DPlacer::setupFlow(const std::string &name)
{
    if (name == "iccad2017")
    {
        // flow for displacement-driven DP
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("CM", DPStage::Technique::ChainMove, 30);
        flow.addStage("LM", DPStage::Technique::LocalMoveForPinAccess, 1);
        //  flow.addStage("MP", DPStage::Technique::GlobalMoveForPinAccess, DPModule::MaxMPIter);
        flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
        if (DPModule::GMEnable)
        {
            flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
        }
    }
    else if (name == "dac2016")
    {
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
    }
    else if (name == "eval")
    {
        flow.addStage("EV", DPStage::Technique::Eval, 1);
    }
    else if (name == "WLDriven")
    {
        // flow for wirelength-driven DP
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
        flow.addStage("LM", DPStage::Technique::LocalMove, DPModule::MaxLMIter);
    }
    else if (name == "DispDriven")
    {
        // flow for displacement-driven DP
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        // flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
        // flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
    }
    else
    {
        // flow for DP API
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
        flow.addStage("LM", DPStage::Technique::LocalMove, DPModule::MaxLMIter);
    }
}

void DPlacer::place(const string &name)
{
    setupFlow(name);

#ifdef LIBGD
    drawDisplacement("dp_displacement_nn.png");
#endif
    //bool GP_f = true;
    for (bool running = true; running;)
    {
        DPStage::Technique tech = flow.technique();
        int nMoved = 0;
        switch (tech)
        {
        case DPStage::Technique::None:
            // first stage
            break;
        case DPStage::Technique::Eval:
            nMoved = eval();
            break;
        case DPStage::Technique::Premove:
            nMoved = premove();
            break;
        case DPStage::Technique::Legalize:
            // forICCAD 2022 change:
            nMoved = legalize();
            break;
        case DPStage::Technique::ChainMove:
            //  nMoved = chainMove();
            nMoved = localChainMove();
            break;
        case DPStage::Technique::GlobalMove:
            /*
            if(!io::IOModule::debug){
            if (GP_f)
            {
                update_share();
                GP_f = false;
            }
            sem_wait(io::IOModule::sh.DP);
            }
            // check_share(*this);
            */
            nMoved = globalMove();
            /*
            if(!io::IOModule::debug){
            update_share(false);
            // check_share(*this);
            sem_post(io::IOModule::sh.DP);
            }*/
            break;
        case DPStage::Technique::GlobalMoveForPinAccess:
            nMoved = globalMoveForPinAccess();
            break;
        case DPStage::Technique::LocalMove:
            /*
            sem_wait(io::IOModule::sh.DP);
            // check_share(*this);
            */
            nMoved = localMove();
            /*
            update_share(false);
            // check_share(*this);
            sem_post(io::IOModule::sh.DP);
            */
            break;
        case DPStage::Technique::LocalMoveForPinAccess:
            nMoved = localMoveForPinAccess();
            break;
        case DPStage::Technique::GlobalNF:
            nMoved = minCostFlow();
            break;
        default:
            break;
        }
        flow.update();
        if (nMoved || !flow.iter() || tech == DPStage::Technique::GlobalMoveForPinAccess)
        {
            flow.report();
        }
        // stopping condition for each stage
        switch (tech)
        {
        case DPStage::Technique::Eval:
            running = flow.nextStage();
            continue;
        case DPStage::Technique::Legalize:
            if (!flow.unplaced())
            {
#ifdef LIBGD
                drawDisplacement("dp_displacement_lg.png", flow.stage(2).dbMaxCell(0));
#endif

                running = flow.nextStage();
                continue;
            }
            break;
        case DPStage::Technique::ChainMove:
            if (!nMoved)
            {
#ifdef LIBGD
                drawDisplacement("dp_displacement_cm.png", flow.stage(2).dbMaxCell(0));
#endif

                running = flow.nextStage();
                continue;
            }
            break;
        case DPStage::Technique::GlobalMove:
            if (!flow.unplaced() && flow.hpwlDiff() > -0.0005)
            {
                running = flow.nextStage();
                continue;
            }
            break;
        case DPStage::Technique::GlobalMoveForPinAccess:
            if (!flow.unplaced() && !flow.overlapSum())
            {
                running = flow.nextStage();
                continue;
            }
            break;
        case DPStage::Technique::LocalMove:
            if (!flow.unplaced() && flow.hpwlDiff() > 0.0)
            {
                running = flow.nextStage();
                continue;
            }
            break;
        case DPStage::Technique::GlobalNF:
            running = flow.nextStage();
            continue;
        default:
            break;
        }
        running = flow.next();
    }

#ifdef LIBGD
    drawLayout("dp_layout.png");
    drawDisplacement("dp_displacement.png");
#endif
}

bool DPlacer::isBookshelfSymbol(unsigned char c)
{
    static char symbols[256] = {0};
    static bool inited = false;
    if (!inited)
    {
        symbols[(int)'('] = 1;
        symbols[(int)')'] = 1;
        // symbols[(int)'['] = 1;
        // symbols[(int)']'] = 1;
        symbols[(int)','] = 1;
        // symbols[(int)'.'] = 1;
        symbols[(int)':'] = 1;
        symbols[(int)';'] = 1;
        // symbols[(int)'/'] = 1;
        symbols[(int)'#'] = 1;
        symbols[(int)'{'] = 1;
        symbols[(int)'}'] = 1;
        symbols[(int)'*'] = 1;
        symbols[(int)'\"'] = 1;
        symbols[(int)'\\'] = 1;

        symbols[(int)' '] = 2;
        symbols[(int)'\t'] = 2;
        symbols[(int)'\n'] = 2;
        symbols[(int)'\r'] = 2;
        inited = true;
    }
    return symbols[(int)c] != 0;
}

bool DPlacer::readBSLine(istream &is, vector<string> &tokens)
{
    tokens.clear();
    string line;
    while (is && tokens.empty())
    {
        // read next line in
        getline(is, line);

        char token[1024] = {0};
        int tokenLen = 0;
        for (const char c : line)
        {
            if (c == '#')
            {
                break;
            }

            if (isBookshelfSymbol(c))
            {
                if (tokenLen > 0)
                {
                    token[tokenLen] = (char)0;
                    tokens.push_back(string(token));
                    token[0] = (char)0;
                    tokenLen = 0;
                }
            }
            else
            {
                token[tokenLen++] = c;
                if (tokenLen > 1024)
                {
                    // TODO: unhandled error
                    tokens.clear();
                    return false;
                }
            }
        }
        // line finished, something else in token
        if (tokenLen > 0)
        {
            token[tokenLen] = (char)0;
            tokens.push_back(string(token));
            tokenLen = 0;
        }
    }
    return !tokens.empty();
}

bool DPlacer::readDefCell(string file, map<string, tuple<int, int, int>> &cell_list)
{
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }

    vector<string> tokens;
    while (DPlacer::readBSLine(fs, tokens))
    {
        if (tokens[0] == "COMPONENTS")
        {
            break;
        }
    }
    cell_list.clear();
    while (DPlacer::readBSLine(fs, tokens))
    {
        if (tokens[0] == "END")
        {
            break;
        }
        else if (tokens[0] == "SOURCE")
        {
            DPlacer::readBSLine(fs, tokens);
        }
        string name = tokens[1];
        readBSLine(fs, tokens);
        istringstream issx(tokens[2]);
        istringstream issy(tokens[3]);
        double x = 0;
        double y = 0;
        issx >> x;
        issy >> y;
        int orient = -1;
        if (tokens[4] == "N")
        {
            orient = 0;
        }
        else if (tokens[4] == "S")
        {
            orient = 2;
        }
        else if (tokens[4] == "FN")
        {
            orient = 4;
        }
        else if (tokens[4] == "FS")
        {
            orient = 6;
        }
        cell_list[name] = make_tuple((int)std::round(x), (int)std::round(y), orient);
    }

    return true;
}

int DPlacer::eval()
{
    int nMoved = 0;
    map<string, tuple<int, int, int>> cell_list;
    readDefCell(DPModule::DefEval, cell_list);
    for (Cell *cell : cells)
    {
        tuple<int, int, int> &t = cell_list[cell->getDBCell()->name()];
        Move move(cell, get<0>(t) / siteW, get<1>(t) / siteH, get<2>(t));
        doLegalMove(move, false);
        ++nMoved;
    }
    return nMoved;
}

int DPlacer::premove()
{
    int nMoved = 0;
    int preMoveDispX = 0;
    int preMoveDispY = 0;
    for (Cell *cell : cells)
    {
        const Segment *segment = getNearestSegment(cell, cell->cx(), cell->ly());
        if (!segment)
        {
            printlog(
                LOG_ERROR, "fail to find segment for %s with %u width", cell->getDBCellType()->name.c_str(), cell->w);
        }
        int dispX = 0;
        int dispY = abs(cell->ly() - segment->y);
        if (dispY)
        {
            cell->pmDy(dispY);
        }
        else
        {
            db::Region *reg = database.regions[cell->fid];
            cell->pmDy(-min(cell->ly() - ceil((reg->ly - database.coreLY) / (double)siteH),
                            floor((reg->hy - database.coreLY) / (double)siteH) - cell->hy()));
        }
        cell->ly(segment->y);
        if (cell->lx() < segment->boundL())
        {
            dispX = segment->boundL() - cell->lx();
            cell->lx(segment->boundL());
        }
        else if (cell->hx() > segment->boundR())
        {
            dispX = cell->hx() - segment->boundR();
            cell->hx(segment->boundR());
        }
        if (dispX > 0 || dispY > 0)
        {
            preMoveDispX += dispX;
            preMoveDispY += dispY;
            nMoved++;
        }
    }

#ifndef NDEBUG
    if (preMoveDispX > 0 || preMoveDispY > 0)
    {
        printlog(LOG_INFO, "pre move disp x = %d", preMoveDispX);
        printlog(LOG_INFO, "pre move disp y = %d", preMoveDispY);
    }
#endif

    return nMoved;
}

int DPlacer::legalize()
{
    cellsIter = cells.begin();
    const int threshold = -1;
    localRegions.resize(DPModule::CPU, LocalRegion(cells.size()));
    tQ = new ConcurrentQueue<pair<Cell *, Region>>(DPModule::CPU, 1, 0);
    rQ = new ConcurrentQueue<pair<Cell *, bool>>(DPModule::CPU, DPModule::CPU - 1, 0);
    tPtok = new ProducerToken(*tQ);
    for (unsigned i = 0; i != DPModule::CPU; ++i)
    {
        rPtoks.emplace_back(*rQ);
    }
    vector<future<unsigned>> futs;
    for (unsigned i = 1; i < DPModule::CPU; ++i)
    {
        futs.push_back(async(&DPlacer::legalizeCell, this, threshold, i));
    }
    int nMoved = legalizeCell(threshold, 0);
    for (future<unsigned> &fut : futs)
    {
        nMoved += fut.get();
    }
    delete tQ;
    delete rQ;
    delete tPtok;
    tQ = nullptr;
    rQ = nullptr;
    tPtok = nullptr;

    return nMoved;
}

unsigned DPlacer::legalizeCell(const int threshold, const unsigned iThread)
{
    unsigned nMoved = 0;
    pair<Cell *, Region> pcr;
    pair<Cell *, bool> pcb;
    //  static unsigned batchIdx = 0;
    while (true)
    {
        if (!iThread)
        {
            oHist = hist;
            oMaxDisp = maxDisp;

            list<pair<Cell *, Region>>::iterator oriRegIter = originalRegions.begin();
            while (oriRegIter != originalRegions.end())
            {
                if (oriRegIter->second.independent(operatingRegions))
                {
                    Cell *cell = oriRegIter->first;
                    operatingRegions[cell] = oriRegIter->second;
                    tQ->enqueue(*tPtok, *oriRegIter);
                    oriRegIter = originalRegions.erase(oriRegIter);
                    if (operatingRegions.size() == DPModule::LGOperRegSize)
                    {
                        break;
                    }
                }
                else
                {
                    ++oriRegIter;
                }
            }
            for (; operatingRegions.size() != DPModule::LGOperRegSize && cellsIter != cells.end(); ++cellsIter)
            {
                Cell *cell = *cellsIter;
                Region oRegion = cell->getOriginalRegion(MLLLocalRegionW, MLLLocalRegionH);

                assert(oRegion.area());
#ifdef LEGALIZE_REGION_SHIFT_TOWARDS_OPTIMAL_REGION
                Region optimalRegion = cell->getOptimalRegion();
                optimalRegion.hx += cell->w;
                optimalRegion.hy += cell->h;

                int targetX = optimalRegion.cx();
                int targetY = optimalRegion.cy();
                targetX = max(oRegion.lx, min(targetX, oRegion.hx));
                targetY = max(oRegion.ly, min(targetY, oRegion.hy));
                oRegion.shift(targetX - oRegion.cx(), targetY - oRegion.cy());
#endif
                if (oRegion.independent(operatingRegions))

                {
                    operatingRegions.emplace(cell, oRegion);
                    tQ->enqueue(*tPtok, make_pair(cell, oRegion));
                    if (operatingRegions.size() == DPModule::LGOperRegSize)
                    {
                        ++cellsIter;
                        break;
                    }
                }
                else
                {
                    originalRegions.emplace_back(cell, oRegion);
                }
            }

            if (operatingRegions.empty() && originalRegions.empty())
            {
                for (unsigned termThrIdx = 1; termThrIdx < DPModule::CPU; ++termThrIdx)
                {
                    tQ->enqueue(*tPtok, make_pair(nullptr, Region()));
                }
                return nMoved;
            }
        }
        while (tQ->try_dequeue_from_producer(*tPtok, pcr))
        {
            Cell *cell = pcr.first;
            if (!cell)
            {
                return nMoved;
            }
            /*
            if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                printlog(LOG_INFO, "cell %s start %u", cell->getDBCell()->name().c_str(), batchIdx);
            }
            */
            cell->tx(cell->ox());
            cell->ty(cell->oy());
            Move move(cell, cell->tx(), cell->ty());

            const Region &oRegion = pcr.second;
            assert(oRegion.lx != INT_MAX && oRegion.ly != INT_MAX && oRegion.hx != INT_MIN && oRegion.hy != INT_MIN);

            if (isLegalMove(move, oRegion, threshold, 1, iThread))
            {
                doLegalMove(move, false);
                ++nMoved;
                rQ->enqueue(make_pair(cell, false));
                /*
                if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                    printlog(LOG_INFO, "cell %s succeeded", cell->getDBCell()->name().c_str());
                }
                */
                continue;
            }
            rQ->enqueue(make_pair(cell, true));
            /*
            if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                printlog(LOG_INFO, "cell %s failed", cell->getDBCell()->name().c_str());
            }
            */
        }

        if (iThread)
        {
            continue;
        }
        //  ++batchIdx;
        while (operatingRegions.size())
        {
            if (!rQ->try_dequeue(pcb))
            {
                continue;
            }
            Cell *cell = pcb.first;
            if (pcb.second)
            {
                Region &region = operatingRegions[cell];
                region.expand(
                    MLLLocalRegionW, MLLLocalRegionW, MLLLocalRegionH, MLLLocalRegionH, database.regions[cell->fid]);
                if (region.isMax())
                {
                    printlog(LOG_WARN,
                             "local region %s covers whole fence %s",
                             cell->getDBCell()->name().c_str(),
                             database.regions[cell->fid]->name().c_str());
                }
                bool is_not_emplaced = true;
                for (list<pair<Cell *, Region>>::iterator iter = originalRegions.begin(); iter != originalRegions.end();
                     ++iter)
                {
                    if (iter->first->i > cell->i)
                    {
                        originalRegions.emplace(iter, cell, region);
                        is_not_emplaced = false;
                        break;
                    }
                }
                if (is_not_emplaced)
                {
                    originalRegions.emplace_back(cell, region);
                }
            }
            operatingRegions.erase(cell);
        }
    }
}

unsigned DPlacer::chainMove()
{
    unsigned nMoves = 0;
    for (unsigned i = 0; i != nRegions; ++i)
    {
        for (unsigned j = 0; j != nDPTypes; ++j)
        {
            nMoves += chainMoveCell(i, j);
        }
    }
    return nMoves;
}

unsigned DPlacer::localChainMove()
{
    Cell *targetCell = cells[0];
    for (Cell *cell : cells)
    {
        if (abs(cell->lx() - cell->ox()) * siteW + abs(cell->ly() - cell->oy()) * siteH >
            abs(targetCell->lx() - targetCell->ox()) * siteW + abs(targetCell->ly() - targetCell->oy()) * siteH)
        {
            targetCell = cell;
        }
    }
    return localChainMoveCell(targetCell);
}

unsigned DPlacer::globalMove()
{
    unsigned nMoves = 0;
    const unsigned threshold = DPModule::GMThresholdBase + flow.iter() * DPModule::GMThresholdStep;
    for (Cell *cell : cells)
    {
        nMoves += globalMoveCell(cell, threshold, 20, 3);
    }
    return nMoves;
}

unsigned DPlacer::globalMoveCell(Cell *cell, int threshold, int Rx, int Ry)
{
    oHist = hist;
    for (int trial = 0; trial < 2; trial++)
    {
        Region trialRegion;
        Region oRegion;
        //if(!io::IOModule::debug)
        //cell->cell_optimal_2dies(oRegion);
        //else
        oRegion=cell->getOptimalRegion();

        if (trial == 0)
        {
#ifdef OPTIMAL_REGION_COVER_FIX
            int dx = max(0, max(oRegion.lx - cell->lx(), cell->lx() - oRegion.hx));
            int dy = max(0, max(oRegion.ly - cell->ly(), cell->ly() - oRegion.hy));
            int distance = dx * siteW + dy * siteH;
            if (cell->placed && distance <= DISTANCE_THRESHOLD)
            {
                return 0;
            }
#else
            if (cell->placed && oRegion.contains(cell->lx(), cell->ly(), cell->w, cell->h))
            {
                // if(cell->placed && oRegion.contains(cell->lx(), cell->ly())){
                return 0;
            }
#endif
#ifdef OPTIMAL_REGION_SIZE_LIMIT
            if (oRegion.width() > OPTIMAL_REGION_SIZE_X_LIMIT)
            {
                if (cell->lx() < oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT / 2)
                {
                    oRegion.hx = oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT;
                }
                else if (cell->lx() > oRegion.hx - OPTIMAL_REGION_SIZE_X_LIMIT / 2)
                {
                    oRegion.lx = oRegion.hx - OPTIMAL_REGION_SIZE_X_LIMIT;
                }
                else
                {
                    oRegion.lx = cell->lx() - OPTIMAL_REGION_SIZE_X_LIMIT / 2;
                    oRegion.hx = oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT;
                }
            }
            if (oRegion.height() > OPTIMAL_REGION_SIZE_Y_LIMIT)
            {
                if (cell->ly() < oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT / 2)
                {
                    oRegion.hy = oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT;
                }
                else if (cell->ly() > oRegion.hy - OPTIMAL_REGION_SIZE_Y_LIMIT / 2)
                {
                    oRegion.ly = oRegion.hy - OPTIMAL_REGION_SIZE_Y_LIMIT;
                }
                else
                {
                    oRegion.ly = cell->ly() - OPTIMAL_REGION_SIZE_Y_LIMIT / 2;
                    oRegion.hy = oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT;
                }
            }
#endif

            trialRegion = oRegion;
        }
        else
        {
            //if(!io::IOModule::debug)
            //cell->cell_ImproveRegion_2die(trialRegion,oRegion);
            //else
            trialRegion=cell->getImproveRegion(oRegion);
        }
        cell->tx(getrand(trialRegion.lx, trialRegion.hx));
        cell->ty(getrand(trialRegion.ly, trialRegion.hy));

        Move move(cell, cell->tx(), cell->ty());

        Region localRegion;
        localRegion.lx = max<long>(coreLX, cell->txLong() - Rx);
        localRegion.ly = max<long>(coreLY, cell->tyLong() - Ry);
        localRegion.hx = min<long>(coreHX - cell->w, cell->txLong() + Rx + cell->w);
        localRegion.hy = min<long>(coreHY - cell->h, cell->tyLong() + Ry + cell->h);
        if (isLegalMove(move, localRegion, threshold))
        {
            int hpwlDiff = getHPWLDiff(move);
            if (!cell->placed || hpwlDiff < 0)
            {
                doLegalMove(move, true);
                return 1;
            }
        }
    }
    return 0;
}

int DPlacer::globalMoveForPinAccess()
{
    int nMoves = 0;

    int stageIter = flow.iter() + 1;
    int base = DPModule::GMThresholdBase;
    int step = DPModule::GMThresholdStep;
    int threshold = base + (stageIter - 1) * step;

    int rangeX = 20 * stageIter; // 30;
    int rangeY = 3 * stageIter;  // 5;
    int nCells = cells.size();
    // printlog(LOG_INFO, "threshold = %d, rangeX = %d, rangeY = %d", threshold*siteW, rangeX, rangeY);
    for (int i = 0; i < nCells; i++)
    {
        if (cellTypeMaps[cells[i]->type].score(cells[i]->ly(), cells[i]->lx()) != 0.0)
            nMoves += globalMoveForPinAccessCell(cells[i], threshold, rangeX, rangeY);
    }
    return nMoves;
}

int DPlacer::globalMoveForPinAccessCell(Cell *cell, int threshold, int Rx, int Ry)
{
    cell->tx(cell->ox());
    cell->ty(cell->oy());

    Move move(cell, cell->tx(), cell->ty());

    Region localRegion;
    localRegion.lx = max<long>(coreLX, cell->txLong() - Rx);
    localRegion.ly = max<long>(coreLY, cell->tyLong() - Ry);
    localRegion.hx = min<long>(coreHX, cell->txLong() + Rx + cell->w);
    localRegion.hy = min<long>(coreHY, cell->tyLong() + Ry + cell->h);
    if (isLegalMove(move, localRegion, threshold, 0))
    {
        if (!cell->placed || getScoreDiff(move) < 0.0)
        {
            doLegalMove(move, true);
            return 1;
        }
    }

    return 0;
}

int DPlacer::localMove()
{
    int nMoves = 0;
    int stageIter = flow.iter() + 1;
    int base = DPModule::LMThresholdBase;
    int step = DPModule::LMThresholdStep;
    int threshold = base + stageIter * step;
    int rangeX = 10;
    int rangeY = 2;
    for (Cell *cell : cells)
    {
        nMoves += localVerticalMove(cell, threshold, rangeX, rangeY);
    }
    return nMoves;
}

int DPlacer::localVerticalMove(Cell *cell, int threshold, int Rx, int Ry)
{
    Region region = cell->getOriginalRegion(Rx, Ry);
    int offset[6] = {1, -1, 2, -2, 4, -4};
    for (int i = 0; i < 6; i++)
    {
        Move move(cell, cell->lx(), cell->ly() + offset[i]);
        // TODO: check if move improve wirelength
        long long kkk=getHPWLDiff(move);
        if (isLegalMove(move, region, threshold) &&  kkk< 0)
        {
            doLegalMove(move, true);
            // updateNet(move);
            return 1;
        }
    }
    return 0;
}

int DPlacer::localMoveForPinAccess()
{
    int nMoves = 0;
    for (Cell *cell : cells)
    {
        nMoves += localShiftForPinAccess(cell, 16);
    }
    for (auto &row : segments)
    {
        for (auto &seg : row)
        {
            nMoves += localSwapForPinAccess(seg, 8);
        }
    }
    return nMoves;
}

MLLCost DPlacer::getPinAccessAndDispCost(Cell *cell, const int x, const int y) const
{
    return cellTypeMaps[cell->type].score(y, x) * 1000 +                               // primary: pin access
           (abs(x - cell->ox()) + abs(y - cell->oy())) / (MLLCost)counts[cell->h - 1]; // secondary: disp
}

int DPlacer::localShiftForPinAccess(Cell *cell, int maxDisp)
{
    // get all candidate moves
    vector<CellMove> moves;
    int boundL = max(coreLX, cell->lx() - maxDisp);
    int boundR = min(coreHX - (int)cell->w, cell->lx() + maxDisp);
    for (int newX = boundL; newX <= boundR; ++newX)
    {
        if (newX != cell->lx())
        {
            moves.emplace_back(cell, newX, cell->ly());
        }
    }

    // find the best one
    MLLCost bestCost = getPinAccessAndDispCost(cell);
    CellMove *bestMove = nullptr;
    removeCell(cell);
    for (auto &move : moves)
    {
        if (!isLegalCellMove(move))
            continue;
        MLLCost newCost = getPinAccessAndDispCost(move);
        if (newCost < bestCost)
        {
            bestCost = newCost;
            bestMove = &move;
        }
    }

    // commit
    if (bestMove != nullptr)
    {
        Move move(*bestMove); // work around for CellMove -> Move
        doLegalMove(move, true);
        return 1;
    }
    else
    {
        insertCell(cell);
        return 0;
    }
}

int DPlacer::localSwapForPinAccess(Segment &seg, int maxDisp)
{
    int nMoves = 0;

    //  note that seg may be changed within for loop
    for (int i = 0; i < (int)seg.cells.size() - 1; ++i)
    {
        // fast pruning
        Cell *cellL = seg.cells.get(i);
        Cell *cellR = seg.cells.get(i + 1);
        if (cellR->lx() - cellL->lx() > maxDisp)
        {
            continue;
        }
        int oldSpace = cellR->lx() - cellL->hx(); // keep the old spacing for simplicity
        if (Cell::getSpace(cellR, cellL) > oldSpace)
        {
            continue;
        }

        // get all candidate moves
        vector<pair<CellMove, CellMove>> moves;
        int newRX = cellL->lx();                       // new x for cellR
        int newLX = cellL->lx() + cellR->w + oldSpace; // new x for cellL
        moves.emplace_back(CellMove(cellR, newRX, cellR->ly()),
                           CellMove(cellL, newLX, cellL->ly())); // keep themselves's
        if (cellR->ly() != cellL->ly())
        {
            moves.emplace_back(CellMove(cellR, newRX, cellL->ly()),
                               CellMove(cellL, newLX, cellR->ly())); // use the opposite's
            moves.emplace_back(CellMove(cellR, newRX, cellL->ly()),
                               CellMove(cellL, newLX, cellL->ly())); // both use cellL->ly()
            moves.emplace_back(CellMove(cellR, newRX, cellR->ly()),
                               CellMove(cellL, newLX, cellR->ly())); // both use cellR->ly()
        }

        // find the best one
        removeCell(cellL);
        removeCell(cellR);
        MLLCost bestCost = getPinAccessAndDispCost(cellL) + getPinAccessAndDispCost(cellR);
        const pair<CellMove, CellMove> *bestMove = nullptr;
        for (const auto &move : moves)
        {
            const auto &[moveR, moveL] = move;
            if (!isLegalCellMove(moveR) || !isLegalCellMove(moveL))
            {
                continue;
            }
            MLLCost newCost = getPinAccessAndDispCost(moveR) + getPinAccessAndDispCost(moveL);
            if (newCost < bestCost)
            {
                bestCost = newCost;
                bestMove = &move;
            }
        }

        // commit
        if (bestMove)
        {
            Move moveR(bestMove->first), moveL(bestMove->second); // work around for CellMove -> Move
            doLegalMove(moveR, true);
            doLegalMove(moveL, true);
            ++nMoves;
        }
        else
        {
            insertCell(cellL);
            insertCell(cellR);
        }
    }
    return nMoves;
}
