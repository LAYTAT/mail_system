//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_STATE_H
#define MAIL_SYSTEM_STATE_H
#include "common_include.h"
#include <set>
#include <map>

struct MyComp {
    bool operator() ( shared_ptr<Update>& a,  shared_ptr<Update>& b) {
        return a->timestamp < b->timestamp;
    }
} myComp;

class Log {
public:
    Log(int server_id): server_2_update_ids(), id_2_update(), server_id(server_id){
        cout << " =========log init========= " << endl;
        for(int server_ = 1; server_ <= TOTAL_SERVER_NUMBER; ++server_) { // shift by
            count(server_);
            load_log_from_file_for_server(server_);
        }
        cout << " ========================== " << endl;
        print_all_updates();
    }

    void print_all_updates() {
        cout << "Log: This is all the update on this server " << endl;
        for(const auto & p : server_2_update_ids) {
            const auto & server_id_ = p.first;
            const auto & update_ids_ = p.second;
            cout << "   Updates for server " << server_id_ << endl;
            if(update_ids_.empty()) {
                cout << "       No updates stored for this server." << endl;
                continue;
            }
            for(const auto & update_id: update_ids_)
            {
                if(id_2_update.count({server_id_,update_id}) == 0) {
                    cout << "       No content stored for update " << update_id << endl;
                    continue;
                }
                cout << "   timestamp = " << id_2_update[{server_id_,update_id}]->timestamp << endl;
            }
        }
    }

    void log_file_cleanup_according_to_knowledge(const vector<vector<int64_t>>& knowledge) {
        cout << "Log: garbage collection." << endl;
        vector<int64_t> min_update_from_server(TOTAL_SERVER_NUMBER + 1, 0);
        for(int i = 1; i <= TOTAL_SERVER_NUMBER; ++i ){
            for(int j = 1; j <= TOTAL_SERVER_NUMBER; ++j ){
                min_update_from_server[j] = min(min_update_from_server[j], knowledge[i][j]);
            }
        }

        for( int s_id = 1; s_id <= TOTAL_SERVER_NUMBER; ++s_id ) {
            auto updates_of_server = server_2_update_ids[s_id];
            for(const auto & stmp :server_2_update_ids[s_id]) {
                if(stmp < min_update_from_server[s_id]) {
                    updates_of_server.erase(stmp);
                    delete_update(s_id, stmp);
                }
            }
            server_2_update_ids[s_id] = updates_of_server;
        }
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        cout << "Log: adding update to log." << endl;
        assert(update != nullptr);
        assert(server_2_update_ids[update->server_id].count(update->timestamp) == 0); // there is no such update before

        cout << "   Adding update to the log" << endl;
        server_2_update_ids[update->server_id].insert(update->timestamp); // this update belongs to this server
        id_2_update[{update->server_id,update->timestamp}] = update; // preserve the update in the memory
        append_to_log(update);
    }

    // delete update to an update
    void delete_update(const int server_id_, const int64_t & timestamp) {
        cout << "Log: delete update from log." << endl;

        assert(server_2_update_ids.count(server_id_) == 1); // must have it before delete it
        server_2_update_ids[server_id_].erase(timestamp); // do not preserve this update for this server
        id_2_update.erase({server_id_,timestamp}); // do not preserve this update in memory
        delete_from_log(server_id_, timestamp);
    }

    shared_ptr<Update> get_update_ptr(const pair<int, int64_t> & key) {
        assert(id_2_update.count(key) == 1);
        return id_2_update[key];
    }

    vector<shared_ptr<Update>> get_updates_of_server(int server_) {
        vector<shared_ptr<Update>>  ret;
        for (const auto & update_id : server_2_update_ids[server_]) {
            ret.push_back(get_update_ptr({server_,update_id}));
        }
        return ret;
    }

private:
    void load_log_from_file_for_server(int i){
        cout << "LOG: loading log from the file" << endl;
        Update update_tmp;
        int j;
        string log_file_name = to_string(server_id) + "_" + to_string(i) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) log_fptr = fopen(log_file_name.c_str(), "w");
        if (log_fptr == nullptr) perror ("5 Error opening file");
        while(fread(&update_tmp,sizeof(Update),1,log_fptr))
        {
            server_2_update_ids[i].insert(update_tmp.timestamp);
            id_2_update[{update_tmp.server_id,update_tmp.timestamp}] = make_shared<Update>(update_tmp);
//            memcpy(id_2_update[update_tmp.timestamp].get(), &update_tmp, sizeof(Update));
        }
        fclose(log_fptr);
    }

    void count(int i){
        Update update_tmp;
        string log_file_name = to_string(server_id) + "_" + to_string(i) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) log_fptr = fopen(log_file_name.c_str(), "w");
        if (log_fptr == nullptr) perror ("6 Error opening file");
        fseek(log_fptr,0,SEEK_END);
        int n = ftell(log_fptr)/sizeof(update_tmp);
        cout << " Here is number ofr updates stored for server " << i << " : " << n << endl;
        fclose(log_fptr);
    }

    void append_to_log(shared_ptr<Update>& update){
        cout << "Log: append update to the file" << endl;
        string log_file_name = to_string(server_id) + "_" + to_string(update->server_id) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"a");
        if (log_fptr == nullptr) perror ("7 Error opening file");
        constexpr static int number_of_updates = 1;
        fwrite(update.get(),sizeof(Update),number_of_updates,log_fptr);
        fclose(log_fptr);
    }

    void delete_from_log(const int server, const int64_t & timestamp){
        cout << "Log: delete update from the file" << endl;
        int found=0;
        string log_file_name = to_string(server_id) + "_" + to_string(server) + LOG_FILE_SUFFIX;
        string tmp_file_name = to_string(server_id) + "_" + to_string(server) + TEMP_FILE_SUFFIX;
        Update update_tmp;
        FILE * new_log_fptr;
        printf("Enter RollNo to Delete : ");

        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) perror ("8 Error opening file");
        auto tmp_fptr = fopen(tmp_file_name.c_str(),"w");
        if (tmp_fptr == nullptr) perror ("9Error opening file");
        while(fread(&update_tmp,sizeof(Update),1,log_fptr)){
            if(update_tmp.timestamp == timestamp){
                found = 1;
            }
            else
                fwrite(&update_tmp,sizeof(Update),1,tmp_fptr);
        }
        fclose(log_fptr);
        fclose(tmp_fptr);


        if(found){
            log_fptr = fopen(log_file_name.c_str(),"w");
            if (log_fptr == nullptr) perror ("10 Error opening file");
            tmp_fptr = fopen(tmp_file_name.c_str(),"r");
            if (tmp_fptr == nullptr) perror ("11 Error opening file");

            while(fread(&update_tmp,sizeof(Update),1,tmp_fptr)){
                fwrite(&update_tmp,sizeof(Update),1,log_fptr);
            }
            fclose(log_fptr);
            fclose(tmp_fptr);
        }
        else
            cout << "File Err: Update with timestamp " << timestamp
            << " is not fount on server " << server << "'s log file." << endl;
    }

private:
    unordered_map<int, set<int64_t>> server_2_update_ids; // timestamp
    map<pair<int, int64_t>, shared_ptr<Update>> id_2_update;
    FILE* log_fptr;
    int server_id;
};

#endif //MAIL_SYSTEM_STATE_H
