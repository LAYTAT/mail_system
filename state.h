//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_LOG_H
#define MAIL_SYSTEM_LOG_H
#include "common_include.h"
#include "knowledge.h"

class State{
public:
    State(const State&) = delete;
    State& operator=(const State &) = delete;
    State():user_2_mailbox(), mail_id_2_email(){
        // TODO: read the state from the file
    }

    // When the user requires the headers of all received emails,
    // we use this function to get the list concerning one user.
    const Email_Box& get_email_box(const string & username) {
        cout << " get email box of user " << username << endl;
        return user_2_mailbox[username];
    }

//    void update(const string& mail_id, const Message::TYPE type) {
//        update_email(mail_id, type);
//    }


    void update(shared_ptr<Update>& update) {
        // write this update to file
        cout << "       update the state." << endl;
        switch (update->type) {
            case Update::TYPE::NEW_EMAIL:
                new_email(make_shared<Email>(update->email));
                break;
            case Update::TYPE::READ: {
                cout << "       read email " << update->mail_id << endl;
                assert(mail_id_2_email.count(update->mail_id) == 1);
                mail_id_2_email[update->mail_id]->header.read_state = true;
                break;
            }

            case Update::TYPE::DELETE: {
                cout << "       delete email " << update->mail_id << endl;
                assert(mail_id_2_email.count(update->mail_id) == 1);
                assert(user_2_mailbox.count(mail_id_2_email[update->mail_id]->header.to_user_name) == 1);
                user_2_mailbox[mail_id_2_email[update->mail_id]->header.to_user_name].erase(update->mail_id);
                mail_id_2_email.erase(update->mail_id);
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
        const auto& mailbox = get_email_box(username);
        vector<Mail_Header> ret;
        for(auto& mid : mailbox) {
            ret.push_back(mail_id_2_email[mid]->get_header());
        }
        return ret;
    }
    ~State(){}


private:
    void print_mails(){
        cout << " =========== Current emails =========== " << endl;
        cout << " mail_id     from    read     send time" << endl;
        for (const auto & p : mail_id_2_email) {
            cout << p.first  << "    " << p.second->header.from_user_name << "    "
            << p.second->header.read_state << "   " << p.second->header.sendtime << endl;
        }
        cout << " ====================================== " << endl;
    }

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
