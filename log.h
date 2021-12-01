//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_STATE_H
#define MAIL_SYSTEM_STATE_H
#include "common_include.h"
// TODO: finish Log class implementation
class Log {
    // read the file from drive to RAM
    Log(){
    }

    // add an update to the log file
    void add_to_log(shared_ptr<Update> update) {
        // TODO
    }

    // delete an update
    void delete_update(const int server_id, const string& timestamp) {
        // TODO

    }
};

#endif //MAIL_SYSTEM_STATE_H
