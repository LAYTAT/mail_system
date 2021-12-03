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
        cout << "   Adding update to the log" << endl;
        server_2_update_id[update->server_id].insert(update->mail_id); // this update belongs to this server
        assert(id_2_update.count(update->mail_id) == 0); // there is no such update before
        id_2_update[update->mail_id] = update; // preserve the update in the memory
        //TODO: add to file
    }

    // delete update to an update
    void delete_update(const int server_id, const string & mail_id) {
        assert(server_2_update_id.count(server_id) == 1); // must have it before delete it
        server_2_update_id[server_id].erase(mail_id); // do not preserve this update for this server
        id_2_update.erase(mail_id); // do not preserve this update in memory
        // TODO: delete from file
    }
private:
    unordered_map<int, set<string>> server_2_update_id; // mail id
    unordered_map<string, shared_ptr<Update>> id_2_update;
};

#endif //MAIL_SYSTEM_STATE_H
