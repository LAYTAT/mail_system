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
#include <cassert>
#include <cstring>
#include <climits>

#define SUBJECT_LEN (128)
#define MSG_LEN (1400)
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
#define MAX_MAIL_ID_LEN  (20)
#define MAX_STATE_AUX_ELEMENT_SIZE (1000)
#define SERVER_USER_NAME_FOR_SPREAD "SVR_CKJL"
#define SERVERS_GROUP "SERVERS_GROUP_CKJL"
#define STATE_FILE_NAME "state"
#define LOG_FILE_SUFFIX ".log"
#define TEMP_FILE_SUFFIX ".temp"
#define TEMP_FILE_NAME "temp"
#define TIME_STAMP_FILE_NAME "timestamp"
#define KNOWLEDGE_FILE_SUFFIX ".know"
#define STATE_AUX_FILE_NAME "aux"
#define REGULAR_KNOWLEDGE_EXCHANGE_FREQUENCY (10000000)

using namespace std;

const string  SERVER_PUBLIC_GROUPS[6] = {"","public_ckjl_1", "public_ckjl_2", "public_ckjl_3", "public_ckjl_4", "public_ckjl_5"};

struct Mail_Header{

    Mail_Header():
    read_state(false)
    {}
    void print(){
        cout << "       mail_id: " << mail_id << endl;
        cout << "       to user: " << to_user_name << endl;
        cout << "       from user: " << from_user_name << endl;
        cout << "       subject: " << subject << endl;
        cout << "       sendtime: " << sendtime << endl;
        cout << "       read state: " << (read_state ? "read" : "unread") << endl;
    }
    int     server{};
    char    mail_id[MAX_MAIL_ID_LEN];
    char    to_user_name[USER_NAME_LEN];
    char    from_user_name[USER_NAME_LEN];
    char    subject[SUBJECT_LEN];
    bool    read_state;
    long int sendtime;
};

struct Email{

    Email() = default;
    Email(Email& others) {
        if(&others != this) {
            memcpy(&header, &others.header, sizeof(Mail_Header));
            memcpy(&msg_str, &others.msg_str, sizeof(others.msg_str));
        } else {
            cout << "DO NOT DO THIS, IT IS MEANINGLESS." << endl;
        }
    }

    void print(){
        cout << "=========Email Print============" << endl;
        header.print();
        cout << "       Content: " << endl;
        cout << "                " << msg_str << endl;
        cout << "================================" << endl;
    }

    Mail_Header get_header_copy(){
        return header;
    }

    char msg_str[EMAIL_CONTENT_LEN];
    Mail_Header header;
};

using Email_Box = unordered_set<string>;

struct Update{
    enum class TYPE {
        READ,
        NEW_EMAIL,
        DELETE
    };
    Update() = default;
    Update(Update& other):
    server_id(other.server_id),
    timestamp(other.timestamp),
    email(other.email),
    type(other.type)
    {
        memcpy(mail_id, other.mail_id, MAX_MAIL_ID_LEN);
    }
    ~Update()= default;

    int server_id{-1};
    int64_t timestamp{};
    char  mail_id[MAX_MAIL_ID_LEN];
    Email email;
    TYPE type;
};

struct Message{
    enum class TYPE {
        UPDATE,
        LIST,
        MEMBERSHIPS,
        NEW_CONNECTION,
        NEW_EMAIL_SUCCESS,
        HEADER,
        DELETE_EMAIL_SUCCESS,
        READ,
        NEW_EMAIL,
        DELETE,
        KNOWLEDGE_EXCHANGE
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

struct Header_List {
    int size;
    char data[MAX_MESSLEN - 4];
};

#endif //MAIL_SYSTEM_COMMON_INCLUDE_H
