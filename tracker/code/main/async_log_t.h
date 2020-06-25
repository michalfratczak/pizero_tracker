#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <tuple>
#include <utility>
#include <iostream>
#include <fstream>

class async_log_t
{
private:
    using logs_t = std::map< std::string, std::vector<std::string> >;
    logs_t      logs_;
    std::mutex  mtx_;
    std::string base_dir_{"./"};

public:
    bool log(const std::string& fname, const std::string& msg)
    {
        std::lock_guard<std::mutex> _lock(mtx_);
        auto log_stream = logs_.find(fname);
        if(log_stream == logs_.end()) {
            bool insert_ok = false;
            std::tie(log_stream, insert_ok) = logs_.emplace( std::make_pair(fname, std::vector<std::string>()) );
            if(!insert_ok)
                return false;
        }
        log_stream->second.push_back(msg);
        return true;
    }

    void save(void)
    {
        logs_t logs_to_save;

        {
            std::lock_guard<std::mutex> _lock(mtx_);
            logs_to_save.swap(logs_);
        }

        for(const auto& log_file_iter : logs_to_save) {
            std::ofstream of;
            of.open( base_dir_ + "/" + log_file_iter.first, std::ofstream::app );
            if(of) {
                for(const auto& msg : log_file_iter.second)
                    of<<msg<<std::endl;
            }
            of.close();
        }
    }
};

/*

int main(int argc, char** argv)
{
    using namespace std;

    signal(SIGINT,  CTRL_C);
	signal(SIGTERM, CTRL_C);

    async_log_t logs;

    // log something 3 times per second
    auto log_producer = [&logs](std::string logfile){
        while(G_RUN) {
            cout<<"Log to "<<logfile<<endl;
            logs.log(logfile, "some message");
            this_thread::sleep_for( chrono::milliseconds(300) );
        }
    };

    // save logs every 3 seconds
    auto log_saver = [&logs]() {
        while(G_RUN) {
            this_thread::sleep_for(chrono::seconds(3));
            logs.save();
        }
    };


    std::vector<std::thread>     threads;

    threads.emplace_back( std::thread( [&log_producer](){log_producer("fileA.log");} ) );
    threads.emplace_back( std::thread( [&log_producer](){log_producer("fileB.log");} ) );
    threads.emplace_back( std::thread( [&log_producer](){log_producer("fileC.log");} ) );
    threads.emplace_back( std::thread(log_saver) );

    for(auto& t : threads)
        if( t.joinable() )
            t.join();

    logs.save(); // last save

}
*/