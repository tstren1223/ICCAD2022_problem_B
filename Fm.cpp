#include "Fm.h"
void FM :: partition()
{                      
    net();    
    system("chmod 755 shmetis");
    system("./shmetis net.hgr 2 5");
    read_net();    
    area_adjust();
    ans();        
}
void FM::reload(){
    read_net();    
    area_adjust();
    ans();
}
int FM :: area_constraint()
{
    if(total_area[1] > database.dies_infro[1].area)
        return 2;  // bottom invalid
    else if(total_area[0] > database.dies_infro[0].area)
        return 1;  // top invalid
    else
        return 0; // both valid
}

void FM :: ans()
{
    for(int i=0;i<database.Nets_N;i++)
    {
        if(nets_infro[i].size[0]>1 && nets_infro[i].size[1]>1)        
            database.crossNets_pair[i] = 0;
        else if(nets_infro[i].size[0]>1 && nets_infro[i].size[1]==1)        
            database.crossNets_pair[i] = 1;
        else if(nets_infro[i].size[0]==1 && nets_infro[i].size[1]>1)        
            database.crossNets_pair[i] = 2;
        else if(nets_infro[i].size[0]==1 && nets_infro[i].size[1]==1)      
            database.crossNets_pair[i] = 3;
        else
            continue;
    }
    if(CrossNets_N!=database.crossNets_pair.size())
    {
        cout<<"FM wrong!"<<endl;
        exit(-1);
    }
    // cells   
    for(int i=0;i<database.Cells_N;i++)    
    {
        database.instances[i].die = cells[i].part;            
        if(cells[i].part)
            database.bottom_nodes.push_back(i);
        else
            database.top_nodes.push_back(i);        
    }
    database.dies_infro[0].cells_num = database.top_nodes.size();
    database.dies_infro[1].cells_num = database.bottom_nodes.size();
}
void FM :: testPartition()
{
    ofstream out_file("testPartition.txt");
    out_file<<"w0:"<<total_area[0]<<" w1:"<<total_area[1]<<endl;
    int idx = 0;
    for(auto &it : database.instances)    
        out_file<<idx++<<" "<<it.die<<" "<<endl;
}

void  FM :: buckets_create()
{           
    // gain cal 
    for(auto &it : nets_infro)
    {
        if(it.second.size[0]<=1 || it.second.size[1]<=1)
        {
            if(it.second.size[0]==0)  // TE
                for(auto i : it.second.part1)
                    cells[i].gain-=1;
            if(it.second.size[0]==1)  // FS            
                for(auto i : it.second.part0)
                    cells[i].gain+=1;            
            if(it.second.size[1]==0)  // TE            
                for(auto i : it.second.part0)
                    cells[i].gain-=1;            
            if(it.second.size[1]==1)  // FS            
                for(auto i : it.second.part1)
                    cells[i].gain+=1;            
        }
        else
            continue;
    }
    // init buckets
    for(int i=0;i<database.Cells_N;i++)
        gain_map[cells[i].gain].insert(i);       
}
void FM :: area_update(int node)
{
    if(cells[node].part==false)
    {
        total_area[0]-=database.instances_infro[node].area[0];
        total_area[1]+=database.instances_infro[node].area[1];
    }
    else
    {
        total_area[0]+=database.instances_infro[node].area[0];
        total_area[1]-=database.instances_infro[node].area[1];
    }
}
void FM :: gain_update(int node)
{
    unordered_map<int,int> update_gain;
    area_update(node);
    cells[node].lock = true;
    gain_map[cells[node].gain].erase(node);    
    for(auto i : database.node_net[node])   // i: net contains node
    {              
        if(cells[node].part==false) // from top to bottom
        {
            nets_infro[i].part0.erase(node);
            nets_infro[i].part1.insert(node);
            nets_infro[i].size[0]--;
            nets_infro[i].size[1]++;
            if(nets_infro[i].size[0]==1)
                for(auto j : nets_infro[i].part0)
                    if(cells[j].lock==false)
                        update_gain[j]+=1;
            if(nets_infro[i].size[0]==0) 
                for(auto j : nets_infro[i].part1)
                    if(cells[j].lock==false)
                        update_gain[j]-=1;    
            if(nets_infro[i].size[1]==2)   
                for(auto &j : nets_infro[i].part1)
                    if(cells[j].lock==false)
                        update_gain[j]-=1;
            if(nets_infro[i].size[1]==1)   
                for(auto &j : nets_infro[i].part0)
                    if(cells[j].lock==false)
                        update_gain[j]+=1;
        }
        else  
        {
            nets_infro[i].part1.erase(node);
            nets_infro[i].part0.insert(node);
            nets_infro[i].size[0]++;
            nets_infro[i].size[1]--;
            if(nets_infro[i].size[1]==1)
                for(auto j : nets_infro[i].part1)
                    if(cells[j].lock==false)
                        update_gain[j]+=1;
            if(nets_infro[i].size[1]==0) 
                for(auto j : nets_infro[i].part0)
                    if(cells[j].lock==false)
                        update_gain[j]-=1;    
            if(nets_infro[i].size[0]==2)   
                for(auto &j : nets_infro[i].part0)
                    if(cells[j].lock==false)
                        update_gain[j]-=1;
            if(nets_infro[i].size[0]==1)   
                for(auto &j : nets_infro[i].part1)
                    if(cells[j].lock==false)
                        update_gain[j]+=1;
        }          
    }     
    cells[node].part = !cells[node].part;

    for(auto &it : update_gain)
    {
        int change_node = it.first;
        int change_gain = it.second;
        if(change_gain==0) 
            continue;
        else
        {        
            gain_map[cells[change_node].gain].erase(change_node);      
            cells[change_node].gain+=change_gain;
            gain_map[cells[change_node].gain].insert(change_node);
        }
    }
}  

void FM :: area_adjust()
{
    buckets_create();    
    int area_result;
    while((area_result=area_constraint())!=0)
    {
        bool which_die = (area_result==1)? false : true;
        bool found = false;
        for(auto &it : gain_map)        
        {
            for(auto i : it.second)     
            {                   
                if(cells[i].part==which_die)
                {    
                    CrossNets_N-=it.first;   
                    gain_update(i);                                 
                    found = true;
                    break;
                }
            }
            if(found)
                break;
        }
    }    
}

void FM :: net()
{
    ofstream out_file("net.hgr");
    out_file<<database.Nets_N<<" "<<database.Cells_N<<endl;
    unordered_set<int> set;
    for(auto &it : database.nets)
    {
        for(auto &it2 : it.pins)
        {
            if(!set.count(it2.first))
            {
                set.insert(it2.first);
                out_file<<it2.first+1<<" ";
            }
            else
                continue;
        }
        out_file<<endl;
        set.clear();
    }

}

void FM :: read_net()
{
    ifstream in_file("net.hgr.part.2");
    stringstream ss;
    ss<<in_file.rdbuf();
    int die,cut = 0;
    cells.resize(database.Cells_N);
    for(int i=0;i<database.Cells_N;i++)
    {
        ss>>die;
        if(die==0)
        {
            cells[i].part = false;
            total_area[0]+=database.instances_infro[i].area[0];
        }
        else
        {
            cells[i].part = true;
            total_area[1]+=database.instances_infro[i].area[1];
        }        
    }
    unordered_set<int> set;
    for(int i=0;i<database.Nets_N;i++)
    {        
        for(auto &it : database.nets[i].pins)
        {
            if(!set.count(it.first))
            {
                set.insert(it.first);
                if(cells[it.first].part==false)
                {
                    nets_infro[i].size[0]++;
                    nets_infro[i].part0.insert(it.first);
                }
                else
                {
                    nets_infro[i].size[1]++;
                    nets_infro[i].part1.insert(it.first);
                }
            }
            else 
                continue;
        }      
        if(nets_infro[i].size[1]>0 && nets_infro[i].size[0]>0)
            cut++;
        set.clear();
    }
    CrossNets_N = cut;
    cout<<"init crossnets:"<<CrossNets_N<<endl;
}
