//
// Created by Junjie Lei on 11/27/21.
//

#ifndef MAIL_SYSTEM_COMMON_INCLUDE_H
#define MAIL_SYSTEM_COMMON_INCLUDE_H

#define SUBJECT_LEN (128)
#define MSG_LEN (1300)
#define EMAIL_LEN (1000)
#define USER_NAME_LEN (64)

#include <unordered_map>
#include <memory>
#include <vector>
using namespace std;
struct Email{
    int mail_id;
    int to_user;
    int from_user;
    char subject[SUBJECT_LEN];
    char msg_str[EMAIL_LEN];
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
    int server_id;
    int timestamp;
    int mail_id;
    Email data;
};

struct Message{
    enum class TYPE {
        UPDATE,
        LIST,
        MEMBERSHIPS,
        READ,
        NEW,
        DELETE
    };
    char data[MSG_LEN];
    Message::TYPE type;
};

struct Mail_Header{
    char *  user_name[USER_NAME_LEN];
    char *  sender_name[USER_NAME_LEN];
    char    server;
    int     num_of_emails;
    int     index;
    int64_t mail_id;
    char *  subject[SUBJECT_LEN];
    int     read_state;
};

#endif //MAIL_SYSTEM_COMMON_INCLUDE_H
