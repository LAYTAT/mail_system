//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_LOG_H
#define MAIL_SYSTEM_LOG_H
#include "common_include.h"
#include "knowledge.hpp"

class State{
public:
    State(const State&) = delete;
    State& operator=(const State &) = delete;
    State(int server_id):user_2_mailbox(), mail_id_2_email(), server(server_id){
        load_state_from_file();
    }

    const Email_Box& get_email_box(const string & username) {
        cout << " get email box of user " << username << endl;
        return user_2_mailbox[username];
    }

    bool is_update_needed(shared_ptr<Update> update) {
        return knowledge.is_update_needed(update->server_id, update->timestamp);
    }

    void update(shared_ptr<Update> update) {
        // write this update to file
        cout << "       update the state." << endl;
        update_state_file(update);
        switch (update->type) {
            case Update::TYPE::NEW_EMAIL:
                new_email(make_shared<Email>(update->email));
                break;
            case Update::TYPE::READ: {
                cout << "           read email from RAM" << update->mail_id << endl;
                assert(mail_id_2_email.count(update->mail_id) == 1);
                mail_id_2_email[update->mail_id]->header.read_state = true;
                update -> email = get_email(update->mail_id); // for return to user
                break;
            }

            case Update::TYPE::DELETE: {
                cout << "           delete email from RAM " << update->mail_id << endl;
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
    void load_state_from_file(){
        Email email_tmp;
        string state_file_str = "./" + to_string(server) + "/" + to_string(server) + STATE_FILE_NAME;
        state_fptr = fopen(state_file_str.c_str(),"r");
        if (state_fptr == nullptr) perror ("Error opening file");
        while(fread(&email_tmp,sizeof(Email),1,state_fptr))
        {
            auto mail_id = email_tmp.header.mail_id;
            auto user_name = email_tmp.header.to_user_name;
            user_2_mailbox[user_name].insert(user_name); // char[] to stirng, implicit conversion
            mail_id_2_email[mail_id] = make_shared<Email>(email_tmp);
        }
        fclose(state_fptr);
    }

    void update_state_file(shared_ptr<Update>& update) {
        string state_file_str = "./" + to_string(server) + "/" + STATE_FILE_NAME;
        string tmp_state_file_str = "./" + to_string(server) + "/" + TEMP_FILE_NAME;
        switch (update->type) {
            case Update::TYPE::READ: {
                cout << "           Read email in file" << endl;
                int found=0;
                Email email_tmp;
                FILE *fp_tmp;
                string mail_id_str(update->mail_id);

                state_fptr = fopen(state_file_str.c_str(),"r");
                if (state_fptr == nullptr) perror ("Error opening file");
                fp_tmp = fopen(tmp_state_file_str.c_str(), "w");
                if (fp_tmp == nullptr) perror ("Error opening file");

                while(fread(&email_tmp,sizeof(Email),1,state_fptr)){
                    if(email_tmp.header.mail_id == mail_id_str){ // implicit conversion
                        memcpy(&email_tmp, &update->email, sizeof(Email));
                        found = 1;
                    }
                    fwrite(&email_tmp, sizeof(email_tmp), 1, fp_tmp);
                }
                fclose(state_fptr);
                fclose(fp_tmp);


                if(found){
                    state_fptr = fopen(state_file_str.c_str(),"w");
                    if (state_fptr == nullptr) perror ("Error opening file");
                    fp_tmp = fopen(tmp_state_file_str.c_str(), "r");
                    if (fp_tmp == nullptr) perror ("Error opening file");

                    while(fread(&email_tmp, sizeof(Email), 1, fp_tmp)){
                        fwrite(&email_tmp,sizeof(Email),1, state_fptr);
                    }
                    fclose(state_fptr);
                    fclose(fp_tmp);
                }
                else
                    cout << "File Err: Email with mail_id " << mail_id_str
                         << " is not fount on server " << server << "'s state file." << endl;
                break;
            }

            case Update::TYPE::NEW_EMAIL: {
                cout << "           Append new email in file" << endl;
                string state_file_name = "./" + to_string(server) + "/" + STATE_FILE_NAME;
                state_fptr = fopen(state_file_name.c_str(),"a");
                if (state_fptr == nullptr) perror ("Error opening file");
                constexpr static int number_of_updates = 1;
                fwrite(&update->email, sizeof(Email), number_of_updates, state_fptr);
                fclose(state_fptr);
                break;
            }

            case Update::TYPE::DELETE: {
                cout << "           Delete email from file" << endl;
                int found=0;
                Email email_tmp;
                FILE *fp_tmp;
                string mail_id_str(update->mail_id);

                state_fptr = fopen(state_file_str.c_str(),"r");
                if (state_fptr == nullptr) perror ("Error opening file");
                fp_tmp = fopen(tmp_state_file_str.c_str(), "w");
                if (fp_tmp == nullptr) perror ("Error opening file");
                while(fread(&email_tmp,sizeof(Email),1,state_fptr)){
                    if(email_tmp.header.mail_id == mail_id_str){ // implicit conversion
                        found = 1;
                    }
                    else
                        fwrite(&email_tmp, sizeof(email_tmp), 1, fp_tmp);
                }
                fclose(state_fptr);
                fclose(fp_tmp);


                if(found){
                    state_fptr = fopen(state_file_str.c_str(),"w");
                    if (state_fptr == nullptr) perror ("Error opening file");
                    fp_tmp = fopen(tmp_state_file_str.c_str(), "r");
                    if (fp_tmp == nullptr) perror ("Error opening file");

                    while(fread(&email_tmp, sizeof(Email), 1, fp_tmp)){
                        fwrite(&email_tmp,sizeof(Email),1, state_fptr);
                    }
                    fclose(state_fptr);
                    fclose(fp_tmp);
                }
                else
                    cout << "File Err: Email with mail_id " << mail_id_str
                         << " is not fount on server " << server << "'s state file." << endl;
                break;
            }
            default:
                cout << "update_state_file: this update type is not dealt with." << endl;
                break;
        }
    }

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
        cout << "       new_mail_id = " << email_ptr->header.mail_id << endl;
        cout << "       user_name = " << email_ptr->header.to_user_name << endl;

        mail_id_2_email[email_ptr->header.mail_id] = email_ptr;
        user_2_mailbox[email_ptr->header.to_user_name].insert(email_ptr->header.mail_id);
    }

    Email get_email(const string & mail_id) {
        assert(mail_id_2_email.count(mail_id) == 1);
        return *mail_id_2_email[mail_id];
    }

    unordered_map<string, Email_Box> user_2_mailbox;
    unordered_map<string, shared_ptr<Email>> mail_id_2_email;
    Knowledge knowledge;
    FILE * state_fptr;
    int server;
};

#endif //MAIL_SYSTEM_LOG_H
