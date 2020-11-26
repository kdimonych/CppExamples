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
#include <vector>
#include <algorithm>
#include <thread>
#include <future>

using namespace std;

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

template<typename Base, typename Exp>
static constexpr inline size_t intPow(const Base& base = 0, const Exp& exp = 0)
{
    return (exp <= 0) ? 1 : base * intPow(base, exp - 1);
}

static constexpr inline unsigned int sikippedPackets(unsigned int continuity_counter, unsigned int prevCc)
{
    integral_constant<unsigned int, intPow(2, 4)> maxN;
    return ((continuity_counter + maxN - 1) - prevCc) % maxN;
}

struct Counters{
    unsigned int counter = 0; //error counter
    unsigned int initial_continuity_counter = 0;
    unsigned int continuity_counter = -1; // -1 means that the Counter is not tuched
};

static vector<Counters> pidCheckWorker(const packet* begin, const packet* end)
{
    integral_constant<size_t, intPow(2, 13)> maxPidCount;
    vector<Counters> errors(maxPidCount);
    
    for(; begin != end; ++begin)
    {
        assert(begin->PID < maxPidCount);
        
        Counters& error_c = errors[begin->PID]; //lookup counter record for certain PID
        
        if(error_c.continuity_counter == -1)
        {
            error_c.continuity_counter = 0;
            error_c.initial_continuity_counter = begin->continuity_counter;
        }
        else
            error_c.counter += sikippedPackets(begin->continuity_counter, error_c.continuity_counter);
        
        error_c.continuity_counter = begin->continuity_counter;
    }
    
    return errors;
}

static void merge_results(vector<Counters>& first_part, const vector<Counters>& last_part)
{
    assert(first_part.size() == last_part.size());
    
    auto fpBegin = first_part.begin();
    const auto fpEnd = first_part.end();
    auto lpBegin = last_part.begin();
    const auto lpEnd = last_part.end();
    
    for(;fpBegin != fpEnd && lpBegin != lpEnd; ++fpBegin, ++lpBegin)
    {
        if(fpBegin->continuity_counter != -1 && lpBegin->continuity_counter != -1)
        {
            fpBegin->counter += lpBegin->counter + sikippedPackets(lpBegin->initial_continuity_counter, fpBegin->continuity_counter);
            fpBegin->continuity_counter = lpBegin->continuity_counter; //It can be omitted to increase performance
        }
    }
}

static vector<Counters> parallelPidCheck(const packet* begin, const packet* end, int cores = 1, int packetsPerCore = intPow(2, 13))
{
    //integral_constant<size_t, 2 * intPow(2, 13)> packetsPerCore;
    
    const size_t size = distance(begin, end);
    if(size <= packetsPerCore || cores == 1)
        //The task is small enough or no free cores left.
        return pidCheckWorker(begin, end);
    else
    {
        //Unfortunately, the task is too big to compute on a single core.
        //Devide task on two sub tusks
        const packet* mid = begin + size/2;
        
        auto res2 = async(std::launch::async, parallelPidCheck, mid, end, cores/2, packetsPerCore); // share the last half of the task to another thread
        cores -= cores/2;
        auto result = parallelPidCheck(begin, mid, cores, packetsPerCore); // try to compute the first half of the task in current thread
        
        //merge results
        merge_results(result, res2.get());
        return result;
    }
}

void PidCheck(char* buf, int len)
{
    // todo:
    // [ {.PID = 3, .cc = 15}, {.PID = 2, .cc = 9}, {.PID = 3, .cc = 1} ]
    // PID 3 has 1 skipped packet
    
    size_t packets = len / sizeof(packet);
    packet* begin = reinterpret_cast<packet*>(buf);
    const packet* end = begin + packets;
    const int hwConcurrency = thread::hardware_concurrency();
    
    auto result = parallelPidCheck(begin, end, hwConcurrency);
    
    //printing results
    for(auto it = result.begin(); it != result.end(); ++it)
    {
        const size_t pid = distance(result.begin(), it);
        if(it->continuity_counter != -1 && it->counter > 0)
            cout << "PID " << pid << " has " << it->counter << " lost packets" << endl;
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
    
    const size_t testDataSize = sizeof(testDataTable)/sizeof(TestStruct);
    const packet* end = data + packets;
    for(size_t i = 0; data != end; ++data, i = ++i % testDataSize)
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
    int maxLines = 10;
    for(int i = 0; data != end && maxLines > 0; ++data, --maxLines)
    {
        cout << "PID: " << data->PID << "\t, cc: " <<  data->continuity_counter << endl;
    }
    if(data != end)
        cout << "..."<< endl;
}

int main(int argc, const char * argv[]) {
    char buf[6 * sizeof(packet)];
    generateData(buf, sizeof(buf));
    printDataPackets(buf, sizeof(buf));
    PidCheck(buf, sizeof(buf));
    return 0;
}
