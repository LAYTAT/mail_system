//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_STATE_H
#define MAIL_SYSTEM_STATE_H
#include "common_include.h"
#include <set>

struct MyComp {
    bool operator() ( shared_ptr<Update>& a,  shared_ptr<Update>& b) {
        return a->timestamp < b->timestamp;
    }
} myComp;

class Log {
public:
    Log(int server_id):server_2_update_id(),id_2_update(), server_id(server_id){
        cout << " =========log init========= " << endl;
        for(int server_ = 1; server_ < TOTAL_SERVER_NUMBER + 1; ++server_) { // shift by
            count(server_); // todo: delete this after debugging
            load_log_from_file_for_server(server_);
        }
        cout << " ========================== " << endl;
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        assert(update != nullptr);
        assert(server_2_update_id[update->server_id].count(update->timestamp) == 0); // there is no such update before

        cout << "   Adding update to the log" << endl;
        server_2_update_id[update->server_id].insert(update->timestamp); // this update belongs to this server
        id_2_update[update->timestamp] = update; // preserve the update in the memory
        append_to_log(update);
    }

    // delete update to an update
    void delete_update(const int server_id, const int64_t & timestamp) {
        assert(server_2_update_id.count(server_id) == 1); // must have it before delete it
        server_2_update_id[server_id].erase(timestamp); // do not preserve this update for this server
        id_2_update.erase(timestamp); // do not preserve this update in memory
        delete_from_log(server_id, timestamp);
    }

private:
    void load_log_from_file_for_server(int i){
        Update update_tmp;
        int j;
        string log_file_name = to_string(server_id) + "_" + to_string(i) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) log_fptr = fopen(log_file_name.c_str(), "w");
        if (log_fptr == nullptr) perror ("Error opening file");
        while(fread(&update_tmp,sizeof(Update),1,log_fptr))
        {
            server_2_update_id[i].insert(update_tmp.timestamp);
            id_2_update[update_tmp.timestamp] = make_shared<Update>(update_tmp);
        }
        fclose(log_fptr);
    }

    void count(int i){
        Update update_tmp;
        string log_file_name = to_string(server_id) + "_" + to_string(i) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) log_fptr = fopen(log_file_name.c_str(), "w");
        if (log_fptr == nullptr) perror ("Error opening file");
        fseek(log_fptr,0,SEEK_END);
        int n = ftell(log_fptr)/sizeof(update_tmp);
        cout << " Here is number ofr updates stored for server " << i << " : " << n << endl;
        fclose(log_fptr);
    }

    void append_to_log(shared_ptr<Update>& update){
        string log_file_name = to_string(server_id) + "_" + to_string(update->server_id) + LOG_FILE_SUFFIX;
        log_fptr = fopen(log_file_name.c_str(),"a");
        if (log_fptr == nullptr) perror ("Error opening file");
        constexpr static int number_of_updates = 1;
        fwrite(update.get(),sizeof(Update),number_of_updates,log_fptr);
        fclose(log_fptr);
    }

    void delete_from_log(const int server, const int64_t & timestamp){
        int found=0;
        string log_file_name = to_string(server_id) + "_" + to_string(server) + LOG_FILE_SUFFIX;
        string tmp_file_name = to_string(server_id) + "_" + to_string(server) + TEMP_FILE_SUFFIX;
        Update update_tmp;
        FILE * new_log_fptr;
        printf("Enter RollNo to Delete : ");

        log_fptr = fopen(log_file_name.c_str(),"r");
        if (log_fptr == nullptr) perror ("Error opening file");
        auto tmp_fptr = fopen(tmp_file_name.c_str(),"w");
        if (tmp_fptr == nullptr) perror ("Error opening file");
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
            if (log_fptr == nullptr) perror ("Error opening file");
            tmp_fptr = fopen(tmp_file_name.c_str(),"r");
            if (tmp_fptr == nullptr) perror ("Error opening file");

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
    unordered_map<int, set<int64_t>> server_2_update_id; // timestamp
    unordered_map<int64_t, shared_ptr<Update>> id_2_update;
    FILE* log_fptr;
    int server_id;
};

#endif //MAIL_SYSTEM_STATE_H
