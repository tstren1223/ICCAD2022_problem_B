#include "data.h"
Data database;

void Data :: separate()
{
    ofstream file("top_cells.txt");
    file<<top_nodes.size()<<endl;
    for(int i=0; i<top_nodes.size(); i++)
    {
        file<<instances_infro[top_nodes[i]].width[0]<<" "<<instances_infro[top_nodes[i]].height[0]
        <<" "<<instances[top_nodes[i]].type<<endl;
    }
    file.close();

    file.open("bot_cells.txt");
    file<<bottom_nodes.size()<<endl;
    for(int i=0; i<bottom_nodes.size(); i++)
    {
        file<<instances_infro[bottom_nodes[i]].width[1]<<" "<<instances_infro[bottom_nodes[i]].height[1]
        <<" "<<instances[bottom_nodes[i]].type<<endl;
    }
    file.close();

    file.open("top_row.txt");
    file<<dies_infro[0].rowN<<endl;
    file<<dies_infro[0].width<<endl;
    file<<dies_infro[0].rowHeight<<endl;
    file.close();

    file.open("bot_row.txt");
    file<<dies_infro[1].rowN<<endl;
    file<<dies_infro[1].width<<endl;
    file<<dies_infro[1].rowHeight<<endl;
    file.close();

    vector<Net> nets_top;
    vector<Net> nets_bot;
    unordered_map<int,int> top_node_idx;
    unordered_map<int,int> bot_node_idx;
    int idx = 0;
    for(auto i : top_nodes)    
        top_node_idx[i] = idx++;
    idx = 0;
    for(auto i : bottom_nodes)    
        bot_node_idx[i] = idx++;
    
    for(auto &it : nets)
    {
        Net top;
        Net bot;
        top.pins.clear();
        bot.pins.clear();
        for(auto &it2 : it.pins)
        {
            if(instances[it2.first].die==false)
                top.pins.push_back(make_pair(it2.first,it2.second));
            else
                bot.pins.push_back(make_pair(it2.first,it2.second));            
        }
        if(top.pins.size()>0)
        {
            top.pin_num = top.pins.size();
            nets_top.push_back(top);
        }
        if(bot.pins.size()>0)
        {
            bot.pin_num = bot.pins.size();
            nets_bot.push_back(bot);
        }
    }

    file.open("top_nets.txt");
    file<<nets_top.size()<<endl;
    for(int i=0;i<nets_top.size();i++)
    {
        file<<nets_top[i].pin_num<<endl;
        for(auto &it : nets_top[i].pins)
        {
            int cell = it.first;
            int pin = it.second;
            file<<top_node_idx[cell]<<" "<<pin<<" "<<instances_infro[cell].pins[0][pin].first<<" "<<instances_infro[cell].pins[0][pin].second<<endl;
        }
    }
    file.close();

    file.open("bot_nets.txt");
    file<<nets_bot.size()<<endl;
    for(int i=0;i<nets_bot.size();i++)
    {
        file<<nets_bot[i].pin_num<<endl;
        for(auto &it : nets_bot[i].pins)
        {
            int cell = it.first;
            int pin = it.second;
            file<<bot_node_idx[cell]<<" "<<pin<<" "<<instances_infro[cell].pins[1][pin].first<<" "<<instances_infro[cell].pins[1][pin].second<<endl;
        }
    }
    file.close();

}

void Data :: update()
{
    ifstream in_file("top_out.txt");
    stringstream ss;
    ss << in_file.rdbuf();
    for(int i=0;i<top_nodes.size();i++)
    {
        ss>>instances[top_nodes[i]].x;
        ss>>instances[top_nodes[i]].y;
    }
    in_file.close();
    ss.clear();
    in_file.open("bot_out.txt");
    ss << in_file.rdbuf();
    for(int i=0;i<bottom_nodes.size();i++)
    {
        ss>>instances[bottom_nodes[i]].x;
        ss>>instances[bottom_nodes[i]].y;
    }
    in_file.close();
    ss.clear();
}

void Data :: read(string in,string out)
{
    in_filename = in;
    out_filename = out;    
    ifstream in_file(in_filename);
    try{
        stringstream ss;
        ss<<in_file.rdbuf();
        string temp;
        int tech_num,cell_num,pin_num;

        ss>>temp;
        ss>>tech_num;
        techs.resize(tech_num);
        for(int i=0;i<tech_num;i++)
        {         
            ss>>temp;
            ss>>temp;
            ss>>cell_num;
            for(int j=0;j<cell_num;j++)
            {
                string cell_name;
                ss>>temp;
                ss>>cell_name;
                ss>>techs[i].cells_map[cell_name].width;
                ss>>techs[i].cells_map[cell_name].height;
                techs[i].cells_map[cell_name].area = techs[i].cells_map[cell_name].width * techs[i].cells_map[cell_name].height;
                ss>>pin_num;
                techs[i].cells_map[cell_name].pins.resize(pin_num);
                for(int k=0;k<pin_num;k++){
                    int x,y;
                    ss>>temp;
                    ss>>temp;
                    ss>>x;
                    ss>>y;
                    techs[i].cells_map[cell_name].pins[k] = make_pair(x,y);
                }
            }
        }        
        
        
        ss>>temp;        
        ss>>temp;
        ss>>temp;
        ss>>dies_infro[0].height;
        ss>>dies_infro[0].width;
        dies_infro[1].height = dies_infro[0].height;
        dies_infro[1].width = dies_infro[0].width;
        
        ss>>temp;
        ss>>dies_infro[0].MaxUtil;
        ss>>temp;
        ss>>dies_infro[1].MaxUtil;

        for(int i=0;i<4;i++)  // skip useless message
            ss>>temp;
        ss>>dies_infro[0].rowHeight;
        ss>>dies_infro[0].rowN;        
        for(int i=0;i<4;i++) // skip useless message
            ss>>temp;
        ss>>dies_infro[1].rowHeight;
        ss>>dies_infro[1].rowN;
        
        for(int i=0;i<2;i++)
            dies_infro[i].area = (long long)dies_infro[i].width * (long long)dies_infro[i].height * dies_infro[i].MaxUtil / 100; 
        
        which_techs.resize(2);
        ss>>temp;
        ss>>temp;
        if(temp=="TA")
            which_techs[0] = 0;
        else
            which_techs[0] = 1;
        ss>>temp;
        ss>>temp;
        if(temp=="TA")
            which_techs[1] = 0;
        else
            which_techs[1] = 1;
        
        ss>>temp;  
        ss>>terminal_infro.width;
        ss>>terminal_infro.height;
        ss>>temp;
        ss>>terminal_infro.spacing;
        ss>>temp;
        ss>>Cells_N;
        instances.resize(Cells_N);
        for(int i=0;i<Cells_N;i++){
            ss>>temp;
            ss>>temp;
            ss>>instances[i].type;
        }
        
        ss>>temp;
        ss>>Nets_N;
        nets.resize(Nets_N);
        for(int i=0;i<Nets_N;i++){
            ss>>temp;
            ss>>temp;
            ss>>nets[i].pin_num;
            nets[i].pins.resize(nets[i].pin_num);
            for(int j=0;j<nets[i].pin_num;j++){
                ss>>temp;
                ss>>temp;
                int pos = temp.find('/');
                int inst_idx = stoi(temp.substr(1,pos)) - 1;  // 0 base
                int pin_idx = stoi(temp.substr(pos+2)) - 1;
                nets[i].pins[j] = make_pair(inst_idx,pin_idx);
                node_net[inst_idx].insert(i);
            }
        }

    }catch(exception &e){        
        cout<<"read data file,exception: "<<e.what()<<"\n";
    }
    Instances_infro_create();
    //calSite();
}

void Data :: Instances_infro_create()
{
    instances_infro.resize(Cells_N);
    for(int i=0;i<Cells_N;i++)
    {
        instances_infro[i].pins.resize(2);
        for(int j=0;j<2;j++)
        {            
            instances_infro[i].area[j] = techs[which_techs[j]].cells_map[instances[i].type].area;
            instances_infro[i].width[j] = techs[which_techs[j]].cells_map[instances[i].type].width;
            instances_infro[i].height[j] = techs[which_techs[j]].cells_map[instances[i].type].height;            
            instances_infro[i].pins[j] = techs[which_techs[j]].cells_map[instances[i].type].pins;
        }                
    }
}


void Data :: testInput()
{
    ofstream out_file("input_test.txt");
    out_file<<"Terminal"<<endl;
    out_file<<"x:"<<terminal_infro.width<<" y:"<<terminal_infro.height<<" spacing:"<<terminal_infro.spacing<<endl;    
    out_file<<"------------------------------"<<endl;
    out_file<<endl;
    out_file<<"Instances infro"<<endl;
    out_file<<"Cells_N"<<Cells_N<<endl;  
    for(int i=0;i<Cells_N;i++)     
    {   
        out_file<<instances[i].type<<endl;
        for(int j=0;j<2;j++)
        {
            if(j==0)
                out_file<<"top:";
            else
                out_file<<"bot";
            out_file<<" width:"<<instances_infro[i].width[j]<<" height:"
            <<instances_infro[i].height[j]<<endl;
            out_file<<"pins"<<endl;
            for(int k=0;k<instances_infro[i].pins[j].size();k++)            
                out_file<<instances_infro[i].pins[j][k].first<<" "<<instances_infro[i].pins[j][k].second<<endl;            
        }
    }
    out_file<<"------------------------------"<<endl;
    out_file<<"Dies infro"<<endl;
    for(int i=0;i<2;i++)
    {
        string temp = (i==0)? "top" : "bottom";
        out_file<<"width  height uti  tech area"<<endl;
        out_file<<dies_infro[i].width<<" "<<dies_infro[i].height<<" "<<dies_infro[i].MaxUtil<<" "<<dies_infro[i].area<<endl;        
    }
    out_file<<"-----------------------------"<<endl;
    out_file<<"Nets_N"<<Nets_N<<endl;
    for(int i=0;i<Nets_N;i++)
    {
        for(auto &it2 : nets[i].pins)        
            out_file<<"cell"<<it2.first<<"/pin"<<it2.second<<endl;        
    }
}
//---------------------------------------------------------------------------
//   Auxilliary Functions
//---------------------------------------------------------------------------

bool rand_bool(){
    if(rand()%2==1)
        return true;
    else
        return false;
}

double rand_01(){
  return double(rand()%10000)/10000; 
}