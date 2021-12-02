//
// Created by Junjie Lei on 11/27/21.
//

#ifndef MAIL_SYSTEM_COMMON_INCLUDE_H
#define MAIL_SYSTEM_COMMON_INCLUDE_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <sys/time.h>
#include <unordered_set>

#define SUBJECT_LEN (128)
#define MSG_LEN (1300)
#define EMAIL_CONTENT_LEN (1000)
#define USER_NAME_LEN (64)
#define SPREAD_NAME (10040)  //10040 10280
#define SPREAD_PRIORITY (0)
#define RECEIVE_GROUP_MEMBERSHIP (1) // 1 if ture, 0 if false
#define KEYBOARD_INPUT_FD (0)
#define MAX_MESSLEN     (102400)
#define MAX_MEMBERS     (100)
#define MAX_VSSETS      (10)
#define TOTAL_SERVER_NUMBER (5)
#define SERVER_USER_NAME_FOR_SPREAD "SVR_CKJL"
#define SERVERS_GROUP "SERVERS_GROUP_CKJL"
#define STATE_FILE_PREFIX "state_file_"
#define LOG_FILE_PREFIX "log_file_"

using namespace std;

const string  SERVER_PUBLIC_GROUPS[6] = {"","ugrad1_public_ckjl", "ugrad2_public_ckjl", "ugrad3_public_ckjl", "ugrad4_public_ckjl", "ugrad5_public_ckjl"};

struct Mail_Header{

    Mail_Header():
            read_state(false)
    {}
    void print(){
        cout << "       mail_id: " << mail_id << endl;
        cout << "       to user: " << to_user_name << endl;
        cout << "       from user: " << from_user_name << endl;
        cout << "      subject: " << subject << endl;
        cout << "      sendtime: " << sendtime << endl;
        cout << "      read state: " << (read_state ? "read" : "unread") << endl;
    }
    int     server{};
    string mail_id{};
    char    to_user_name[USER_NAME_LEN];
    char    from_user_name[USER_NAME_LEN];
    char    subject[SUBJECT_LEN];
    bool    read_state;
    long int sendtime;
};

struct Email{

    Email() = default;
    Email(Email&) = default;

    Mail_Header header;
    void print(){
        cout << "   This is the email:";
        header.print();
        cout << "       Content: " << msg_str << endl;
    }
    char msg_str[EMAIL_CONTENT_LEN];
};

using Email_Box = unordered_set<string>;

struct Update{
    Update() = default;
    ~Update()= default;
    int server_id{-1};
    int64_t timestamp{};
    int mail_id{};
    Email email;
};

struct Message{
    enum class TYPE {
        UPDATE,
        LIST,
        MEMBERSHIPS,
        READ,
        NEW_EMAIL,
        DELETE,
        NEW_CONNECTION,
        NEW_EMAIL_SUCCESS
    };
    Message::TYPE type;
    char data[MSG_LEN];
    int16_t size;
};

long int get_time(){
    struct timeval t;
    gettimeofday(&t, nullptr);
    return (t.tv_sec * 1000000L +t.tv_usec);
}

#endif //MAIL_SYSTEM_COMMON_INCLUDE_H
