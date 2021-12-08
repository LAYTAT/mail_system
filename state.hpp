//
// Created by Junjie Lei on 11/28/21.
//

#ifndef MAIL_SYSTEM_LOG_H
#define MAIL_SYSTEM_LOG_H
#include "common_include.h"
#include "knowledge.hpp"
#include <set>

struct Reconcile_Entry {
    Reconcile_Entry() = default;
    Reconcile_Entry(const string& mail_id_str, bool read) {
        assert(strlen(mail_id_str.c_str()) < MAX_MAIL_ID_LEN);
        memcpy(mail_id, mail_id_str.c_str(), strlen(mail_id_str.c_str()));
        is_read = read;
    }
    char mail_id[MAX_MAIL_ID_LEN] = "default_mail_id";
    bool is_read{true}; // if not read, it is for deletion
};

class State{
public:
    State(const State&) = delete;
    State& operator=(const State &) = delete;
    explicit State(int server_id):user_2_mailbox(), mail_id_2_email(), server_knowledge(server_id), server(server_id), server_timestamp(0){
        cout << "State: init from file" << endl;
        load_state_from_file();
        cout << "State: loading aux from file " << endl;
        load_state_aux_from_file();
        cout << "State: initialized" << endl;
        print_all_mails();
    }

    vector<vector<int64_t>> get_knowledge() const {
        cout << "State: copy out knowledge array." << endl;
        return server_knowledge.get_matrix();
    }

    Knowledge get_knowledge_copy(){
        cout << "State: copy out knowledge," << endl;
        return server_knowledge;
    }

    void update_knowledge(const vector<Knowledge> & other_knowledges){
        cout << "State: update my knowledge " << endl;
        for(const auto & k : other_knowledges) {
            server_knowledge.update_knowledge_with_other_knowledge(k);
        }
    }

    vector<tuple<int, int64_t, int64_t>> get_sending_updates_range(const set<int> & current_members){
        return server_knowledge.get_sending_updates_range(current_members);
    }

    const Email_Box& get_email_box(const string & username) {
        cout << "State: get email box of user " << username << endl;
        if(user_2_mailbox.count(username) == 0) {
            cout << " Here is nothing for user " << username <<
            ", creating a empty mailbox for it." << endl;
        }
        return user_2_mailbox[username];
    }

    bool is_update_needed(const shared_ptr<Update>& update) {
        return server_knowledge.is_update_needed(update->server_id, update->timestamp);
    }

    void update(shared_ptr<Update> update) {
        // write this update to file
        cout << "State: Update the state." << endl;

        // change state in ram
        switch (update->type) {
            case Update::TYPE::NEW_EMAIL:
                new_email(update);
                break;
            case Update::TYPE::READ: {
                cout << "State:         read email " << update->mail_id << " from RAM" << endl;

                if(deleted_emails.count(update->mail_id) == 1) {
                    cout << "State:             This mail is already deleted, dismissed." << endl;
                    break;
                }

                if(mail_id_2_email.count(update->mail_id) == 0){
                    cout << "State:             This mail is not received." << endl;
                    cout << "State:             Creating a dummy email." << endl;

                    // create a dummy email in ram
                    auto dummy_email = make_shared<Email>(update->email);
                    memcpy(dummy_email->header.mail_id, update->mail_id, strlen(update->mail_id));
                    dummy_email->header.read_state = true;
                    read_emails.insert(update->mail_id); // mark this email as read
                    mail_id_2_email[update->mail_id] = dummy_email;
                    user_2_mailbox[update->email.header.to_user_name].insert(update->mail_id); // there is no content currently

                    // add aux to file in case it crashes
                    Reconcile_Entry entry_(update->mail_id, true);
                    append_aux_to_file(entry_);

                    // write dummy email into file in case of crashes
                    cout << "State:                 Append new email in file" << endl;
                    string state_file_str = to_string(server) + "." + STATE_FILE_NAME;
                    state_fptr = fopen(state_file_str.c_str(),"a");
                    if (state_fptr == nullptr) perror ("25 Error opening file");
                    fwrite(dummy_email.get(), sizeof(Email), 1, state_fptr);
                    fclose(state_fptr);
                    break;
                }

                // change state in hard drive
                update_state_file(update);

                // Change state in RAM
                mail_id_2_email[update->mail_id]->header.read_state = true;
                update -> email = get_email(update->mail_id); // for return to user if needed

                break;
            }

            case Update::TYPE::DELETE: {
                auto delete_mail_id_ = update->mail_id;
                cout << "State:    delete email from RAM " << endl;

                if(read_emails.count(delete_mail_id_)){ // if deletion on dummy email, the email_id in delete_emails will dismiss the possible new_email
                    cout << "State:     deletion happen on a dummy mail, deleting it from file." << endl;
                    read_emails.erase(delete_mail_id_); // so email_id in read_emails is not needed anymore
                }

                if(deleted_emails.count(delete_mail_id_) == 1) {
                    cout << "State:          deletion on mail " << delete_mail_id_ << "already happened, dismiss" << endl;
                    break;
                }

                if(mail_id_2_email.count(update->mail_id) == 0) {
                    deleted_emails.insert(update->mail_id);
                    cout << "State:          deletion happens on a mail that this server does not have, dismiss" << endl;
                    break;
                }

                if (mail_id_2_email[update->mail_id] == nullptr) {
                    cout << "State:          Do not have the content this mail with mail id " << update->mail_id << endl;
                    break;
                }

                if(user_2_mailbox.count(mail_id_2_email[update->mail_id]->header.to_user_name) == 0) {
                    cout <<  "State:         This server does not have mail box for user "
                    <<  mail_id_2_email[update->mail_id]->header.to_user_name << endl;
                    break;
                }

                // change state in hard drive
                update_state_file(update);

                deleted_emails.insert(update->mail_id);

                user_2_mailbox[mail_id_2_email[update->mail_id]->header.to_user_name].erase(update->mail_id);
                mail_id_2_email.erase(update->mail_id);
                break;
            }

            default:
                cout << " a type is not dealt with " << endl;
                break;
        }

        // update my own knowledge
        server_knowledge.update_knowledge_with_update(update);
    }

    vector<Mail_Header> get_header_list(const string & username){
        cout << "State: get header list for user " << username << endl;
        const auto& mailbox = get_email_box(username);
        cout << "   user " << username << " has " << mailbox.size()
        << " emails in his inbox" << endl;
        vector<Mail_Header> ret;
        for(auto& mid : mailbox) {
            cout << "   mail id: " << mid << endl;
            assert(mail_id_2_email[mid] != nullptr);
            ret.push_back(mail_id_2_email[mid]->get_header_copy());
        }
        return ret;
    }

    void print_all_mails(){
        cout << "State: This is all the mails on this server " << endl;
        for(const auto & p : user_2_mailbox) {
            const auto & user = p.first;
            const auto & mailbox = p.second;
            cout << "   This is all the mails for user " << user << endl;
            if(mailbox.empty()) {
                cout << "       user " << user << " has no emails." << endl;
            } else {
                for(const auto & mid : mailbox) {
                    if(mail_id_2_email.count(mid) == 0) {
                        cout << "       mail id " << mid << " has no corresponding email." << endl;
                    }else {
                        if(mail_id_2_email[mid] == nullptr) {
                            cout << "       mail id " << mid << " email has no content." << endl;
                        } else {
                            mail_id_2_email[mid]->print();
                        }
                    }
                }
            }
        }
    }

    ~State()= default;


    int64_t get_server_timestamp(){
        server_timestamp++;

        string timestamp_file_str = to_string(server) + "." + TIME_STAMP_FILE_NAME;
        auto timestamp_file_ptr = fopen(timestamp_file_str.c_str(),"w");
        if (timestamp_file_ptr == nullptr)
            perror ("1 Error opening file");
        fwrite(&server_timestamp, sizeof(int), 1, timestamp_file_ptr);
        fclose(timestamp_file_ptr);

        return server_timestamp;
    }

    void aux_cleanup(){
        cout << "State: Conducting aux cleanup." << endl;
        for(const auto & mail_id : deleted_emails) {
            Reconcile_Entry entry_(mail_id, false);
            delete_aux_from_file(entry_);
        }
        deleted_emails.clear();
    }

private:
    void delete_aux_from_file(Reconcile_Entry& re) const{
        cout << "State:      Delete aux from file" << endl;
        int found=0;
        string aux_file_str = to_string(server) + "." + STATE_AUX_FILE_NAME;
        string tmp_aux_file_str = to_string(server) + "." + TEMP_FILE_NAME;
        Reconcile_Entry entry_tmp{};
        FILE *fp_tmp;
        string mail_id_str(re.mail_id);

        auto aux_fptr = fopen(aux_file_str.c_str(),"r");
        if (aux_fptr == nullptr) perror ("31 Error opening file");
        fp_tmp = fopen(tmp_aux_file_str.c_str(), "w");
        if (fp_tmp == nullptr) perror ("32 Error opening file");
        while(fread(&entry_tmp,sizeof(Reconcile_Entry),1, aux_fptr)){
            if(entry_tmp.mail_id == mail_id_str && entry_tmp.is_read == re.is_read){ // implicit conversion
                found = 1;
            }
            else
                fwrite(&entry_tmp, sizeof(Reconcile_Entry), 1, fp_tmp);
        }
        fclose(aux_fptr);
        fclose(fp_tmp);

        if(found){
            aux_fptr = fopen(aux_file_str.c_str(),"w");
            if (aux_fptr == nullptr) perror ("33 Error opening file");
            fp_tmp = fopen(tmp_aux_file_str.c_str(), "r");
            if (fp_tmp == nullptr) perror ("34 Error opening file");

            while(fread(&entry_tmp, sizeof(Reconcile_Entry), 1, fp_tmp)){
                fwrite(&entry_tmp,sizeof(Reconcile_Entry),1, aux_fptr);
            }
            fclose(aux_fptr);
            fclose(fp_tmp);
        }
        else
            cout << "File Err: Reconcile with mail_id " << mail_id_str
                 << " is not fount on server " << server << "'s state file." << endl;
    }

    void append_aux_to_file(Reconcile_Entry& re) const{
        string aux_file_str = to_string(server) + "." + STATE_AUX_FILE_NAME;
        cout << "State:      Append new aux in file" << endl;
        auto aux_fptr = fopen(aux_file_str.c_str(),"a");
        if (aux_fptr == nullptr) perror ("30 Error opening file");
        fwrite(&re, sizeof(Reconcile_Entry), 1, aux_fptr);
        fclose(aux_fptr);
    }

    void load_state_aux_from_file(){
        cout << "State: load state_aux from the file " << endl;
        // load the email from file
        Reconcile_Entry entry_tmp{};
        string aux_file_str = to_string(server) + "." + STATE_AUX_FILE_NAME;
        auto aux_fptr = fopen(aux_file_str.c_str(),"r");
        if (aux_fptr == nullptr) aux_fptr = fopen(aux_file_str.c_str(), "w");
        if (aux_fptr == nullptr) perror ("29 Error opening file");
        // read from hard drive
        while(fread(&entry_tmp,sizeof(Reconcile_Entry),1,aux_fptr))
        {
            auto mail_id = entry_tmp.mail_id;
            cout << "Reconcile Entry: email " << mail_id
                 << ((entry_tmp.is_read) ? " is read. " : " is deleted.")  << endl;
            // write to ram
            if(entry_tmp.is_read){
                read_emails.insert(entry_tmp.mail_id);
            } else {
                deleted_emails.insert(entry_tmp.mail_id);
            }
        }
        fclose(aux_fptr);
    }

    void load_state_from_file(){
        cout << "State: load state from the file " << endl;

        // load the email from file
        Email email_tmp;
        string state_file_str = to_string(server) + "." + STATE_FILE_NAME;
        state_fptr = fopen(state_file_str.c_str(),"r");
        if (state_fptr == nullptr) state_fptr = fopen(state_file_str.c_str(), "w");
        if (state_fptr == nullptr) perror ("2 Error opening file");
        cout << "==================== state init ====================" << endl;
        cout << "       mail_id        user_name         subject      content"      << endl;
        while(fread(&email_tmp,sizeof(Email),1,state_fptr))
        {
            auto mail_id = email_tmp.header.mail_id;
            auto user_name = email_tmp.header.to_user_name;
            cout << "         " << mail_id << "            " << user_name
            << "       " << email_tmp.header.subject << "      " << email_tmp.msg_str <<endl;
            user_2_mailbox[user_name].insert(mail_id); // char[] to stirng, implicit conversion
            mail_id_2_email[mail_id] = make_shared<Email>(email_tmp);
        }
        cout << "====================================================" << endl;
        fclose(state_fptr);

        // load time stamp
        string timestamp_file_str = to_string(server) + "." + TIME_STAMP_FILE_NAME;
        auto timestamp_file_ptr = fopen(timestamp_file_str.c_str(),"r");
        if (timestamp_file_ptr == nullptr) {
            server_timestamp = 0;
            timestamp_file_ptr = fopen(timestamp_file_str.c_str(),"w");
            if (timestamp_file_ptr == nullptr) perror ("3 Error opening file");
        } else
            fread(&server_timestamp, sizeof(int), 1, timestamp_file_ptr);
        cout << " this is the init timestamp for server " << server << " :  " << server_timestamp << endl;
        fclose(timestamp_file_ptr);
    }

    void update_state_file(shared_ptr<Update>& update) {
        cout << "State: update state with an update " << endl;

        string state_file_str = to_string(server) + "." + STATE_FILE_NAME;
        string tmp_state_file_str = to_string(server) + "." + TEMP_FILE_NAME;
        switch (update->type) {
            case Update::TYPE::READ: {
                cout << "State:     Read email in file" << endl;
                int found=0;
                Email email_tmp;
                FILE *fp_tmp;
                string mail_id_str(update->mail_id);

                state_fptr = fopen(state_file_str.c_str(),"r");
                if (state_fptr == nullptr) perror ("4 Error opening file");
                fp_tmp = fopen(tmp_state_file_str.c_str(), "w");
                if (fp_tmp == nullptr) perror ("12 Error opening file");

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
                    if (state_fptr == nullptr) perror ("12 Error opening file");
                    fp_tmp = fopen(tmp_state_file_str.c_str(), "r");
                    if (fp_tmp == nullptr) perror ("13 Error opening file");

                    while(fread(&email_tmp, sizeof(Email), 1, fp_tmp)){
                        fwrite(&email_tmp,sizeof(Email),1, state_fptr);
                    }
                    fclose(state_fptr);
                    fclose(fp_tmp);
                }
                else
                    cout << "State: Email with mail_id " << mail_id_str
                         << " is not found on server " << server << "'s state file." << endl;
                break;
            }

            case Update::TYPE::NEW_EMAIL: {
                cout << "State:      Append new email in file" << endl;
                state_fptr = fopen(state_file_str.c_str(),"a");
                if (state_fptr == nullptr) perror ("14 Error opening file");
                fwrite(&update->email, sizeof(Email), 1, state_fptr);
                fclose(state_fptr);
                break;
            }

            case Update::TYPE::DELETE: {
                cout << "State:      Delete email from file" << endl;
                int found=0;
                Email email_tmp;
                FILE *fp_tmp;
                string mail_id_str(update->mail_id);

                state_fptr = fopen(state_file_str.c_str(),"r");
                if (state_fptr == nullptr) perror ("15 Error opening file");
                fp_tmp = fopen(tmp_state_file_str.c_str(), "w");
                if (fp_tmp == nullptr) perror ("16 Error opening file");
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
                    if (state_fptr == nullptr) perror ("17 Error opening file");
                    fp_tmp = fopen(tmp_state_file_str.c_str(), "r");
                    if (fp_tmp == nullptr) perror ("18 Error opening file");

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

    void new_email(shared_ptr<Update>& update) {
        cout << "State:   add new email to state." << endl;
        cout << "State:     new_mail_id = " << update->email.header.mail_id << endl;
        cout << "State:     user_name = " << update->email.header.to_user_name << endl;

        auto email_ptr = make_shared<Email>(update->email);
        auto new_mail_id = email_ptr->header.mail_id;

        // is this email is already stored as a dummy email
        if(read_emails.count(new_mail_id) == 1) {
            cout << "State:     This email is already read, filling dummy email's content." << endl;
            memcpy(mail_id_2_email[new_mail_id]->msg_str, email_ptr->msg_str, strlen(email_ptr->msg_str));
            read_emails.erase(new_mail_id); // no longer a dummy node

            // delete this aux from file
            cout << "State:          AUX: Removing read(dummy) email from aux" << new_mail_id << endl;
            Reconcile_Entry entry_(new_mail_id, true);
            delete_aux_from_file(entry_);

            // update this email in file
            cout << "State:     Update email content in file" << endl;
            int found=0;
            Email email_tmp;
            FILE *fp_tmp;
            string mail_id_str(new_mail_id);
            string state_file_str = to_string(server) + "." + STATE_FILE_NAME;
            string tmp_state_file_str = to_string(server) + "." + TEMP_FILE_NAME;
            state_fptr = fopen(state_file_str.c_str(),"r");
            if (state_fptr == nullptr) perror ("33 Error opening file");
            fp_tmp = fopen(tmp_state_file_str.c_str(), "w");
            if (fp_tmp == nullptr) perror ("34 Error opening file");
            while(fread(&email_tmp,sizeof(Email),1,state_fptr)){
                if(email_tmp.header.mail_id == mail_id_str){ // implicit conversion
                    memcpy(&email_tmp, mail_id_2_email[new_mail_id].get(), sizeof(Email));
                    found = 1;
                }
                fwrite(&email_tmp, sizeof(email_tmp), 1, fp_tmp);
            }
            fclose(state_fptr);
            fclose(fp_tmp);
            if(found){
                state_fptr = fopen(state_file_str.c_str(),"w");
                if (state_fptr == nullptr) perror ("35 Error opening file");
                fp_tmp = fopen(tmp_state_file_str.c_str(), "r");
                if (fp_tmp == nullptr) perror ("36 Error opening file");

                while(fread(&email_tmp, sizeof(Email), 1, fp_tmp)){
                    fwrite(&email_tmp,sizeof(Email),1, state_fptr);
                }
                fclose(state_fptr);
                fclose(fp_tmp);
            }
            else
                cout << "State:      Email with mail_id " << mail_id_str
                     << " is not found on server " << server << "'s state file." << endl;
            return;
        }

        if(deleted_emails.count(new_mail_id) == 1) {
            cout << "State:     This email to be newed is already deleted, dismissed" << endl;
            return;
        }

        // add new email to storage
        update_state_file(update);

        // add new email to ram
        mail_id_2_email[new_mail_id] = email_ptr;
        user_2_mailbox[email_ptr->header.to_user_name].insert(new_mail_id);
    }

    Email get_email(const string & mail_id) {
        cout << "State: copy out email " << mail_id << endl;
        assert(mail_id_2_email.count(mail_id) == 1);
        return *mail_id_2_email[mail_id];
    }

    unordered_map<string, Email_Box> user_2_mailbox;
    unordered_map<string, shared_ptr<Email>> mail_id_2_email;
    unordered_set<string> deleted_emails;
    unordered_set<string> read_emails;
    Knowledge server_knowledge;
    FILE * state_fptr{};
    int server;
    int64_t server_timestamp;
};

#endif //MAIL_SYSTEM_LOG_H
