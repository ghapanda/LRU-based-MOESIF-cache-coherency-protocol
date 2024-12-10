
#include <iostream>
#include <fstream>   // For file handling
#include <string>    // For std::string
#include <regex>
using namespace std;

#define u32 unsigned int
#define NUM_SETS 4
#define NUM_CORES 4
#define MAX_LRU 3

//stats
u32 cache_hits=0;
u32 cache_misses=0;
u32 write_backs=0;
u32 broadcasts=0;
u32 cache_to_cache_transfers=0;



bool exclusive(u32 tag,int core_id); //check for exclusiveness
class cache_line{
    public:
      cache_line();
      u32 index;
      char coherency_state;
      u32 LRU_state;
      u32 tag;
      bool dirty; 
      int core_id;
      void update_coherency_state(char op, bool miss); 
         

};
cache_line::cache_line(): coherency_state('I'),tag(0),dirty(0),core_id(-1){}






class core{
    public:
    core();
    u32 ID; 
    cache_line cache[NUM_SETS];
    cache_line* LRU[NUM_SETS];
    void update_LRU(int LRU_index);
    void bring_line(u32 tag,char op);
    void update_existing_line(int index, char op);
    void evict(cache_line line);
    int cache_hit(u32 tag);
    void set_lines_core_id();


};

core cores[NUM_CORES];//initialize the cores

core::core(){
 
  for(int i=0;i<NUM_SETS;i++){
    cache_line* line=&cache[i];
    line->index=i;
    line->LRU_state=i;
    LRU[i]=&cache[i];

  }
}

void core::set_lines_core_id(){  //each cache line identifies its parent core by the core_id
    for (int i=0;i<NUM_SETS;i++){
        cache[i].core_id=this->ID;
    }
}
int core::cache_hit(u32 tag){ //check for hit or miss
    for(int i=0;i<NUM_SETS;i++){
        if(cache[i].tag==tag&&cache[i].coherency_state!='I'){

          return i;
        }

    }
    return -1;
}
void core:: evict(cache_line line){ //for evicting the least recently used block
    if(line.dirty){
        write_backs++;
    }
}

void core::update_LRU(int LRU_index){ //LRU circular policy 

    cache_line* temp=LRU[LRU_index];

    for(int i=LRU_index;i<MAX_LRU;i++){
        LRU[i]=LRU[i+1];
        LRU[i]->LRU_state=i;
    }
    LRU[MAX_LRU]=temp;
    temp->LRU_state=MAX_LRU;
}

void core::bring_line(u32 tag,char op){ //for cache misses
    for(int i=0;i<NUM_SETS;i++){   //first try to evict an invalid line
        if(cache[i].coherency_state=='I'){
            cache[i].tag=tag;
            if(op=='w'){
            cache[i].dirty=1;
            }
            else{
            cache[i].dirty=0;
            }

            update_LRU(cache[i].LRU_state);
            
            cache[i].update_coherency_state(op, true); //this also takes care of broadcasting; indicate miss=true
            return;
        }
    }

    evict(*LRU[0]);  //if no invalid line, evic line with LRU=0
    LRU[0]->tag=tag;
    LRU[0]->coherency_state='I';
    if(op=='w'){
        LRU[0]->dirty=1;
    }
    else{
        LRU[0]->dirty=0;
    }
    update_LRU(0);
    LRU[MAX_LRU]->update_coherency_state(op, true);//this also takes care of broadcasting; indicate miss=true

}


 void core::update_existing_line(int index, char op){ //for cache hit

   u32 curr_LRU=cache[index].LRU_state;
   update_LRU(curr_LRU);
   if(op=='w'){
    cache[index].dirty=1;
   }
   cache[index].update_coherency_state(op,false); //this also takes care of broadcasting; indicate miss=false
   
 }
bool exclusive(u32 tag,int core_id){
    for(int i=0;i<NUM_CORES;i++){
        if(i!=core_id){
        int hit_line=cores[i].cache_hit(tag);
        if(hit_line!=-1&&cores[i].cache[hit_line].coherency_state!='I')
        return false;
        }
    }

    return true;

}


//processing the files 
u32 get_core_id(string line){
    regex pattern("P[1-9]+");
    smatch match;
    regex_search(line, match, pattern);

    u32 core_id=match.str().at(1)-'0';
    return core_id;
}
u32 get_address(string line){
    regex pattern("<[0-9]+>");
    smatch match;
    regex_search(line, match, pattern);//extract formatted address in form of <160>

    regex addrs_pattern("[0-9]{3,}");
    smatch raw_addrs;
    string formatted_addrs=match.str(); //extract formatted address in form of <160>
    regex_search(formatted_addrs, raw_addrs, addrs_pattern); //extract the raw address from <>
    u32 addrs=stoi(raw_addrs.str());
    return addrs;
}
char get_operation(string line){
    regex pattern("(read)|(write)");
    smatch match;
    regex_search(line,match,pattern);
    string operation=match.str();
    if(operation=="read"){
        return 'r';
    }
    else{
        return 'w';
    }
}




//broadcast function. This will be called by update_coherency_state
void broadcast(u32 tag, char op, int curr_core, bool miss){
    broadcasts++;
    for(int i=0;i<NUM_CORES;i++){
        if(i!=curr_core){  //broadcast to everyone else 
        int hit_line=cores[i].cache_hit(tag);
        if(hit_line!=-1){
            cache_line* curr_line=&(cores[i].cache[hit_line]);
            char curr_state=curr_line->coherency_state;
            if(op=='r'){
               switch (curr_state){
                case 'M':
                case 'O':
                curr_line->coherency_state='O';
                if(miss){cache_to_cache_transfers++;}
                break;
                case 'E':
                curr_line->coherency_state='F';
                if(miss){cache_to_cache_transfers++;}
                break;
                case 'S':
                curr_line->coherency_state='S';
                break;
                case 'I':
                curr_line->coherency_state='I';
                case 'F':
                curr_line->coherency_state='F';
                if(miss){cache_to_cache_transfers++;}
                break;
               }

            }
            
            else if(op=='w'){
               switch (curr_state){
                case 'M':
                case 'O':
                write_backs++;
                curr_line->coherency_state='I';
                break;
                case 'E': 
                case 'S':
                case 'I':
                case 'F':
                curr_line->coherency_state='I';
                break;
               }
            }
        }
    }
    }
}


//update coherency state of each individual line of cache
void cache_line::update_coherency_state(char op, bool miss){
    bool broadcast_=true;
    char prev_state=coherency_state;
    if(op=='r'){ //when reading cache block
        switch (coherency_state){
           case 'M':
           case 'O':
           coherency_state='O';
           break;
           case 'E':
           broadcast_=false; //no need to broadcast when this core is the only one with this data
           break;
           case 'S':
           break;
           case 'I':
           if(exclusive(tag,core_id)){
            coherency_state='E';
           }
           else{
            coherency_state='S';
           }
           break;
           case 'F':
           break;

        }
 
    }

    else if(op=='w'){ //when writing to the cache block
        switch (coherency_state){
           case 'M':
           case 'O':
           coherency_state='M';
           write_backs++;
           break;
           case 'E':
           coherency_state='M';
           broadcast_=false; //no need to broadcast. silently goes to Modify
           break;
           case 'S':
           coherency_state='M';
           break;
           case 'I':
           coherency_state='M';
           break;
           case 'F':
           coherency_state='M';
           break;

        }
    }


    if(broadcast_){ //check if broadcast is necessary then broad cast
        broadcast(tag,op,core_id,miss);
    }
}
int main(int argc, const char * argv[]) {
    if(argc<2){
        exit(1); //no argument
    }
    ifstream file(argv[1]);
    if(!file){
        exit(1); //unable to read file
    }

  

   for(int i=0;i<NUM_CORES;i++){  
    cores[i].ID=i;
    cores[i].set_lines_core_id();
   }


    string line;
    while (getline(file, line)) { // Read the file line by line
       u32 id=get_core_id(line)-1;
       u32 tag = get_address(line);
       char op=get_operation(line);
      
       int cache_hit=cores[id].cache_hit(tag);
       if(cache_hit!=-1){
         cores[id].update_existing_line(cache_hit,op);
         cache_hits++;
       }
       else{
        cores[id].bring_line(tag,op);
        cache_misses++;
       }
    }

    file.close(); // Close the file
    cout<<cache_hits<<endl;
    cout<<cache_misses<<endl;
    cout<<write_backs<<endl;
    cout<<broadcasts<<endl;
    cout<<cache_to_cache_transfers;
    return 0;

 
}
