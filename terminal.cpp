#include "terminal.h"

void AddTerminal:: terminal_insert()
{
    terminals_grid.clear();
    grid_row = (database.dies_infro[0].height - database.terminal_infro.spacing*2) / database.terminal_infro.height;
    grid_col = (database.dies_infro[0].width - database.terminal_infro.spacing*2) / database.terminal_infro.width;
    terminals_grid.resize(grid_row,vector<int>(grid_col,0));
    for(auto &it : database.crossNets_pair)
        terminal_BFS(it.first,it.second);
}
void AddTerminal :: findCenter(int net_idx,int cross_type,int &row,int &col)
{
    int top_lowleft[2],top_topright[2],bot_lowleft[2],bot_topright[2];
    int centerX,centerY;
    for(int i=0;i<2;i++)  
    {
        top_lowleft[i] = INT_MAX;
        bot_lowleft[i] = INT_MAX;
        top_topright[i] = INT_MIN;
        bot_topright[i] = INT_MIN;
    }
    int pin_x,pin_y;    
    for(auto &it : database.nets[net_idx].pins)
    {        
        if(database.instances[it.first].die==false) // top
        {
            pin_x = database.instances[it.first].x + database.instances_infro[it.first].pins[0][it.second].first;
            pin_y = database.instances[it.first].y + database.instances_infro[it.first].pins[0][it.second].second;
            top_lowleft[0] = min(pin_x,top_lowleft[0]);
            top_lowleft[1] = min(pin_y,top_lowleft[1]);
            top_topright[0] = max(pin_x,top_topright[0]);
            top_topright[1] = max(pin_y,top_topright[1]); 
        }
        else
        {
            pin_x = database.instances[it.first].x + database.instances_infro[it.first].pins[1][it.second].first;
            pin_y = database.instances[it.first].y + database.instances_infro[it.first].pins[1][it.second].second;                
            bot_lowleft[0] = min(pin_x,bot_lowleft[0]);
            bot_lowleft[1] = min(pin_y,bot_lowleft[1]);
            bot_topright[0] = max(pin_x,bot_topright[0]);
            bot_topright[1] = max(pin_y,bot_topright[1]); 
        }                                                
    } 
    if(cross_type==0)  
    {                        
        int minX2 = min(top_topright[0],bot_topright[0]);
        int maxX1 = max(top_lowleft[0],bot_lowleft[0]);
        int minY2 = min(top_topright[1],bot_topright[1]);
        int maxY1 = max(top_lowleft[1],bot_lowleft[1]);
        centerX = (minX2 + maxX1)/2;
        centerY = (minY2 + maxY1)/2;
    }
    else if(cross_type==1) // bot = 1
    {
        // deal X
        if(bot_lowleft[0]<=top_topright[0] && bot_lowleft[0]>=top_lowleft[0])
            centerX = bot_lowleft[0];
        else if(bot_lowleft[0]>top_topright[0])
            centerX = (bot_lowleft[0]+top_topright[0])/2;
        else 
            centerX = (bot_lowleft[0]+top_lowleft[0])/2;
        // deal Y
        if(bot_lowleft[1]<=top_topright[1] && bot_lowleft[1]>=top_lowleft[1])
            centerY = bot_lowleft[1];
        else if(bot_lowleft[1]>top_topright[1])
            centerY = (bot_lowleft[1]+top_topright[1])/2;
        else 
            centerY = (bot_lowleft[1]+top_lowleft[1])/2;    
    }
    else if(cross_type==2)
    {
        // deal X
        if(top_lowleft[0]<=bot_topright[0] && top_lowleft[0]>=bot_lowleft[0])
            centerX = top_lowleft[0];
        else if(top_lowleft[0]>bot_topright[0])
            centerX = (top_lowleft[0]+bot_topright[0])/2;
        else 
            centerX = (top_lowleft[0]+bot_lowleft[0])/2;
        // deal Y
        if(top_lowleft[1]<=bot_topright[1] && top_lowleft[1]>=bot_lowleft[1])
            centerY = top_lowleft[1];
        else if(top_lowleft[1]>bot_topright[1])
            centerY = (top_lowleft[1]+bot_topright[1])/2;
        else 
            centerY = (top_lowleft[1]+bot_lowleft[1])/2; 
    }
    else
    {
        centerX = (top_lowleft[0]+bot_topright[0])/2;
        centerY = (top_lowleft[1]+bot_lowleft[1])/2; 
    }    
    row = centerY/database.terminal_infro.height,col = centerX/database.terminal_infro.width; 
    if(row>=grid_row)    
        row = grid_row-1;
    else if(col>=grid_col)
        col = grid_col-1;
}
void AddTerminal :: terminal_BFS(int net_idx,int cross_type)
{
    int row,col;
    findCenter(net_idx,cross_type,row,col);    
    set<pair<int,int>> visited;
    queue<pair<int,int>> q_pair;    
    q_pair.push(make_pair(row,col));
    while(!q_pair.empty())
    {
        row = q_pair.front().first;
        col = q_pair.front().second;
        q_pair.pop();
        if(terminals_grid[row][col]==0)
        {
            terminals_grid[row][col] = net_idx;
            terminals_map[net_idx] = make_pair(row,col);            
            for(int i=0;i<8;i++)
                if(row+area_vec[i][0]>=0 && row+area_vec[i][0]<=grid_row-1&& col+area_vec[i][1]>=0 && col+area_vec[i][1]<=grid_col-1) 
                    terminals_grid[row+area_vec[i][0]][col+area_vec[i][1]] = net_idx;
            return;
        }
        else
        {            
            visited.insert(make_pair(row,col));
            for(int i=0;i<4;i++)
            {
                int next_row = row+dir_vec[i],next_col = col+dir_vec[i+1];
                if(next_row>=0 && next_row<=grid_row-1&& next_col>=0 && next_col<=grid_col-1 && !visited.count(make_pair(next_row,next_col)))        
                    q_pair.push(make_pair(next_row,next_col));                            
            }
        }   
    }
}
void AddTerminal :: output()
{
    ofstream out_file(database.out_filename);
    out_file<<"TopDiePlacement "<<database.top_nodes.size()<<endl;
    for(int i=0;i<database.top_nodes.size();i++)               
    {
            out_file<<"Inst "<<"C"<<database.top_nodes[i]+1<<" "<<database.instances[database.top_nodes[i]].x
            <<" "<<database.instances[database.top_nodes[i]].y<<endl;  
    }

    out_file<<"BottomDiePlacement "<<database.bottom_nodes.size()<<endl;
    for(int i=0;i<database.bottom_nodes.size();i++)               
    {
            out_file<<"Inst "<<"C"<<database.bottom_nodes[i]+1<<" "<<database.instances[database.bottom_nodes[i]].x
            <<" "<<database.instances[database.bottom_nodes[i]].y<<endl;  
    }
    out_file<<"NumTerminals "<<terminals_map.size()<<endl;
    for(auto &it : terminals_map)
    {                
        int x = terminals_map[it.first].second*database.terminal_infro.width + database.terminal_infro.width*0.5+database.terminal_infro.spacing;
        int y = terminals_map[it.first].first*database.terminal_infro.height + database.terminal_infro.height*0.5+database.terminal_infro.spacing;
        out_file<<"Terminal "<<"N"<<it.first+1<<" "<<x<<" "<<y<<endl;
    }        
}

