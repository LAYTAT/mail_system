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
    Log():server_2_updates(){
        // TODO:read the file from drive to RAMs
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        assert(update != nullptr);
        cout << "   Adding update to the log" << endl;
        if(server_2_updates.count(update->server_id) == 0)
            server_2_updates.insert(make_pair(update->server_id, set<shared_ptr<Update>, MyComp>(myComp)));
        server_2_updates[update->server_id].insert(update);
        //TODO: add to file
    }

    // delete update to an update
    void delete_update(const int server_id, int64_t timestamp) {
        vector<shared_ptr<Update>> new_vec;
        while( (*server_2_updates[server_id].rbegin())->timestamp >= timestamp){
            auto it = server_2_updates[server_id].end();
            --it;
            server_2_updates[server_id].erase(it);
        }
        // TODO: delete from file
    }
private:
    unordered_map<int, set<shared_ptr<Update>, MyComp>> server_2_updates;
};

#endif //MAIL_SYSTEM_STATE_H
