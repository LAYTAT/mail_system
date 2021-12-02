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
    const Email_Box& get_email_box(const string & username) {
        cout << " get email box of user " << username << endl;
        return user_2_mailbox[username];
    }

    void update(const string& mail_id, const Message::TYPE type) {
        update_email(mail_id, type);
    }


    void update(Update& update, const Message::TYPE type) {
        // write this update to file
        cout << "       update the state." << endl;
        switch (type) {
            case Message::TYPE::NEW_EMAIL:
                new_email(make_shared<Email>(update.email));
                break;
            case Message::TYPE::DELETE:
            case Message::TYPE::READ: {
                string mail_id_str(update.email.header.mail_id, strlen(update.email.header.mail_id));
                update_email(mail_id_str, type);
                update.email = get_email(mail_id_str);
                break;
            }
            default:
                cout << " a type is not dealt with " << endl;
                break;
        }
        // TODO: send this update to other servers
    }

    vector<Mail_Header> get_header_list(const string & username){
        cout << " get header list for user " << username << endl;
        auto mailbox = get_email_box(username);
        vector<Mail_Header> ret;
        for(auto& mid : mailbox) {
            ret.push_back(mail_id_2_email[mid]->get_header());
        }
        return ret;
    }

    ~State(){}


private:
    void print_mails(){
        cout << "   Current emails ===========begin" << endl;
        cout << " mail_id   msg_str";
        for (const auto & p : mail_id_2_email) {
            cout << p.first  << "   " << p.second->msg_str << endl;
        }
        cout << "   Current emails ===========end" << endl;
    }
    // When receivs an update, we use this funtion to commit the
    // the change to the state
    void update_email(const string& email_id, const Message::TYPE type) {
        print_mails();
        cout << "   Updating the email " << email_id << endl;
        if(type == Message::TYPE::READ) {
            cout << "       read email " << email_id << endl;
            assert(mail_id_2_email.count(email_id) == 1);
            mail_id_2_email[email_id]->header.read_state = true;
        } else if(type == Message::TYPE::DELETE) {
            cout << "       delete email " << email_id << endl;
            user_2_mailbox[mail_id_2_email[email_id]->header.to_user_name].erase(email_id);
            mail_id_2_email.erase(email_id);
        }
        print_mails();
        // TODO: change to state file
    }

    //When the user sends a new email to another user,
    // add the email to the receiving mailbox of the receiver.
    void new_email(shared_ptr<Email> email_ptr) {
        cout << "   add new email to state." << endl;
        string new_mail_id(email_ptr->header.mail_id, strlen(email_ptr->header.mail_id));
        cout << "       new_mail_id = " << new_mail_id << endl;
        mail_id_2_email[new_mail_id] = email_ptr;
        string user_name(email_ptr->header.to_user_name);
        cout << "       user_name = " << user_name << endl;
        user_2_mailbox[user_name].insert(new_mail_id);
        // TODO: change to state file
    }

    Email get_email(const string & mail_id) {
        return *mail_id_2_email[mail_id];
    }

    unordered_map<string, Email_Box> user_2_mailbox;
    unordered_map<string, shared_ptr<Email>> mail_id_2_email;
};

#endif //MAIL_SYSTEM_LOG_H
