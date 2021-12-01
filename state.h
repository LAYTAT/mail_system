//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_LOG_H
#define MAIL_SYSTEM_LOG_H
#include "common_include.h"
#include "knowledge.h"

// TODO: finish State class implementation
class State{
public:
    State(const State&) = delete;
    State& operator=(const State &) = delete;
    State(){
        // TODO: read the state from the file
    }

    // When the user requires the headers of all received emails,
    // we use this function to get the list concerning one user.
    shared_ptr<Email_Box> get_email_box(const int user_name) {
        // TODO
        return nullptr;
    }

    // When receivs an update, we use this funtion to commit the
    // the change to the state
    void update_email(const string& email_id, const Message::TYPE type) {
        // TODO
    }

    //When the user sends a new email to another user,
    // add the email to the receiving mailbox of the receiver.
    void new_email(shared_ptr<Email>) {
        // TODO
    }

    ~State(){}


private:
    unordered_map<int, shared_ptr<Email_Box>> user_2_mailbox;
    unordered_map<int, shared_ptr<Email>> mail_id_2_email;
};

#endif //MAIL_SYSTEM_LOG_H
