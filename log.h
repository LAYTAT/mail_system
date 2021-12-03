//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_STATE_H
#define MAIL_SYSTEM_STATE_H
#include "common_include.h"
class Log {
public:
    Log():server_2_updates(){
        // TODO:read the file from drive to RAMs
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        cout << "   Adding update to the log" << endl;
        server_2_updates[update->server_id].push_back(update);
        //TODO: add to file
    }

    // delete update to an update
    void delete_update(const int server_id, int64_t timestamp) {
        vector<shared_ptr<Update>> new_vec;
        while(server_2_updates[server_id].back()->timestamp >= timestamp){
            server_2_updates[server_id].pop_back();
        }
        // TODO: delete from file
    }
private:
    unordered_map<int, vector<shared_ptr<Update>>> server_2_updates;
};

#endif //MAIL_SYSTEM_STATE_H
