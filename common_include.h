//
// Created by Junjie Lei on 11/27/21.
//

#ifndef MAIL_SYSTEM_COMMON_INCLUDE_H
#define MAIL_SYSTEM_COMMON_INCLUDE_H

#define SUBJECT_LEN (128)
#define MSG_LEN (1000)

#include <unordered_map>
#include <memory>
#include <vector>
using namespace std;
struct Email{
    int mail_id;
    int to_user;
    int from_user;
    char subject[SUBJECT_LEN];
    char msg_str[MSG_LEN];
};

using Emails = vector<shared_ptr<Email>> ;
struct Email_Box{
    Email_Box(): emails(){}
    Email_Box(const Email_Box&) = delete;
    Email_Box& operator=(const Email_Box &) = delete;
    ~Email_Box(){}

    Emails emails;
};

struct State{
    State(const State&) = delete;
    State& operator=(const State &) = delete;
    ~State(){}

    unordered_map<int, shared_ptr<Email_Box>> user_2_mailbox;
    unordered_map<int, shared_ptr<Email>> mail_id_2_email;
};

struct Update{
    enum class TYPE {
        READ,
        NEW,
        DELETE
    };
    int server_id;
    int timestamp;
    int mail_id;
    Update::TYPE type;
    Email data;
};


#endif //MAIL_SYSTEM_COMMON_INCLUDE_H
