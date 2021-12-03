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
    Log():server_2_update_id(),id_2_update(){
        // TODO:read the file from drive to RAMs
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        assert(update != nullptr);
        assert(server_2_update_id[update->server_id].count(update->timestamp) == 0); // there is no such update before

        cout << "   Adding update to the log" << endl;
        server_2_update_id[update->server_id].insert(update->timestamp); // this update belongs to this server
        id_2_update[update->timestamp] = update; // preserve the update in the memory
        //TODO: add to file
    }

    // delete update to an update
    void delete_update(const int server_id, const int64_t & timestamp) {
        assert(server_2_update_id.count(server_id) == 1); // must have it before delete it
        server_2_update_id[server_id].erase(timestamp); // do not preserve this update for this server
        id_2_update.erase(timestamp); // do not preserve this update in memory
        // TODO: delete from file
    }
private:
    unordered_map<int, set<int64_t>> server_2_update_id; // mail id
    unordered_map<int64_t, shared_ptr<Update>> id_2_update;
};

#endif //MAIL_SYSTEM_STATE_H
