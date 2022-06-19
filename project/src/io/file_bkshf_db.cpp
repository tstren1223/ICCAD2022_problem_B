#include <set>

#include "../db/db.h"
#include "../global.h"
#include "io.h"
#include "bits/stdc++.h"
using namespace db;

// ICCAD2022
int _scale = 0;
class Tech
{
public:
    int nCells;
    int nTypes;
    unsigned nNets;
    map<string, int> cellMap;
    map<string, int> typeMap;
    vector<string> cellName;
    vector<int> cellX;
    vector<int> cellY;
    vector<char> cellFixed;
    vector<string> typeName;
    vector<int> cellType;
    vector<int> typeWidth;
    vector<int> typeHeight;
    vector<char> typeFixed;
    map<string, int> typePinMap;
    vector<int> typeNPins;
    vector<vector<string>> typePinName;
    vector<vector<char>> typePinDir;
    vector<vector<double>> typePinX;
    vector<vector<double>> typePinY;
    vector<string> netName;
    vector<vector<int>> netCells;
    vector<vector<int>> netPins;
    // AREA
    unordered_map<int, long long> type_areas;
    Tech()
    {
        nCells = 0;
        nTypes = 0;
        nNets = 0;
    }
    void setup_areas()
    {
        for (int i = 0; i < nTypes; i++)
            type_areas[i] = (long long)typeWidth[i] * (long long)typeHeight[i];
    }
    long long area(string type_name)
    { // if it is sring name
        if (typeMap.find(type_name) == typeMap.end())
        {
            cerr << "Instance setting up the area of instance:" << type_name << "with error!" << endl;
        }
        int i = typeMap[type_name];
        return type_areas[i];
    }
};
Tech tech_A, tech_B; // tech_B can be duplicate of techA

class Die_infro
{
    // LEF
public:
    Tech *tech;
    double MaxUtil;
    int lowleftX, lowleftY, toprightX, toprightY;
    int rowheight;

    // ICCAD2022
    double Terminal_w;
    double Terminal_h;
    double Terminal_spacing;
    int cell_num;
    // Ripple
    vector<int> rowX;
    vector<int> rowY;
    vector<int> rowSites;
    int nRows;
    Die_infro()
    {
        nRows = 0;
        cell_num = 0;
    }
    int area_constraint(long long &needed_area)
    {
        if (needed_area > ((height() * width()) * MaxUtil / 100))
            return 1; // invalid
        else
            return 0; // valid
    };
    long long height()
    {
        return toprightY - lowleftY;
    }
    long long width()
    {
        return toprightX - lowleftX;
    }
    long long dieheight()
    {
        return nRows * rowheight;
    }
};
Die_infro top, bottom;

struct part_Net
{
    unordered_set<int> part0;
    unordered_set<int> part1;
    int size[2] = {0, 0};
};
struct part_Cell
{
    int gain = 0;
    bool lock = false;
    bool part; // 0 for part1 1 for part2
};

class Instance
{
public:
    // instance
    int cell_inst_num;
    vector<string> cName;     // cell index as input
    vector<string> type_name; // cell index as input
    unordered_map<int, long long> cell_instances_areas;
    unordered_map<int, unordered_set<int>> cell_net_map;
    // net
    int net_num;
    vector<int> net_pin_num;
    vector<string> net_name;
    vector<vector<string>> net_cell_name;
    vector<vector<string>> net_pin_name;
    unordered_map<string, int> cell_inst_map; // from typename to index
    vector<Die_infro *> cell_inst_die;        // 0 for top,1 for bottom
    vector<int> cell_inst_size;
    class FM
    {
    private:
        long long total_area[2];
        vector<part_Cell> cells;
        unordered_map<int, part_Net> nets_infro;
        map<int, unordered_set<int>, greater<int>> gain_map;
        Instance *global_inst;

    public:
        unordered_map<int, int> crossNets_pair;
        int CrossNets_N;
        FM()
        {
            total_area[0] = 0;
            total_area[1] = 0;
        };
        void set_Instance(Instance *s)
        {
            global_inst = s;
        }
        // FM
        void partition()
        {
            net();
            if (system("chmod 755 shmetis") == -1)
                printlog(LOG_ERROR, "fail to execute chmod 755 shmetis");
            if (system("./shmetis net.hgr 2 5") == -1)
                printlog(LOG_ERROR, "fail to execute ./shmetis net.hgr 2 5");
            read_net();
            area_adjust();
            ans();
        }

        int area_constraint()
        {
            if (bottom.area_constraint(total_area[1]))
                return 2; // bottom invalid
            else if (top.area_constraint(total_area[0]))
                return 1; // top invalid
            else
                return 0; // both valid
        }

        void ans()
        {
            for (int i = 0; i < global_inst->net_num; i++)
            {
                if (nets_infro[i].size[0] > 1 && nets_infro[i].size[1] > 1)
                    crossNets_pair[i] = 0;
                else if (nets_infro[i].size[0] > 1 && nets_infro[i].size[1] == 1)
                    crossNets_pair[i] = 1;
                else if (nets_infro[i].size[0] == 1 && nets_infro[i].size[1] > 1)
                    crossNets_pair[i] = 2;
                else if (nets_infro[i].size[0] == 1 && nets_infro[i].size[1] == 1)
                    crossNets_pair[i] = 3;
                else
                    continue;
            }
            cout << "cross:" << CrossNets_N << " match?" << (CrossNets_N == (int)crossNets_pair.size()) << endl;
            CrossNets_N = crossNets_pair.size();
        }
        void testPartition()
        {
            ofstream out_file("testPartition.txt");
            out_file << "w0:" << total_area[0] << " w1:" << total_area[1] << endl;
            for (int i = 0; i < global_inst->cell_inst_num; i++)
                out_file << i + 1 << " " << (cells[i].part == false ? "top" : "bottom") << " " << endl;
        }

        void buckets_create()
        {
            // gain cal
            for (auto &it : nets_infro)
            {
                if (it.second.size[0] <= 1 || it.second.size[1] <= 1)
                {
                    if (it.second.size[0] == 0) // TE
                        for (auto i : it.second.part1)
                            cells[i].gain -= 1;
                    if (it.second.size[0] == 1) // FS
                        for (auto i : it.second.part0)
                            cells[i].gain += 1;
                    if (it.second.size[1] == 0) // TE
                        for (auto i : it.second.part0)
                            cells[i].gain -= 1;
                    if (it.second.size[1] == 1) // FS
                        for (auto i : it.second.part1)
                            cells[i].gain += 1;
                }
                else
                    continue;
            }
            // init buckets
            for (int i = 0; i < global_inst->cell_inst_num; i++)
                gain_map[cells[i].gain].insert(i);
        }
        void area_update(int node)
        {
            if (cells[node].part == false)
            {
                total_area[0] -= top.tech->area(global_inst->type_name[node]);
                total_area[1] += bottom.tech->area(global_inst->type_name[node]);
            }
            else
            {
                total_area[0] += top.tech->area(global_inst->type_name[node]);
                total_area[1] -= bottom.tech->area(global_inst->type_name[node]);
            }
        }
        void gain_update(int node)
        {
            unordered_map<int, int> update_gain;
            area_update(node);
            cells[node].lock = true;
            gain_map[cells[node].gain].erase(node);
            for (auto i : global_inst->cell_net_map[node]) // i: net contains node
            {
                if (cells[node].part == false) // from top to bottom
                {
                    nets_infro[i].part0.erase(node);
                    nets_infro[i].part1.insert(node);
                    nets_infro[i].size[0]--;
                    nets_infro[i].size[1]++;
                    if (nets_infro[i].size[0] == 1)
                        for (auto j : nets_infro[i].part0)
                            if (cells[j].lock == false)
                                update_gain[j] += 1;
                    if (nets_infro[i].size[0] == 0)
                        for (auto j : nets_infro[i].part1)
                            if (cells[j].lock == false)
                                update_gain[j] -= 1;
                    if (nets_infro[i].size[1] == 2)
                        for (auto &j : nets_infro[i].part1)
                            if (cells[j].lock == false)
                                update_gain[j] -= 1;
                    if (nets_infro[i].size[1] == 1)
                        for (auto &j : nets_infro[i].part0)
                            if (cells[j].lock == false)
                                update_gain[j] += 1;
                }
                else
                {
                    nets_infro[i].part1.erase(node);
                    nets_infro[i].part0.insert(node);
                    nets_infro[i].size[0]++;
                    nets_infro[i].size[1]--;
                    if (nets_infro[i].size[1] == 1)
                        for (auto j : nets_infro[i].part1)
                            if (cells[j].lock == false)
                                update_gain[j] += 1;
                    if (nets_infro[i].size[1] == 0)
                        for (auto j : nets_infro[i].part0)
                            if (cells[j].lock == false)
                                update_gain[j] -= 1;
                    if (nets_infro[i].size[0] == 2)
                        for (auto &j : nets_infro[i].part0)
                            if (cells[j].lock == false)
                                update_gain[j] -= 1;
                    if (nets_infro[i].size[0] == 1)
                        for (auto &j : nets_infro[i].part1)
                            if (cells[j].lock == false)
                                update_gain[j] += 1;
                }
            }
            cells[node].part = !cells[node].part;

            for (auto &it : update_gain)
            {
                int change_node = it.first;
                int change_gain = it.second;
                if (change_gain == 0)
                    continue;
                else
                {
                    gain_map[cells[change_node].gain].erase(change_node);
                    cells[change_node].gain += change_gain;
                    gain_map[cells[change_node].gain].insert(change_node);
                }
            }
        }

        void area_adjust()
        {
            buckets_create();
            int area_result;
            while ((area_result = area_constraint()) != 0)
            {
                bool which_die = (area_result == 1) ? false : true;
                bool found = false;
                for (auto &it : gain_map)
                {
                    for (auto i : it.second)
                    {
                        if (cells[i].part == which_die)
                        {
                            CrossNets_N -= it.first;
                            gain_update(i);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                }
            }
        }

        void net()
        {
            ofstream out_file("net.hgr");
            out_file << global_inst->net_num << " " << global_inst->cell_inst_num << endl;
            unordered_set<int> set;

            for (auto i = 0; i < global_inst->net_num; i++)
            {
                for (auto j = 0; j < global_inst->net_pin_num[i]; j++)
                {

                    if (!set.count(global_inst->cell_inst_map[global_inst->net_cell_name[i][j]]))
                    {
                        set.insert(global_inst->cell_inst_map[global_inst->net_cell_name[i][j]]);
                        out_file << global_inst->cell_inst_map[global_inst->net_cell_name[i][j]] + 1 << " ";
                    }
                    else
                        continue;
                }
                out_file << endl;
                cout << endl;
                set.clear();
            }
        }

        void read_net()
        {
            ifstream in_file("net.hgr.part.2");
            stringstream ss;
            ss << in_file.rdbuf();
            int die, cut = 0;
            cells.resize(global_inst->cell_inst_num);
            for (int i = 0; i < global_inst->cell_inst_num; i++)
            {
                ss >> die;
                if (die == 0)
                {
                    cells[i].part = false;
                    total_area[0] += top.tech->area(global_inst->type_name[i]);
                }
                else
                {
                    cells[i].part = true;
                    total_area[1] += bottom.tech->area(global_inst->type_name[i]);
                }
            }
            unordered_set<int> set;
            set.clear();
            for (int i = 0; i < global_inst->net_num; i++)
            {
                for (int j = 0; j < global_inst->net_pin_num[i]; j++)
                {
                    int cell_encoding = global_inst->cell_inst_map[global_inst->net_cell_name[i][j]]; // instance encoding
                    if (!set.count(cell_encoding))
                    {
                        set.insert(cell_encoding);
                        if (cells[cell_encoding].part == false)
                        {
                            nets_infro[i].size[0]++;
                            nets_infro[i].part0.insert(cell_encoding);
                        }
                        else
                        {
                            nets_infro[i].size[1]++;
                            nets_infro[i].part1.insert(cell_encoding);
                        }
                    }
                    else
                        continue;
                }
                if (nets_infro[i].size[1] > 0 && nets_infro[i].size[0] > 0)
                    cut++;
                set.clear();
            }
            CrossNets_N = cut;
            printlog(LOG_INFO, "#init crossnets: : %d", CrossNets_N);
        }
        // Output to gloabl
        void output_to_global(Instance &I)
        {
            for (int i = 0; i < I.cell_inst_num; i++)
            {
                if (cells[i].part == false)
                {
                    I.cell_inst_die[i] = &top;
                }
                else
                {
                    I.cell_inst_die[i] = &bottom;
                }
            }
        }
    };
    // FM
    FM f;
    // after placement
    map<string, pair<int, int>> cell_xy; // leftbottom
    unordered_map<int, pair<int, int>> terminals_map;
    vector<vector<int>> terminals_grid;

    // Others
    Instance()
    {
        net_num = 0;
        cell_inst_num = 0;
        f.set_Instance(this);
    }
    void setup_cells_areas()
    {
        for (int i = 0; i < cell_inst_num; i++)
        {
            Tech &a = *(cell_inst_die[i]->tech);
            if (a.typeMap.find(type_name[i]) == a.typeMap.end())
            {
                cerr << "Instance setting up the area of instance:" << type_name[i] << "with error!" << endl;
                continue;
            }
            cell_instances_areas[i] = a.type_areas[a.typeMap[type_name[i]]];
        }
    }
    void setup_cell_net_map()
    {
        for (int i = 0; i < net_num; i++)
        {
            for (int j = 0; j < net_pin_num[i]; j++)
            {
                int index = cell_inst_map[net_cell_name[i][j]];
                if (cell_net_map.find(index) == cell_net_map.end())
                {
                    cell_net_map[index] = (*new unordered_set<int>);
                }
                cell_net_map[index].insert(i);
            }
        }
    }
    pair<int, int> pin_result(int i, int j)
    {
        if (cell_inst_map.find(net_cell_name[i][j]) == cell_inst_map.end() || cell_xy.find(net_cell_name[i][j]) == cell_xy.end() || cell_inst_map.find(net_cell_name[i][j]) == cell_inst_map.end())
        {
            cerr << "Error to find pin result with cell_name:" << net_cell_name[i][j] << endl;
        }
        int cell_index = cell_inst_map[net_cell_name[i][j]];
        string tpName;
        string tname = type_name[cell_index];
        tpName.append(tname);
        tpName.append(":");
        tpName.append(net_pin_name[i][j]);
        pair<int, int> base = cell_xy[net_cell_name[i][j]];
        Die_infro *die = cell_inst_die[cell_index];
        Tech *tech = die->tech;
        int type_index = tech->typeMap[tname];
        int pin_index = tech->typePinMap[tpName];
        base.first += tech->typePinX[type_index][pin_index];
        base.second += tech->typePinY[type_index][pin_index];
        return base;
    }
    Die_infro *pin_to_die(int i, int j)
    {
        if (cell_inst_map.find(net_cell_name[i][j]) == cell_inst_map.end())
        {
            cerr << "Error to find pin_to_die with cell_name:" << net_cell_name[i][j] << endl;
        }
        int cell_index = cell_inst_map[net_cell_name[i][j]];
        return cell_inst_die[cell_index];
    }
    Die_infro *cell_to_die(string c)
    {
        if (cell_inst_map.find(c) == cell_inst_map.end())
        {
            cerr << "Error to find cell_to_die with cell_name:" << c << endl;
        }
        int cell_index = cell_inst_map[c];
        return cell_inst_die[cell_index];
    }
    pair<int, int> cell_rxy(string c)
    {
        if (cell_inst_map.find(c) == cell_inst_map.end() || cell_xy.find(c) == cell_xy.end() || cell_inst_map.find(c) == cell_inst_map.end())
        {
            cerr << "Error to find cell_rxy result with cell_name:" << c << endl;
        }
        int cell_index = cell_inst_map[c];
        string tname = type_name[cell_index];
        pair<int, int> base = cell_xy[c];
        Die_infro *die = cell_inst_die[cell_index];
        Tech *tech = die->tech;
        int type_index = tech->typeMap[tname];
        base.first += tech->typeWidth[type_index];
        base.second += tech->typeHeight[type_index];
        return base;
    }
    pair<int, int> cell_wh(string c)
    {
        if (cell_inst_map.find(c) == cell_inst_map.end() || cell_inst_map.find(c) == cell_inst_map.end())
        {
            cerr << "Error to find cell_wh result with cell_name:" << c << endl;
        }
        int cell_index = cell_inst_map[c];
        string tname = type_name[cell_index];
        Die_infro *die = cell_inst_die[cell_index];
        Tech *tech = die->tech;
        int type_index = tech->typeMap[tname];
        return make_pair(tech->typeWidth[type_index], tech->typeHeight[type_index]);
    }
    long long getCost()
    {
        long long hpwl = 0;
        for (int i = 0; i < net_num; i++)
        {
            int top_minX = INT_MAX, top_maxX = INT_MIN, top_minY = INT_MAX, top_maxY = INT_MIN;
            int bot_minX = INT_MAX, bot_maxX = INT_MIN, bot_minY = INT_MAX, bot_maxY = INT_MIN;
            int x, y;
            for (int j = 0; j < net_pin_num[i]; j++)
            {
                x = cell_xy[net_cell_name[i][j]].first;
                y = cell_xy[net_cell_name[i][j]].second;
                if (cell_to_die(net_cell_name[i][j]) == &bottom) // bot
                {
                    bot_minX = min(x, bot_minX);
                    bot_maxX = max(x, bot_maxX);
                    bot_minY = min(y, bot_minY);
                    bot_maxY = max(y, bot_maxY);
                }
                else
                {
                    top_minX = min(x, top_minX);
                    top_maxX = max(x, top_maxX);
                    top_minY = min(y, top_minY);
                    top_maxY = max(y, top_maxY);
                }
            }
            if (terminals_map.count(i))
            {
                x = terminals_map[i].second * top.Terminal_w + top.Terminal_w / 2 + top.Terminal_spacing;
                y = terminals_map[i].first * top.Terminal_h + top.Terminal_h / 2 + top.Terminal_spacing;
                bot_minX = min(x, bot_minX);
                bot_maxX = max(x, bot_maxX);
                bot_minY = min(y, bot_minY);
                bot_maxY = max(y, bot_maxY);
                top_minX = min(x, top_minX);
                top_maxX = max(x, top_maxX);
                top_minY = min(y, top_minY);
                top_maxY = max(y, top_maxY);
            }
            hpwl += ((top_maxX - top_minX) + (top_maxY - top_minY) + (bot_maxX - bot_minX) + (bot_maxY - bot_minY));
        }
        return hpwl;
    }
    int terminal_grids_check()
    {
        int blank = 0;
        for (int i = 0; i < grid_row; i++)
        {
            for (int j = 0; j < grid_col; j++)
            {
                if (terminals_grid[i][j] == -1)
                    blank++;
            }
        }
        return blank;
    }
    bool terminal_BFS(int net_idx, int cross_type)
    {
        // init
        int top_lowleft[2], top_topright[2], bot_lowleft[2], bot_topright[2];
        for (int i = 0; i < 2; i++)
        {
            top_lowleft[i] = INT_MAX;
            bot_lowleft[i] = INT_MAX;
            top_topright[i] = INT_MIN;
            bot_topright[i] = INT_MIN;
        }
        int pin_x, pin_y;
        int row, col, centerX, centerY;
        int i = net_idx;
        for (int j = 0; j < net_pin_num[i]; j++)
        {
            pair<int, int> pin_xy = pin_result(i, j);
            pin_x = pin_xy.first;
            pin_y = pin_xy.second;
            Die_infro *now_die = pin_to_die(i, j);
            if (now_die == &top)
            {
                top_lowleft[0] = min(pin_x, top_lowleft[0]);
                top_lowleft[1] = min(pin_y, top_lowleft[1]);
                top_topright[0] = max(pin_x, top_topright[0]);
                top_topright[1] = max(pin_y, top_topright[1]);
            }
            else if (now_die == &bottom)
            {
                bot_lowleft[0] = min(pin_x, bot_lowleft[0]);
                bot_lowleft[1] = min(pin_y, bot_lowleft[1]);
                bot_topright[0] = max(pin_x, bot_topright[0]);
                bot_topright[1] = max(pin_y, bot_topright[1]);
            }
        }
        if (cross_type == 0)
        {
            int minX2 = min(top_topright[0], bot_topright[0]);
            int maxX1 = max(top_lowleft[0], bot_lowleft[0]);
            int minY2 = min(top_topright[1], bot_topright[1]);
            int maxY1 = max(top_lowleft[1], bot_lowleft[1]);
            centerX = (minX2 + maxX1) / 2;
            centerY = (minY2 + maxY1) / 2;
        }
        else if (cross_type == 1) // bot = 1
        {
            // deal X
            if (bot_lowleft[0] <= top_topright[0] && bot_lowleft[0] >= top_lowleft[0])
                centerX = bot_lowleft[0];
            else if (bot_lowleft[0] > top_topright[0])
                centerX = (bot_lowleft[0] + top_topright[0]) / 2;
            else
                centerX = (bot_lowleft[0] + top_lowleft[0]) / 2;
            // deal Y
            if (bot_lowleft[1] <= top_topright[1] && bot_lowleft[1] >= top_lowleft[1])
                centerY = bot_lowleft[1];
            else if (bot_lowleft[1] > top_topright[1])
                centerY = (bot_lowleft[1] + top_topright[1]) / 2;
            else
                centerY = (bot_lowleft[1] + top_lowleft[1]) / 2;
        }
        else if (cross_type == 2)
        {
            // deal X
            if (top_lowleft[0] <= bot_topright[0] && top_lowleft[0] >= bot_lowleft[0])
                centerX = top_lowleft[0];
            else if (top_lowleft[0] > bot_topright[0])
                centerX = (top_lowleft[0] + bot_topright[0]) / 2;
            else
                centerX = (top_lowleft[0] + bot_lowleft[0]) / 2;
            // deal Y
            if (top_lowleft[1] <= bot_topright[1] && top_lowleft[1] >= bot_lowleft[1])
                centerY = top_lowleft[1];
            else if (top_lowleft[1] > bot_topright[1])
                centerY = (top_lowleft[1] + bot_topright[1]) / 2;
            else
                centerY = (top_lowleft[1] + bot_lowleft[1]) / 2;
        }
        else
        {
            centerX = (top_lowleft[0] + bot_topright[0]) / 2;
            centerY = (top_lowleft[1] + bot_lowleft[1]) / 2;
        }
        // BFS
        row = centerY / top.Terminal_h, col = centerX / top.Terminal_w;
        if (row >= grid_row)
            row = grid_row - 1;
        else if (col >= grid_col)
            col = grid_col - 1;
        queue<pair<int, int>> q_pair;
        vector<vector<int>> area_vec = {{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}};
        vector<int> dir_vec = {1, 0, -1, 0, 1};
        q_pair.push(make_pair(row, col));
        while (!q_pair.empty())
        {
            row = q_pair.front().first;
            col = q_pair.front().second;
            q_pair.pop();
            if (debug_terminal)
            {
                cout << "remained:" << terminal_grids_check() << endl;
                cout << "WANT TO:" << net_idx << endl;
                cout << terminals_grid[row][col] << "ROWCOL:" << row << " " << col << endl;
            }
            if (terminals_grid[row][col] == -1)
            {
                terminals_grid[row][col] = net_idx;
                terminals_map[net_idx] = make_pair(row, col);
                for (int i = 0; i < 8; i++)
                    if (row + area_vec[i][0] >= 0 && row + area_vec[i][0] <= grid_row - 1 && col + area_vec[i][1] >= 0 && col + area_vec[i][1] <= grid_col - 1)
                        terminals_grid[row + area_vec[i][0]][col + area_vec[i][1]] = net_idx;
                return true;
            }
            else
            {
                terminals_grid[row][col] = net_idx;
                for (int i = 0; i < 4; i++)
                    if (row + dir_vec[i] >= 0 && row + dir_vec[i] <= grid_row - 1 && col + dir_vec[i + 1] >= 0 && col + dir_vec[i + 1] <= grid_col - 1 && terminals_grid[row + dir_vec[i]][col + dir_vec[i + 1]] != net_idx)
                        q_pair.push(make_pair(row + dir_vec[i], col + dir_vec[i + 1]));
            }
        }
        return false;
    }
    int grid_row, grid_col;
    bool debug_terminal;
    bool terminal_insert()
    {
        // terminal insert at top
        // cout<<"terminal_insert"<<endl;
        terminals_map.clear();
        grid_row = int((top.dieheight() - top.Terminal_spacing * 2) / top.Terminal_h);
        grid_col = int((top.width() - top.Terminal_spacing * 2) / top.Terminal_w);
        terminals_grid = vector<vector<int>>(grid_row, vector<int>(grid_col, -1));
        debug_terminal = false;
        for (auto &it : f.crossNets_pair)
        {
            // if (it.first == 2374)
            // debug_terminal = true;
            if (terminal_BFS(it.first, it.second) == false)
            {

                return false;
            }
        }
        return true;
    }
    void layout_info()
    {
        ofstream outfile("layout.txt");
        if (!outfile.good())
        {
            printlog(LOG_ERROR, "cannot open file: layout.txt");
            return;
        }
        outfile << top.rowheight << " " << top.height() << " " << top.width()
                << " " << bottom.rowheight << " " << bottom.height() << " " << bottom.width() << endl;
        outfile << cell_inst_num << " " << terminals_map.size() << endl;
        for (auto &it : cell_xy) // x,y,width,height,idx
        {
            outfile << cell_inst_map[it.first] + 1 << " " << ((cell_to_die(it.first) == &top) ? 0 : 1) << " " << it.second.first << " " << it.second.second;
            outfile << " " << cell_wh(it.first).first << " " << cell_wh(it.first).second << endl;
        }

        for (auto &it : terminals_map)
        {
            int x = terminals_map[it.first].second * top.Terminal_w + top.Terminal_spacing;
            int y = terminals_map[it.first].first * top.Terminal_h + top.Terminal_spacing;
            outfile << x << " " << y << " " << top.Terminal_w << " " << top.Terminal_h << endl;
        }
        outfile.close();
    }
    bool writeICCAD2022(const std::string &file)
    {
        ofstream out_file(file.c_str());
        if (!out_file.good())
        {
            printlog(LOG_ERROR, "cannot open file: %s", file);
            return false;
        }
        out_file << "TopDiePlacement " << top.cell_num << endl;
        for (int i = 0; i < cell_inst_num; i++)
            if (cell_inst_die[i] == &top)
                out_file << "Inst " << cName[i] << " " << cell_xy[cName[i]].first << " " << cell_xy[cName[i]].second << endl;
        out_file << "BottomDiePlacement " << bottom.cell_num << endl;
        for (int i = 0; i < cell_inst_num; i++)
            if (cell_inst_die[i] == &bottom)
                out_file << "Inst " << cName[i] << " " << cell_xy[cName[i]].first << " " << cell_xy[cName[i]].second << endl;
        cout << "check match?" << f.CrossNets_N << " " << (f.CrossNets_N == (int)terminals_map.size()) << endl;
        out_file << "NumTerminals " << f.CrossNets_N << endl;
        for (int i = 0; i < net_num; i++)
        {
            if (terminals_map.count(i))
            {
                int x = terminals_map[i].second * top.Terminal_w;
                int y = terminals_map[i].first * top.Terminal_h;
                out_file << "Terminal "
                         << "N" << i + 1 << " " << (x + top.Terminal_w * 0.5 + top.Terminal_spacing) << " " << (y + top.Terminal_h * 0.5 + top.Terminal_spacing) << endl;
            }
        }
        out_file.close();
        return true;
    }
    bool write_Place_result(bool TOP, string file = "")
    {
        if (TOP == false)
        {
            ofstream out_file("placement_r");
            ofstream out2_file("net.hgr.part.2");
            if (!out_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: placement_r");
                return false;
            }
            if (!out2_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: placement_r");
                return false;
            }
            for (int i = 0; i < cell_inst_num; i++)
            {
                out2_file << (cell_inst_die[i] == &top ? 0 : 1) << endl;
            }
            out2_file.close();
            for (int i = 0; i < cell_inst_num; i++)
            {
                out_file << cell_xy[cName[i]].first << " " << cell_xy[cName[i]].second << endl;
            }
            out_file.close();
        }
        else
        {
            ofstream out_file(file);
            ofstream out2_file("net.hgr.part.2");
            if (!out_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
                return false;
            }
            if (!out2_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: net.hgr.part.2");
                return false;
            }
            for (int i = 0; i < cell_inst_num; i++)
            {
                out2_file << (cell_inst_die[i] == &top ? 0 : 1) << endl;
            }
            out2_file.close();
            out_file << top.cell_num << endl;
            for (int i = 0; i < cell_inst_num; i++)
            {
                if (cell_inst_die[i] == &top)
                    out_file << i << " " << cell_xy[cName[i]].first << " " << cell_xy[cName[i]].second << endl;
            }
            out_file.close();
        }
        return true;
    }
    bool load_Place_result(bool ANS, string file = "")
    {
        f.read_net();
        f.area_adjust();
        f.ans();
        if (ANS == false)
        {
            ifstream in_file("placement_r");
            if (!in_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: placement_r");
                return false;
            }
            for (int i = 0; i < cell_inst_num; i++)
            {
                int x, y;
                in_file >> x >> y;
                cell_xy[cName[i]] = make_pair(x, y);
            }
            in_file.close();
            printlog(LOG_INFO, "Reload placement success!");
        }
        else
        {
            ifstream in_file(file);
            if (!in_file.good())
            {
                printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
                return false;
            }
            int top_num;
            in_file >> top_num;
            for (int i = 0; i < top_num; i++)
            {
                int index, x, y;
                in_file >> index >> x >> y;
                cell_xy[cName[index]] = make_pair(x, y);
            }
            in_file.close();
            printlog(LOG_INFO, "Reload top die result success!");
        }
        return true;
    }
};
Instance global_inst;

// ripple
class BookshelfData
{
public:
    int nCells;
    int nTypes;
    unsigned nNets;
    int nRows;

    std::string format;
    map<string, int> cellMap;
    map<string, int> typeMap;
    vector<string> cellName;
    vector<int> cellX;
    vector<int> cellY;
    vector<char> cellFixed;
    vector<string> typeName;
    vector<int> cellType;
    vector<int> typeWidth;
    vector<int> typeHeight;
    vector<char> typeFixed;
    map<string, int> typePinMap;
    vector<int> typeNPins;
    vector<vector<string>> typePinName;
    vector<vector<char>> typePinDir;
    vector<vector<double>> typePinX;
    vector<vector<double>> typePinY;
    vector<string> netName;
    vector<vector<int>> netCells;
    vector<vector<int>> netPins;
    vector<int> rowX;
    vector<int> rowY;
    vector<int> rowSites;

    int siteWidth;
    int siteHeight;

    int gridNX;
    int gridNY;
    int gridNZ;
    vector<int> capV;
    vector<int> capH;
    vector<int> wireWidth;
    vector<int> wireSpace;
    vector<int> viaSpace;
    int gridOriginX;
    int gridOriginY;
    int tileW;
    int tileH;
    double blockagePorosity;
    // ICCAD2022
    void load_Die(Die_infro &a)
    {
        nRows = a.nRows;
        rowX = a.rowX;
        rowY = a.rowY;
        rowSites = a.rowSites;
        load_tech(*a.tech);
    }
    void load_tech(Tech &a)
    {
        nCells = a.nCells;
        nTypes = a.nTypes;
        nNets = a.nNets;
        cellMap = a.cellMap;
        typeMap = a.typeMap;
        cellName = a.cellName;
        cellX = a.cellX;
        cellY = a.cellY;
        cellFixed = a.cellFixed;
        typeName = a.typeName;
        cellType = a.cellType;
        typeWidth = a.typeWidth;
        typeHeight = a.typeHeight;
        typeFixed = a.typeFixed;
        typePinMap = a.typePinMap;
        typeNPins = a.typeNPins;
        typePinName = a.typePinName;
        typePinDir = a.typePinDir;
        typePinX = a.typePinX;
        typePinY = a.typePinY;
        netName = a.netName;
        netCells = a.netCells;
        netPins = a.netPins;
    }
    BookshelfData()
    {
        nCells = 0;
        nTypes = 0;
        nNets = 0;
        nRows = 0;
    }

    void clearData()
    {
        cellMap.clear();
        typeMap.clear();
        cellName.clear();
        cellX.clear();
        cellY.clear();
        cellFixed.clear();
        typeName.clear();
        cellType.clear();
        typeWidth.clear();
        typeHeight.clear();
        typePinMap.clear();
        typeNPins.clear();
        typePinName.clear();
        typePinDir.clear();
        typePinX.clear();
        typePinY.clear();
        netName.clear();
        netCells.clear();
        netPins.clear();
        rowX.clear();
        rowY.clear();
        rowSites.clear();
    }

    void scaleData(int scale)
    {
        for (int i = 0; i < nCells; i++)
        {
            cellX[i] *= scale;
            cellY[i] *= scale;
        }
        for (int i = 0; i < nTypes; i++)
        {
            typeWidth[i] *= scale;
            typeHeight[i] *= scale;
            for (int j = 0; j < typeNPins[i]; j++)
            {
                typePinX[i][j] *= scale;
                typePinY[i][j] *= scale;
            }
        }
        for (int i = 0; i < nRows; i++)
        {
            rowX[i] *= scale;
            rowY[i] *= scale;
        }
        siteWidth *= scale;
        siteHeight *= scale;
    }

    void estimateSiteSize()
    {
        set<int> sizeSet;
        set<int> heightSet;

        for (int i = 0; i < nCells; i++)
        {
            if (cellFixed[i] == (char)1)
            {
                continue;
            }
            int type = cellType[i];
            int typeW = typeWidth[type];
            int typeH = typeHeight[type];
            if (sizeSet.count(typeW) == 0)
            {
                sizeSet.insert(typeW);
            }
            if (sizeSet.count(typeH) == 0)
            {
                sizeSet.insert(typeH);
            }
            if (heightSet.count(typeH) == 0)
            {
                heightSet.insert(typeH);
            }
        }

        vector<int> sizes;
        vector<int> heights;
        set<int>::iterator ii = sizeSet.begin();
        set<int>::iterator ie = sizeSet.end();
        for (; ii != ie; ++ii)
        {
            sizes.push_back(*ii);
        }
        ii = heightSet.begin();
        ie = heightSet.end();
        for (; ii != ie; ++ii)
        {
            heights.push_back(*ii);
        }
        siteWidth = gcd(sizes);
        siteHeight = gcd(heights);
        printlog(LOG_INFO, "estimate site size = %d x %d", siteWidth, siteHeight);
        ii = heightSet.begin();
        ie = heightSet.end();
        for (; ii != ie; ++ii)
        {
            printlog(LOG_INFO, "standard cell heights: %d rows", (*ii) / siteHeight);
        }
    }
    int gcd(vector<int> &nums)
    {
        if (nums.size() == 0)
        {
            return 0;
        }
        if (nums.size() == 1)
        {
            return nums[0];
        }
        int primes[20] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71};
        int greatestFactor = 1;
        for (bool factorFound = true; factorFound;)
        {
            factorFound = false;
            for (int f = 0; f < 10; f++)
            {
                int factor = primes[f];
                bool factorValid = true;
                for (int i = 0; i < (int)nums.size(); i++)
                {
                    int num = nums[i];
                    // printlog(LOG_INFO, "%d : %d", i, num);
                    if (num % factor != 0)
                    {
                        factorValid = false;
                        break;
                    }
                }
                if (!factorValid)
                {
                    continue;
                }
                greatestFactor *= factor;
                for (int i = 0; i < (int)nums.size(); i++)
                {
                    nums[i] /= factor;
                }
                factorFound = true;
                break;
            }
        }
        return greatestFactor;
    }
};

BookshelfData bsData;

bool isBookshelfSymbol(unsigned char c)
{
    static char symbols[256] = {0};
    static bool inited = false;
    if (!inited)
    {
        symbols[(int)'('] = 1;
        symbols[(int)')'] = 1;
        symbols[(int)'/'] = 1;
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

bool readBSLine(istream &is, vector<string> &tokens)
{
    tokens.clear();
    string line;
    while (is && tokens.empty())
    {
        // read next line in
        getline(is, line);
        char token[1024] = {0};
        int lineLen = (int)line.size();
        int tokenLen = 0;
        for (int i = 0; i < lineLen; i++)
        {
            char c = line[i];
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

void printTokens(vector<string> &tokens)
{
    for (auto const &token : tokens)
    {
        std::cout << token << " : ";
    }
    std::cout << std::endl;
}
bool Database::readBSAux(const std::string &auxFile, const std::string &plFile)
{
    std::string directory;
    bsData.format = io::IOModule::BookshelfVariety;
    bsData.nCells = 0;
    bsData.nTypes = 0;
    bsData.nNets = 0;
    bsData.nRows = 0;

    std::string fileNodes;
    std::string fileNets;
    std::string fileScl;
    std::string fileRoute;
    std::string fileShapes;
    std::string fileWts;
    std::string filePl;
    unsigned found = auxFile.find_last_of("/\\");
    if (found == auxFile.npos)
    {
        directory = "./";
    }
    else
    {
        directory = auxFile.substr(0, found);
        directory += "/";
    }

    printlog(LOG_INFO, "dir = %s", directory.c_str());

    ifstream fs(auxFile.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", auxFile.c_str());
        return false;
    }

    vector<string> tokens;
    readBSLine(fs, tokens);
    fs.close();

    for (int i = 1; i < (int)tokens.size(); i++)
    {
        const string &file = tokens[i];
        const size_t dotPos = file.find_last_of(".");
        if (dotPos == string::npos)
        {
            continue;
        }

        string ext = file.substr(dotPos);
        if (ext == ".nodes")
        {
            fileNodes = directory + file;
        }
        else if (ext == ".nets")
        {
            fileNets = directory + file;
        }
        else if (ext == ".scl")
        {
            fileScl = directory + file;
        }
        else if (ext == ".route")
        {
            fileRoute = directory + file;
        }
        else if (ext == ".shapes")
        {
            fileShapes = directory + file;
        }
        else if (ext == ".wts")
        {
            fileWts = directory + file;
        }
        else if (ext == ".pl")
        {
            filePl = directory + file;
        }
        else
        {
            printlog(LOG_ERROR, "unrecognized file extension: %s", ext.c_str());
        }
    }

    // step 1: read floorplan, rows from:
    //  scl - rows
    // step 2: read cell types from:
    //  nets - pin name, pin offset
    //  nodes - with, height, cell type name
    // step 3: read cell from:
    //  nodes - cell name, type
    // step 4: read placement:
    //  pl - cell fix/movable, position

    // bsData.siteWidth = 240;
    // bsData.siteHeight = 2880;
    readBSNodes(fileNodes);

    bsData.estimateSiteSize();

    if (bsData.format == "iccad2012")
    {
        readBSNets(fileNets);
        readBSRoute(fileRoute);
        readBSShapes(fileShapes);
        readBSWts(fileWts);
        // readBSPl    ( filePl    );
        readBSPl(plFile);
        readBSScl(fileScl);
    }
    else if (bsData.format == "wu2016" || bsData.format == "lin2016")
    {
        readBSNets(fileNets);
        // readBSRoute ( fileRoute );
        // readBSShapes( fileShapes);
        readBSWts(fileWts);
        // readBSPl    ( filePl    );
        readBSPl(plFile);
        readBSScl(fileScl);
    }
    else if (bsData.format == "dac2012")
    {
        readBSNets(fileNets);
        readBSRoute(fileRoute);
        readBSShapes(fileShapes);
        readBSWts(fileWts);
        readBSPl(filePl);
        readBSScl(fileScl);
    }
    // data1
    if (bsData.siteWidth < 10)
    {
        bsData.scaleData(100);
        // database.scale = 100;
    }
    else if (bsData.siteWidth < 100)
    {
        bsData.scaleData(10);
        // database.scale = 10;
    }
    else
    {
        // database.scale = 1;
    }
    database.dieLX = INT_MAX;
    database.dieLY = INT_MAX;
    database.dieHX = INT_MIN;
    database.dieHY = INT_MIN;

    for (int i = 0; i < bsData.nRows; i++)
    {
        Row *row = database.addRow("core_SITE_ROW_" + to_string(i), "core", bsData.rowX[i], bsData.rowY[i]);
        row->xStep(bsData.siteWidth);
        row->yStep(bsData.siteHeight);
        row->xNum(bsData.rowSites[i]);
        row->yNum(1);
        row->flip((i % 2) == 1);
        database.dieLX = std::min(database.dieLX, row->x());
        database.dieLY = std::min(database.dieLY, row->y());
        database.dieHX = std::max(database.dieHX, row->x() + (int)row->width());
        database.dieHY = std::max(database.dieHY, row->y() + bsData.siteHeight);
    }
    int defaultPitch = bsData.siteWidth;
    int defaultWidth = bsData.siteWidth / 2;
    int defaultSpace = bsData.siteWidth - defaultWidth;
    unsigned nLayers = 9;
    for (unsigned i = 0; i != nLayers; ++i)
    {
        Layer &layer = database.addLayer(string("M").append(to_string(i + 1)), 'r');
        layer.direction = 'h';
        layer.pitch = defaultPitch;
        layer.offset = defaultSpace;
        layer.width = defaultWidth;
        layer.spacing = defaultSpace;
        if (!i)
        {
            layer.track.direction = 'x';
        }
        else if (i % 2)
        {
            layer.track.direction = 'v';
        }
        else
        {
            layer.track.direction = 'h';
        }
        if (i % 2)
        {
            layer.track.start = database.dieLX + (layer.pitch / 2);
            layer.track.num = (database.dieHX - database.dieLX) / layer.pitch;
        }
        else
        {
            layer.track.start = database.dieLY + (layer.pitch / 2);
            layer.track.num = (database.dieHY - database.dieLY) / layer.pitch;
        }
        if (layer.track.num <= 0)
            layer.track.num = 1;
        layer.track.step = layer.pitch;

        if (i + 1 == nLayers)
        {
            break;
        }
        else
        {
            database.addLayer(string("N").append(to_string(i + 1)), 'c');
        }
    }

    const Layer &layer = database.layers[0];

    for (int i = 0; i < bsData.nTypes; i++)
    {
        unsigned libcell = rtimer.lLib.addCell(bsData.typeName[i]);
        CellType *celltype = database.addCellType(bsData.typeName[i], libcell);
        celltype->width = bsData.typeWidth[i];
        celltype->height = bsData.typeHeight[i];
        celltype->stdcell = (!bsData.typeFixed[i]);
        for (int j = 0; j < bsData.typeNPins[i]; ++j)
        {
            char direction = 'x';
            switch (bsData.typePinDir[i][j])
            {
            case 'I':
                direction = 'i';
                break;
            case 'O':
                direction = 'o';
                break;
            default:
                // printlog(LOG_ERROR, "unknown pin direction: %c", bsData.typePinDir[i][j]);
                break;
            }
            PinType *pintype = celltype->addPin(bsData.typePinName[i][j], direction, 's');
            pintype->addShape(layer, bsData.typePinX[i][j], bsData.typePinY[i][j]);
        }
    }

    bool isWu2016 = (bsData.format == "wu2016");
    for (int i = 0; i < bsData.nCells; i++)
    {
        int typeID = bsData.cellType[i];
        if (typeID < 0)
        {
            IOPin *iopin = database.addIOPin(bsData.cellName[i]);
            switch (typeID)
            {
            case -1:
                iopin->type->direction('o');
                break;
            case -2:
                iopin->type->direction('i');
                break;
            default:
                iopin->type->direction('x');
                break;
            }
            iopin->type->addShape(layer, bsData.cellX[i], bsData.cellY[i]);
        }
        else
        {
            string celltypename(bsData.typeName[typeID]);
            Cell *cell = database.addCell(bsData.cellName[i], database.getCellType(celltypename));
            cell->place(bsData.cellX[i], bsData.cellY[i], false, false);
            cell->fixed((bsData.cellFixed[i] == (char)1));
            if (!cell->fixed() && isWu2016 && cell->height() > 2 * bsData.siteHeight)
            {
                printlog(LOG_ERROR,
                         "cell %s [w=%d h=%d] is forced to be fixed at (%d,%d)",
                         cell->name().c_str(),
                         cell->width(),
                         cell->height(),
                         cell->lx(),
                         cell->ly());
                cell->highlighted = true;
                cell->fixed(true);
            }
        }
    }

    for (unsigned i = 0; i != bsData.nNets; ++i)
    {
        Net *net = database.addNet(bsData.netName[i]);
        for (unsigned j = 0; j != bsData.netCells[i].size(); ++j)
        {
            Pin *pin = nullptr;
            int cellID = bsData.netCells[i][j];
            if (bsData.cellType[cellID] < 0)
            {
                IOPin *iopin = database.getIOPin(bsData.cellName[cellID]);
                pin = iopin->pin;
            }
            else
            {
                Cell *cell = database.getCell(bsData.cellName[cellID]);
                pin = cell->pin(bsData.netPins[i][j]);
            }
            pin->net = net;
            net->addPin(pin);
        }
    }

    bsData.clearData();
    return true;
}
bool Database::readBSNodes(const std::string &file)
{
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    // int nNodes = 0;
    // int nTerminals = 0;
    vector<string> tokens;
    while (readBSLine(fs, tokens))
    {
        // printlog(LOG_INFO, "%d : %s", i++, tokens[0].c_str());
        if (tokens[0] == "UCLA")
        {
            continue;
        }
        else if (tokens[0] == "NumNodes")
        {
            // nNodes = atoi(tokens[1].c_str());
        }
        else if (tokens[0] == "NumTerminals")
        {
            // nTerminals = atoi(tokens[1].c_str());
        }
        else if (tokens.size() >= 3)
        {
            string cName = tokens[0];
            int cWidth = atoi(tokens[1].c_str());
            int cHeight = atoi(tokens[2].c_str());
            bool cFixed = false;
            string cType = cName;
            if (tokens.size() >= 4)
            {
                cType = tokens[3];
                //  cout << cName << '\t' << cType << endl;
            }
            if (cType == "terminal" && cWidth > 1 && cHeight > 1)
            {
                // printlog(LOG_INFO, "read terminal");
                cType = cName;
                cFixed = true;
            }
            if (cType == "terminal_NI")
            {
                cType = cName;
                cFixed = true;
            }
            int typeID = -1;
            if (cType == "terminal")
            {
                // printlog(LOG_INFO, "read terminal");
                typeID = -1;
                cFixed = true;
            }
            else if (bsData.typeMap.find(cType) == bsData.typeMap.end())
            {
                typeID = bsData.nTypes++;
                bsData.typeMap[cType] = typeID;
                bsData.typeName.push_back(cType);
                bsData.typeWidth.push_back(cWidth);
                bsData.typeHeight.push_back(cHeight);
                bsData.typeFixed.push_back((char)(cFixed ? 1 : 0));
                bsData.typeNPins.push_back(0);
                bsData.typePinName.push_back(vector<string>());
                bsData.typePinDir.push_back(vector<char>());
                bsData.typePinX.push_back(vector<double>());
                bsData.typePinY.push_back(vector<double>());
                //  cout << cType << '\t' << tokens.size() << endl;
            }
            else
            {
                typeID = bsData.typeMap[cType];
                assert(cWidth == bsData.typeWidth[typeID]);
                assert(cHeight == bsData.typeHeight[typeID]);
            }
            int cellID = bsData.nCells++;
            bsData.cellMap[cName] = cellID;
            bsData.cellType.push_back(typeID);
            bsData.cellFixed.push_back((char)(cFixed ? 1 : 0));
            bsData.cellName.push_back(cName);
            bsData.cellX.push_back(0);
            bsData.cellY.push_back(0);
        }
    }

    fs.close();
    return true;
}
void check_input(vector<string> &s)
{
    // for (int i = 0; i < (int)s.size(); i++)
    // cout << s[i] << " ";
    // cout << endl;
}
bool Database::readBSNets(const std::string &file)
{
    std::cout << "reading net" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens))
    {
        if (tokens[0] == "UCLA")
        {
            continue;
        }
        else if (tokens[0] == "NumNets")
        {
            // printlog(LOG_INFO, "#nets : %d", atoi(tokens[1].c_str()));
            int numNets = atoi(tokens[1].c_str());
            bsData.netName.resize(numNets);
            bsData.netCells.resize(numNets);
            bsData.netPins.resize(numNets);
        }
        else if (tokens[0] == "NumPins")
        {
            // printlog(LOG_INFO, "#pins : %d", atoi(tokens[1].c_str()));
        }
        else if (tokens[0] == "NetDegree")
        {
            int degree = atoi(tokens[1].c_str());
            string nName = tokens[2];
            int netID = bsData.nNets++;
            bsData.netName[netID] = nName;
            for (int i = 0; i < degree; i++)
            {
                readBSLine(fs, tokens);
                string cName = tokens[0];
                if (bsData.cellMap.find(cName) == bsData.cellMap.end())
                {
                    assert(false);
                    printlog(LOG_ERROR, "cell not found : %s", cName.c_str());
                    return false;
                }

                int cellID = bsData.cellMap[cName];
                int typeID = bsData.cellType[cellID];
                int typePinID = -1;

                char dir = tokens[1].c_str()[0];
                double pinX = 0;
                double pinY = 0;
                pinX = bsData.typeWidth[typeID] * 0.5 + (double)atof(tokens[2].c_str());
                pinY = bsData.typeHeight[typeID] * 0.5 + (double)atof(tokens[3].c_str());
                /*
                if(tokens.size() >= 6){
                    //pinX = (int)round(atof(tokens[4].c_str())*bsData.siteWidth);
                    //pinY = (int)round(atof(tokens[5].c_str())*bsData.siteWidth);
                }else{
                    pinX = bsData.typeWidth[typeID]  * 0.5 + (int)round(atof(tokens[2].c_str()));
                    pinY = bsData.typeHeight[typeID] * 0.5 + (int)round(atof(tokens[3].c_str()));
                }
                */
                string pinName = (tokens.size() < 7) ? "" : tokens[6];

                string tpName;
                if (pinName == "" && typeID >= 0)
                {
                    stringstream ss;
                    ss << bsData.typeNPins[typeID];
                    pinName = ss.str();
                    // printlog(LOG_INFO, "pinname = %s", pinName.c_str());
                }
                if (typeID >= 0)
                {
                    tpName.append(bsData.typeName[typeID]);
                    tpName.append(":");
                    tpName.append(pinName);
                }
                if (typeID == -1)
                {
                    // IOPin
                    if (dir == 'I')
                    {
                        typeID = -1;
                    }
                    else if (dir == 'O')
                    {
                        typeID = -2;
                    }
                    else
                    {
                        typeID = -3;
                    }
                }
                else if (bsData.typePinMap.find(tpName) == bsData.typePinMap.end())
                {
                    typePinID = bsData.typeNPins[typeID]++;
                    bsData.typePinMap[tpName] = typePinID;
                    bsData.typePinName[typeID].push_back(pinName);
                    bsData.typePinDir[typeID].push_back(dir);
                    bsData.typePinX[typeID].push_back(pinX);
                    bsData.typePinY[typeID].push_back(pinY);
                }
                else
                {
                    typePinID = bsData.typePinMap[tpName];
                    assert(bsData.typePinX[typeID][typePinID] == pinX);
                    assert(bsData.typePinY[typeID][typePinID] == pinY);
                    assert(bsData.typePinDir[typeID][typePinID] == dir);
                }
                bsData.netCells[netID].push_back(cellID);
                bsData.netPins[netID].push_back(typePinID);
            }
        }
    }

    fs.close();
    return true;
}

bool Database::readBSScl(const std::string &file)
{
    std::cout << "reading scl" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens))
    {
        if (tokens[0] == "UCLA")
        {
            continue;
        }
        else if (tokens[0] == "NumRows")
        {
            int numRows = atoi(tokens[1].c_str());
            bsData.rowX.resize(numRows);
            bsData.rowY.resize(numRows);
            bsData.rowSites.resize(numRows);
        }
        else if (tokens[0] == "Coordinate")
        {
            bsData.rowY[bsData.nRows] = atoi(tokens[1].c_str());
        }
        else if (tokens[0] == "SubrowOrigin" && tokens[2] == "NumSites")
        {
            bsData.rowX[bsData.nRows] = atoi(tokens[1].c_str());
            if (bsData.format == "wu2016")
            {
                bsData.rowSites[bsData.nRows] = atoi(tokens[3].c_str()) / bsData.siteWidth;
            }
            else
            {
                bsData.rowSites[bsData.nRows] = atoi(tokens[3].c_str());
            }
        }
        else if (tokens[0] == "Sitewidth")
        {
            // bsData.siteWidth = atoi(tokens[1].c_str());
        }
        else if (tokens[0] == "End")
        {
            bsData.nRows++;
        }
    }
    fs.close();
    return true;
}

bool Database::readBSRoute(const std::string &file)
{
    std::cout << "reading route" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    const unsigned ReadingHeader = 0;
    const unsigned ReadingPinLayer = 1;
    const unsigned ReadingBlockages = 2;
    const unsigned ReadingCapAdjust = 3;
    unsigned status = ReadingHeader;
    while (readBSLine(fs, tokens))
    {
        if (tokens[0] == "Grid")
        {
            bsData.gridNX = atoi(tokens[1].c_str());
            bsData.gridNY = atoi(tokens[2].c_str());
            bsData.gridNZ = atoi(tokens[3].c_str());
        }
        else if (tokens[0] == "VerticalCapacity")
        {
            for (unsigned i = 1; i < tokens.size(); i++)
            {
                bsData.capV.push_back(atoi(tokens[i].c_str()));
            }
        }
        else if (tokens[0] == "HorizontalCapacity")
        {
            for (unsigned i = 1; i < tokens.size(); i++)
            {
                bsData.capH.push_back(atoi(tokens[i].c_str()));
            }
        }
        else if (tokens[0] == "MinWireWidth")
        {
            for (unsigned i = 1; i < tokens.size(); i++)
            {
                bsData.wireWidth.push_back(atoi(tokens[i].c_str()));
            }
        }
        else if (tokens[0] == "MinWireSpacing")
        {
            for (unsigned i = 1; i < tokens.size(); i++)
            {
                bsData.wireSpace.push_back(atoi(tokens[i].c_str()));
            }
        }
        else if (tokens[0] == "ViaSpacing")
        {
            for (unsigned i = 1; i < tokens.size(); i++)
            {
                bsData.viaSpace.push_back(atoi(tokens[i].c_str()));
            }
        }
        else if (tokens[0] == "GridOrigin")
        {
            bsData.gridOriginX = atoi(tokens[1].c_str());
            bsData.gridOriginY = atoi(tokens[2].c_str());
        }
        else if (tokens[0] == "TileSize")
        {
            bsData.tileW = atoi(tokens[1].c_str());
            bsData.tileH = atoi(tokens[2].c_str());
        }
        else if (tokens[0] == "BlockagePorosity")
        {
            bsData.blockagePorosity = atof(tokens[1].c_str());
        }
        else if (tokens[0] == "NumNiTerminals")
        {
            status = ReadingPinLayer;
        }
        else if (tokens[0] == "NumBlockageNodes")
        {
            status = ReadingBlockages;
        }
        else if (tokens[0] == "NumEdgeCapacityAdjustments")
        {
            status = ReadingCapAdjust;
        }
        else if (status == ReadingPinLayer)
        {
            std::string pin = tokens[0];
            //  int layer = atoi(tokens[1].c_str());
            if (bsData.cellMap.find(pin) == bsData.cellMap.end())
            {
                printlog(LOG_ERROR, "pin not found : %s", pin.c_str());
                getchar();
            }
            //  cout<<pin<<" "<<layer<<endl;
        }
        else if (status == ReadingBlockages)
        {
            std::string node = tokens[0];
            int maxLayer = 0;
            for (unsigned i = 2; i < tokens.size(); i++)
            {
                maxLayer = std::max(maxLayer, atoi(tokens[i].c_str()));
            }
            if (bsData.cellMap.find(node) == bsData.cellMap.end())
            {
                printlog(LOG_ERROR, "node not found : %s", node.c_str());
                getchar();
            }
            //  cout<<node<<" "<<maxLayer<<endl;
        }
        else if (status == ReadingCapAdjust)
        {
            /*
            int fx = atoi(tokens[0].c_str());
            int fy = atoi(tokens[1].c_str());
            int fz = atoi(tokens[2].c_str());
            int tx = atoi(tokens[3].c_str());
            int ty = atoi(tokens[4].c_str());
            int tz = atoi(tokens[5].c_str());
            int cap = atoi(tokens[6].c_str());
            cout<<fx<<" "<<fy<<" "<<fz<<" "<<tx<<" "<<ty<<" "<<tz<<" "<<cap<<endl;
            */
        }
    }
    fs.close();
    return true;
}

bool Database::readBSShapes(const std::string &file)
{
    std::cout << "reading shapes" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    fs.close();
    return true;
}

bool Database::readBSWts(const std::string &file)
{
    std::cout << "reading weights" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    fs.close();
    return true;
}

bool Database::readBSPl(const std::string &file)
{
    std::cout << "reading placement" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens))
    {
        if (tokens[0] == "UCLA")
        {
            continue;
        }
        else if (tokens.size() >= 4)
        {
            map<string, int>::iterator itr = bsData.cellMap.find(tokens[0]);
            if (itr == bsData.cellMap.end())
            {
                assert(false);
                printlog(LOG_ERROR, "cell not found: %s", tokens[0].c_str());
                return false;
            }
            int cell = itr->second;
            double x = atof(tokens[1].c_str());
            double y = atof(tokens[2].c_str());
            bsData.cellX[cell] = (int)round(x);
            bsData.cellY[cell] = (int)round(y);

            if (tokens.size() >= 5)
            {
                if (tokens[4] == "/FIXED" || tokens[4] == "/FIXED_NI")
                {
                    bsData.cellFixed[cell] = (char)1;
                }
            }
        }
    }
    fs.close();
    return true;
}

bool Database::writeBSPl(const std::string &file)
{
    ofstream fs(file.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    fs << "UCLA pl 1.0\n";
    fs << "# User   : Chinese University of Hong Kong\n\n";
    for (auto cell : database.cells)
    {
        fs << '\t' << left << setw(60) << cell->name() << right << setw(8) << cell->lx() / siteW << setw(8)
           << cell->ly() / siteW << " : N";
        if (cell->fixed())
        {
            if (cell->width() / siteW == 1 && cell->height() / siteW == 1)
            {
                fs << " /FIXED_NI";
            }
            else
            {
                fs << " /FIXED";
            }
        }
        fs << endl;
    }
    fs.close();
    return true;
}

// ICCAD2022 read

bool Database::readICCAD2022_setup(const std::string &FILE, int times, bool place_load, bool TOP, bool ANS)
{
    if (!times)
    {
        printlog(LOG_INFO, "reading iccad2022.");
        readICCAD2022(FILE, place_load, TOP, ANS); // this auxfile should be input txt
        // process top die first
        if (place_load)
            return true;
        if (TOP || !ANS)
            bsData.load_Die(top);
        else
            bsData.load_Die(bottom);
    }
    else if (times == 1)
    {
        if (place_load)
            return true;
        if (TOP || !ANS)
            bsData.load_Die(bottom);
        else
            bsData.load_Die(top);
    }
    bsData.estimateSiteSize();
    if (bsData.siteWidth < 10)
    {
        _scale = 100;
        // database.scale = 100;
    }
    else if (bsData.siteWidth < 100)
    {
        _scale = 10;
        // database.scale = 10;
    }
    else
    {
        // database.scale = 1;
    }
    bsData.scaleData(_scale);
    database.clear();
    database.dieLX = INT_MAX;
    database.dieLY = INT_MAX;
    database.dieHX = INT_MIN;
    database.dieHY = INT_MIN;

    for (int i = 0; i < bsData.nRows; i++)
    {
        Row *row = database.addRow("core_SITE_ROW_" + to_string(i), "core", bsData.rowX[i], bsData.rowY[i]);
        row->xStep(bsData.siteWidth);
        row->yStep(bsData.siteHeight);
        row->xNum(bsData.rowSites[i]);
        row->yNum(1);
        row->flip((i % 2) == 1);
        database.dieLX = std::min(database.dieLX, row->x());
        database.dieLY = std::min(database.dieLY, row->y());
        database.dieHX = std::max(database.dieHX, row->x() + (int)row->width());
        database.dieHY = std::max(database.dieHY, row->y() + bsData.siteHeight);
    }
    int defaultPitch = bsData.siteWidth;
    int defaultWidth = bsData.siteWidth / 2;
    int defaultSpace = bsData.siteWidth - defaultWidth;
    unsigned nLayers = 9;
    for (unsigned i = 0; i != nLayers; ++i)
    {
        Layer &layer = database.addLayer(string("M").append(to_string(i + 1)), 'r');
        layer.direction = 'h';
        layer.pitch = defaultPitch;
        layer.offset = defaultSpace;
        layer.width = defaultWidth;
        layer.spacing = defaultSpace;
        if (!i)
        {
            layer.track.direction = 'x';
        }
        else if (i % 2)
        {
            layer.track.direction = 'v';
        }
        else
        {
            layer.track.direction = 'h';
        }
        if (i % 2)
        {
            layer.track.start = database.dieLX + (layer.pitch / 2);
            layer.track.num = (database.dieHX - database.dieLX) / layer.pitch;
        }
        else
        {
            layer.track.start = database.dieLY + (layer.pitch / 2);
            layer.track.num = (database.dieHY - database.dieLY) / layer.pitch;
        }
        if (layer.track.num <= 0)
            layer.track.num = 1;
        layer.track.step = layer.pitch;

        if (i + 1 == nLayers)
        {
            break;
        }
        else
        {
            database.addLayer(string("N").append(to_string(i + 1)), 'c');
        }
    }

    const Layer &layer = database.layers[0];

    for (int i = 0; i < bsData.nTypes; i++)
    {
        unsigned libcell = rtimer.lLib.addCell(bsData.typeName[i]);
        CellType *celltype = database.addCellType(bsData.typeName[i], libcell);
        celltype->width = bsData.typeWidth[i];
        celltype->height = bsData.typeHeight[i];
        celltype->stdcell = (!bsData.typeFixed[i]);
        for (int j = 0; j < bsData.typeNPins[i]; ++j)
        {
            char direction = 'x';
            switch (bsData.typePinDir[i][j])
            {
            case 'I':
                direction = 'i';
                break;
            case 'O':
                direction = 'o';
                break;
            default:
                // printlog(LOG_ERROR, "unknown pin direction: %c", bsData.typePinDir[i][j]);
                break;
            }
            PinType *pintype = celltype->addPin(bsData.typePinName[i][j], direction, 's');
            pintype->addShape(layer, bsData.typePinX[i][j], bsData.typePinY[i][j]);
        }
    }
    for (int i = 0; i < bsData.nCells; i++)
    {
        int typeID = bsData.cellType[i];
        if (typeID < 0)
        {
            IOPin *iopin = database.addIOPin(bsData.cellName[i]);
            switch (typeID)
            {
            case -1:
                iopin->type->direction('o');
                break;
            case -2:
                iopin->type->direction('i');
                break;
            default:
                iopin->type->direction('x');
                break;
            }
            iopin->type->addShape(layer, bsData.cellX[i], bsData.cellY[i]);
        }
        else
        {
            string celltypename(bsData.typeName[typeID]);
            Cell *cell = database.addCell(bsData.cellName[i], database.getCellType(celltypename));
            cell->place(bsData.cellX[i], bsData.cellY[i], false, false);
            cell->fixed((bsData.cellFixed[i] == (char)1));
        }
    }

    for (unsigned i = 0; i != bsData.nNets; ++i)
    {
        Net *net = database.addNet(bsData.netName[i]);
        for (unsigned j = 0; j != bsData.netCells[i].size(); ++j)
        {
            Pin *pin = nullptr;
            int cellID = bsData.netCells[i][j];
            if (bsData.cellType[cellID] < 0)
            {
                IOPin *iopin = database.getIOPin(bsData.cellName[cellID]);
                pin = iopin->pin;
            }
            else
            {
                Cell *cell = database.getCell(bsData.cellName[cellID]);
                pin = cell->pin(bsData.netPins[i][j]);
            }
            pin->net = net;
            net->addPin(pin);
        }
    }

    bsData.clearData();
    return true;
}
bool Database::readICCAD2022(const std::string &in_filename, bool place_load, bool TOP, bool ANS)
{
    ifstream fs(in_filename.c_str());
    if (!fs.good())
    {
        printlog(LOG_ERROR, "cannot open file: %s", in_filename.c_str());
        return false;
    }
    vector<string> tokens;
    Tech *now_tech = NULL;
    while (readBSLine(fs, tokens))
    {
        check_input(tokens);
        if (tokens[0] == "NumTechnologies")
        {
            // num_tech = atoi(tokens[1].c_str());
            continue;
        }
        else if (tokens[0] == "Tech")
        {
            if (tokens[1] == "TA")
            {
                now_tech = &tech_A;
            }
            else if (tokens[1] == "TB")
            {
                now_tech = &tech_B;
            }
            // nNodes = atoi(tokens[2].c_str());
            continue;
        }
        else if (tokens[0] == "LibCell")
        {
            string cType = tokens[1];
            int cWidth = atoi(tokens[2].c_str());
            int cHeight = atoi(tokens[3].c_str());
            bool cFixed = false;
            int pinnum = atoi(tokens[4].c_str());
            int typeID = -1;
            if ((*now_tech).typeMap.find(cType) == (*now_tech).typeMap.end())
            {
                typeID = (*now_tech).nTypes++;
                (*now_tech).typeMap[cType] = typeID;
                (*now_tech).typeName.push_back(cType);
                (*now_tech).typeWidth.push_back(cWidth);
                (*now_tech).typeHeight.push_back(cHeight);
                (*now_tech).typeFixed.push_back((char)(cFixed ? 1 : 0));
                (*now_tech).typeNPins.push_back(0);
                (*now_tech).typePinName.push_back(vector<string>());
                (*now_tech).typePinDir.push_back(vector<char>());
                (*now_tech).typePinX.push_back(vector<double>());
                (*now_tech).typePinY.push_back(vector<double>());
                //  cout << cType << '\t' << tokens.size() << endl;
            }
            else
            {
                typeID = (*now_tech).typeMap[cType];
                assert(cWidth == (*now_tech).typeWidth[typeID]);
                assert(cHeight == (*now_tech).typeHeight[typeID]);
            }
            for (int i = 0; i != pinnum; i++)
            {
                readBSLine(fs, tokens);
                // cout<<"pinnum:"<<pinnum<<endl;
                check_input(tokens);
                int typePinID = -1;
                double pinX = 0;
                double pinY = 0;
                pinX = (double)atof(tokens[2].c_str());
                pinY = (double)atof(tokens[3].c_str());

                string pinName = tokens[1];

                string tpName;
                if (typeID >= 0)
                {
                    tpName.append((*now_tech).typeName[typeID]);
                    tpName.append(":");
                    tpName.append(pinName);
                }
                if ((*now_tech).typePinMap.find(tpName) == (*now_tech).typePinMap.end())
                {
                    typePinID = (*now_tech).typeNPins[typeID]++;
                    (*now_tech).typePinMap[tpName] = typePinID;
                    (*now_tech).typePinName[typeID].push_back(pinName);
                    (*now_tech).typePinDir[typeID].push_back('x');
                    (*now_tech).typePinX[typeID].push_back(pinX);
                    (*now_tech).typePinY[typeID].push_back(pinY);
                }
                else
                {
                    typePinID = (*now_tech).typePinMap[tpName];
                    assert((*now_tech).typePinX[typeID][typePinID] == pinX);
                    assert((*now_tech).typePinY[typeID][typePinID] == pinY);
                    assert((*now_tech).typePinDir[typeID][typePinID] == 'x');
                }
            }
        }
        else if (tokens[0] == "NumInstances")
        {
            global_inst.cell_inst_num = atoi(tokens[1].c_str());
            global_inst.cName.resize(global_inst.cell_inst_num);
            global_inst.type_name.resize(global_inst.cell_inst_num);
            global_inst.cell_inst_die.resize(global_inst.cell_inst_num, NULL);
            global_inst.cell_inst_size.resize(global_inst.cell_inst_num);
            for (int i = 0; i != global_inst.cell_inst_num; i++)
            {
                readBSLine(fs, tokens);
                check_input(tokens);
                global_inst.cName[i] = tokens[1];
                global_inst.type_name[i] = tokens[2];
                global_inst.cell_inst_map[tokens[1]] = i;
            }
        }
        else if (tokens[0] == "DieSize")
        {
            top.lowleftX = bottom.lowleftX = atoi(tokens[1].c_str());
            top.lowleftY = bottom.lowleftY = atoi(tokens[2].c_str());
            top.toprightX = bottom.toprightX = atoi(tokens[3].c_str());
            top.toprightY = bottom.toprightY = atoi(tokens[4].c_str());
        }
        else if (tokens[0] == "TopDieMaxUtil")
        {
            top.MaxUtil = (double)atof(tokens[1].c_str());
        }
        else if (tokens[0] == "BottomDieMaxUtil")
        {
            bottom.MaxUtil = (double)atof(tokens[1].c_str());
        }
        else if (tokens[0] == "TopDieRows") // use bsData1 to stor top and bsData2 to store bottom
        {
            int numRows = atoi(tokens[5].c_str());
            top.rowX.resize(numRows);
            top.rowY.resize(numRows);
            top.rowSites.resize(numRows);
            int Y = atoi(tokens[2].c_str());
            int X = atoi(tokens[1].c_str());
            int stepX = atoi(tokens[3].c_str());
            int stepY = top.rowheight = atoi(tokens[4].c_str());
            for (int i = 0; i != numRows; i++)
            {
                top.rowY[top.nRows] = Y;
                top.rowX[top.nRows] = X;
                top.rowSites[top.nRows] = stepX;
                Y += stepY;
                top.nRows++;
            }
        }
        else if (tokens[0] == "BottomDieRows")
        {
            int numRows = atoi(tokens[5].c_str());
            bottom.rowX.resize(numRows);
            bottom.rowY.resize(numRows);
            bottom.rowSites.resize(numRows);
            int Y = atoi(tokens[2].c_str());
            int X = atoi(tokens[1].c_str());
            int stepX = atoi(tokens[3].c_str());
            int stepY = bottom.rowheight = atoi(tokens[4].c_str());
            for (int i = 0; i != numRows; i++)
            {
                bottom.rowY[bottom.nRows] = Y;
                bottom.rowX[bottom.nRows] = X;
                bottom.rowSites[bottom.nRows] = stepX;
                Y += stepY;
                bottom.nRows++;
            }
        }
        else if (tokens[0] == "TopDieTech")
        {
            top.tech = (tokens[1] == "TA") ? &tech_A : &tech_B;
        }
        else if (tokens[0] == "BottomDieTech")
        {
            bottom.tech = (tokens[1] == "TA") ? &tech_A : &tech_B;
            if (bottom.tech == top.tech)
            { // avoid same address
                if (top.tech == &tech_A)
                {
                    bottom.tech = &tech_B;
                    tech_B = tech_A;
                }
                else
                {
                    bottom.tech = &tech_A;
                    tech_A = tech_B;
                }
            }
        }
        else if (tokens[0] == "TerminalSize")
        {
            top.Terminal_w = bottom.Terminal_w = atoi(tokens[1].c_str());
            top.Terminal_h = bottom.Terminal_h = atoi(tokens[2].c_str());
        }
        else if (tokens[0] == "TerminalSpacing")
        {
            top.Terminal_spacing = bottom.Terminal_spacing = atoi(tokens[1].c_str());
        }
        else if (tokens[0] == "NumNets")
        {
            global_inst.net_num = atoi(tokens[1].c_str());
            global_inst.net_name.resize(global_inst.net_num);
            global_inst.net_cell_name.resize(global_inst.net_num);
            global_inst.net_pin_name.resize(global_inst.net_num);
            global_inst.net_pin_num.resize(global_inst.net_num);
            for (int i = 0; i != global_inst.net_num; i++)
            {
                readBSLine(fs, tokens);
                check_input(tokens);
                global_inst.net_name[i] = tokens[1];
                global_inst.net_pin_num[i] = atoi(tokens[2].c_str());
                global_inst.net_cell_name[i].resize(global_inst.net_pin_num[i]);
                global_inst.net_pin_name[i].resize(global_inst.net_pin_num[i]);
                for (int j = 0; j != global_inst.net_pin_num[i]; j++)
                {
                    readBSLine(fs, tokens);
                    check_input(tokens);
                    global_inst.net_cell_name[i][j] = tokens[1];
                    global_inst.net_pin_name[i][j] = tokens[2];
                }
            }
        }
    }
    // TODO
    // setting up FM related
    tech_A.setup_areas();
    tech_B.setup_areas();
    global_inst.setup_cell_net_map();
    // hmetis
    if (place_load == false && ANS == false)
    {
        global_inst.f.partition();
    }
    else
    {
        global_inst.load_Place_result(ANS, "top.txt");
    }
    global_inst.f.output_to_global(global_inst);
    printlog(LOG_INFO, "Partition over.");
    // allocate cell instance
    for (int i = 0; i < global_inst.cell_inst_num; i++)
    {
        int cell_encoding = i;
        global_inst.cell_inst_die[cell_encoding]->cell_num++;
        (now_tech) = global_inst.cell_inst_die[cell_encoding]->tech;
        int typeID = (*now_tech).typeMap[global_inst.type_name[cell_encoding]];
        int cellID = (*now_tech).nCells++;
        if ((*now_tech).cellMap.find(global_inst.cName[cell_encoding]) != (*now_tech).cellMap.end())
        {
            printlog(LOG_ERROR, "Repeatedly Allocate Instance: %s ", global_inst.cName[cell_encoding]);
        }
        (*now_tech).cellMap[global_inst.cName[cell_encoding]] = cellID;
        (*now_tech).cellType.push_back(typeID);
        (*now_tech).cellFixed.push_back((char)((*now_tech).typeFixed[typeID] ? 1 : 0));
        (*now_tech).cellName.push_back(global_inst.cName[cell_encoding]);
        (*now_tech).cellX.push_back(0);
        (*now_tech).cellY.push_back(0);
    }
    // allocate net
    for (int i = 0; i < global_inst.net_num; i++)
    {
        bool net_al = false;
        bool net_bl = false;
        for (int j = 0; j < global_inst.net_pin_num[i]; j++)
        {
            int cell_encoding = global_inst.cell_inst_map[global_inst.net_cell_name[i][j]]; // instance encoding
            (now_tech) = global_inst.cell_inst_die[cell_encoding]->tech;
            // Net
            string nName = global_inst.net_name[i];
            int netID = 0;
            if (net_al && (now_tech) == &tech_A)
            {
                netID = (*now_tech).nNets - 1;
            }
            else if ((now_tech) == &tech_A)
            {
                netID = (*now_tech).nNets++;
                (*now_tech).netName.push_back(nName);
                (*now_tech).netCells.push_back(*new vector<int>);
                (*now_tech).netPins.push_back(*new vector<int>);
                net_al = true;
            }
            if (net_bl && (now_tech) == &tech_B)
            {
                netID = (*now_tech).nNets - 1;
            }
            else if ((now_tech) == &tech_B)
            {
                netID = (*now_tech).nNets++;
                (*now_tech).netName.push_back(nName);
                (*now_tech).netCells.push_back(*new vector<int>);
                (*now_tech).netPins.push_back(*new vector<int>);
                net_bl = true;
            }
            string cName = global_inst.net_cell_name[i][j];
            string pinName = global_inst.net_pin_name[i][j];
            if ((*now_tech).cellMap.find(cName) == (*now_tech).cellMap.end())
            {
                assert(false);
                printlog(LOG_ERROR, "cell not found in data1: %s", cName.c_str());
                return -1;
            }
            int cellID = (*now_tech).cellMap[cName];

            int typeID = (*now_tech).cellType[cellID];
            int typePinID = -1;
            string tpName;
            tpName.append((*now_tech).typeName[typeID]);
            tpName.append(":");
            tpName.append(pinName);
            if ((*now_tech).typePinMap.find(tpName) == (*now_tech).typePinMap.end())
            {
                assert(false);
                printlog(LOG_ERROR, "cell_pin not found in data1: %s", tpName);
                return -1;
            }
            typePinID = (*now_tech).typePinMap[tpName];
            (*now_tech).netCells[netID].push_back(cellID);
            (*now_tech).netPins[netID].push_back(typePinID);
        }
    }

    fs.close();
    return true;
}
// ICCAD2022 write

bool Database::writeICCAD2022(const std::string &file, int load_num, bool place_load, bool TOP, bool ANS)
{
    // load_num==1-->top die complete
    // load_num==2-->bottom die complete
    if (TOP == false && ANS == false)
    {
        if (load_num == 1 && place_load == false)
        {
            checkPlaceError();
            printlog(LOG_INFO, "--------------Load the result of top die--------------");
            for (auto cell : database.cells)
            {
                if (global_inst.cell_xy.find(cell->name()) != global_inst.cell_xy.end())
                {
                    cerr << "Repeat load placement result of " << cell->name() << endl;
                    return false;
                }
                else
                {
                    global_inst.cell_xy[cell->name()] = make_pair(cell->lx() / _scale, cell->ly() / _scale);
                }
            }
        }
        else if (load_num == 2 || place_load == true)
        {
            if (place_load == false)
            {
                checkPlaceError();
                printlog(LOG_INFO, "--------------Load the result of bottom die--------------");
                for (auto cell : database.cells)
                {
                    if (global_inst.cell_xy.find(cell->name()) != global_inst.cell_xy.end())
                    {
                        cerr << "Repeat load placement result of " << cell->name() << endl;
                    }
                    else
                    {
                        global_inst.cell_xy[cell->name()] = make_pair(cell->lx() / _scale, cell->ly() / _scale);
                    }
                }
            }
            printlog(LOG_INFO, "Loading is over");
            if (place_load == false)
                global_inst.write_Place_result(TOP);
            // terminal
            if (!global_inst.terminal_insert())
            {
                printlog(LOG_ERROR, "Terminal Inersion fails");
            }
            printlog(LOG_INFO, "Terminal Insersion is over");
            // Analyze for HPWL
            printlog(LOG_INFO, "Total HPWL for 2 dies: %lld", global_inst.getCost());
            // output
            global_inst.layout_info();
            if (!global_inst.writeICCAD2022(file))
            {
                printlog(LOG_ERROR, "Output to ICCAD format fails");
            }
            else
                printlog(LOG_INFO, "Output to %s", file.c_str());
        }
    }
    else
    {
        if (TOP)
        {

            checkPlaceError();
            printlog(LOG_INFO, "--------------Load the result of top die--------------");
            for (auto cell : database.cells)
            {
                if (global_inst.cell_xy.find(cell->name()) != global_inst.cell_xy.end())
                {
                    cerr << "Repeat load placement result of " << cell->name() << endl;
                    return false;
                }
                else
                {
                    global_inst.cell_xy[cell->name()] = make_pair(cell->lx() / _scale, cell->ly() / _scale);
                }
            }
            global_inst.write_Place_result(TOP, "top.txt");
        }
        if (ANS)
        {
            checkPlaceError();
            printlog(LOG_INFO, "--------------Load the result of top die--------------");
            for (auto cell : database.cells)
            {
                if (global_inst.cell_xy.find(cell->name()) != global_inst.cell_xy.end())
                {
                    cerr << "Repeat load placement result of " << cell->name() << endl;
                }
                else
                {
                    global_inst.cell_xy[cell->name()] = make_pair(cell->lx() / _scale, cell->ly() / _scale);
                }
            }
            if (place_load == false)
                global_inst.write_Place_result(TOP);
            if (!global_inst.terminal_insert())
            {
                printlog(LOG_ERROR, "Terminal Inersion fails");
            }
            printlog(LOG_INFO, "Terminal Insersion is over");
            // Analyze for HPWL
            printlog(LOG_INFO, "Total HPWL for 2 dies: %lld", global_inst.getCost());
            // output
            global_inst.layout_info();
            if (!global_inst.writeICCAD2022(file))
            {
                printlog(LOG_ERROR, "Output to ICCAD format fails");
            }
            else
                printlog(LOG_INFO, "Output to %s", file.c_str());
        }
    }
    return true;
}