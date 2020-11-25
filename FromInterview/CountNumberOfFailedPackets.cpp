/**
  There is a sequence of packets "packet". There can be several packets with the same PID.
  Each packet of the sequence with the same PID has continuity_counter property which indicate continued number of the packet.
  Continued number of the packet is a sequence like 0, 1, 2, ..., 15, 0,1,2, ..., etc.
 
    The task is to write method void PidCheck(char* buf, int len) which will be able to count number of skipped packets with the same PID.
*/

#include <stdio.h>
#include <iostream>
#include <sys/stat.h>

#include <unordered_map>
#include <algorithm>

using namespace std;

long _filelength(const char *f)
{
    struct stat st;
    stat(f, &st);
    return st.st_size;
}

struct packet
{             
  unsigned int sync_byte                      :8;    
  unsigned int transport_error_indicator      :1;   
  unsigned int payload_unit_start_indicator   :1;   
  unsigned int transport_priority             :1;   
  unsigned int PID                            :13;  
  unsigned int scrambling_control             :2;
  unsigned int adaptation_field_control       :2;   
  unsigned int continuity_counter             :4;  
  
  char data[184];
};

static constexpr auto sikippedPacets(const unsigned int& continuity_counter, unsigned int prevCc)
{
    integral_constant<int, 16> maxN;
    return ((continuity_counter + maxN - 1) - prevCc) % maxN;
}

void PidCheck(char* buf, int len)
{
    // todo:
    // [ {.PID = 3, .cc = 15}, {.PID = 2, .cc = 9}, {.PID = 3, .cc = 1} ]
    // PID 3 has 1 skipped packet
      
    using PidT = unsigned int;
    struct Counters{
        int counter; //error counter
        unsigned int continuity_counter; //last cc
    };
      
    unordered_map<PidT, Counters> e_counters;
    int packets = len / sizeof(packet);
    packet* data = reinterpret_cast<packet*>(buf);
    const packet* end = data + packets;
    
    for(; data != end; ++data)
    {
        auto it = e_counters.find(data->PID);
        if(it == e_counters.end())
        {
            int temp = data->PID;//perfect forwarding doesn't works with bit fields
            e_counters.emplace(temp, Counters{0, data->continuity_counter});
        }
        else
        {
            it->second.counter += sikippedPacets(data->continuity_counter, it->second.continuity_counter);
            it->second.continuity_counter = data->continuity_counter;
        }
    }
      
    //printing results
    for(const auto& item : e_counters)
    {
        cout << "PID " << item.first << " has " << item.second.counter << " lossed packets" << endl;
    }
  
}

//Fills raw buffer with packert data
#include<cstring>
void generateData(char* buf, size_t len){
    int packets = len / sizeof(packet);
    packet* data = reinterpret_cast<packet*>(buf);
    
    struct TestStruct{
        unsigned int PID;
        unsigned int cc;
    };
    
    constexpr static TestStruct testDataTable [] = {
        {3, 0}, {3, 1}, {4, 1}, {5, 0}, {4, 3}, {3, 0}
    };
    
    memset(data, 1, len);
    
    const packet* end = data + packets;
    for(size_t i = 0; data != end; ++data, i = ++i % sizeof(testDataTable))
    {
        //new (data) packet; //to ensure that all build-in initialisation are done. (The structure "package" can have a default constructor)
        
        data->PID = testDataTable[i].PID;
        data->continuity_counter = testDataTable[i].cc;
    }
}

//Prints the data packets from a row buffer
void printDataPackets(char* buf, size_t len)
{
    const int packets = len / sizeof(packet);
    packet* data = reinterpret_cast<packet*>(buf);
    
    const packet* end = data + packets;
    for(int i = 0; data != end; ++data)
    {
        cout << "PID: " << data->PID << "\t, cc: " <<  data->continuity_counter << endl;
    }
}

int main(int argc, const char * argv[]) {
    char buf[6 * sizeof(packet)];
    generateData(buf, sizeof(buf));
    printDataPackets(buf, sizeof(buf));
    PidCheck(buf, sizeof(buf));
    return 0;
}
