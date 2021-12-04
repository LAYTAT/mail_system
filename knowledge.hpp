//
// Created by Junjie Lei on 11/30/21.
//

#ifndef MAIL_SYSTEM_KNOWLEDGE_HPP
#define MAIL_SYSTEM_KNOWLEDGE_HPP

#include <set>

class Knowledge {
public:
    Knowledge() = default;
    explicit Knowledge(int server_id):server(server_id){
        //load knowledge from file
        cout << "Knowledge: loading knowledge from file." << endl;
        string knowledge_file_str = to_string(server) + KNOWLEDGE_FILE_SUFFIX;
        auto konwledge_file_ptr = fopen(knowledge_file_str.c_str(),"r");
        if (konwledge_file_ptr == nullptr) {
            cout << "there is no knowledge file, start it from scratch." << endl;
        } else {
            fread(knowledge_vec, (TOTAL_SERVER_NUMBER + 1)*(TOTAL_SERVER_NUMBER + 1) * sizeof (int64_t), 1, konwledge_file_ptr);
            this->print();
            fclose(konwledge_file_ptr);
        }
        print();
        cout << "load knowledge from file complete." << endl;
    }

    ~Knowledge()= default;
    Knowledge& operator=(Knowledge &) = delete;

    // Update on the current server’s knowledge matrix
    // using the received knowledge matrix from other servers.
    void update_knowledge_with_other_knowledge(const Knowledge & other_knowledge) {
        cout << "Knowledge: update whole matrix with matrix from server "<< other_knowledge.get_server() << endl;
        const auto other_matrix = other_knowledge.get_matrix();
        cout << "Knowledge: this is the knowledge matrix from server "<< other_knowledge.get_server() << endl;
        other_knowledge.print();
        for(int i = 1; i <= TOTAL_SERVER_NUMBER ; ++i ) {
            if(i == server)
                continue;
            for(int j = 1; j <= TOTAL_SERVER_NUMBER ; ++ j) {
                if(other_matrix[i][j] > knowledge_vec[i][j]) {
                    knowledge_vec[i][j] = other_matrix[i][j];
                }
            }
        }
        cout << "Knowledge: After update my matrix." << endl;
        print();
        //write to file
        save_update_to_file();
    }

    // change knowledge only in the current server row
    void update_knowledge_with_update(const shared_ptr<Update>& executed_update) {
        cout << "Knowledge: update what this server know in the matrix" << endl;
//        cout << "current server(" << server <<") used to have newest update from " << executed_update->server_id
//        << " is " <<  knowledge_vec[server][executed_update->server_id]
//        << " now it is " << executed_update->timestamp << endl;
        knowledge_vec[server][executed_update->server_id] = executed_update->timestamp;
        print();
        //write to file
        save_update_to_file();
    }

    int get_server() const{
        return server;
    }

    // return all the updates that the current server should send to other server
    // for updates from each server, the one who knows the most and has the smaller
    // server_id will be the one who sends all these needed updates
    vector<pair<int, int64_t>> get_sending_updates(const set<int> & current_members){
        cout << "Knowledge: get needed updates from this server" << endl;
        vector<pair<int, int64_t>> ret;
        vector<int64_t> max_update_from_server(TOTAL_SERVER_NUMBER + 1, 0);
        vector<int64_t> min_update_from_server(TOTAL_SERVER_NUMBER + 1, 0);
        for(int i : current_members){
            for(int j = 1; j <= TOTAL_SERVER_NUMBER; ++j ){
                max_update_from_server[j] = max(max_update_from_server[j], knowledge_vec[i][j]);
                min_update_from_server[j] = min(min_update_from_server[j], knowledge_vec[i][j]);
            }
        }

        cout << "Knowledge: This is the min update: [";
        for(const auto & i : min_update_from_server) cout << i << ", ";
        cout << "]" << endl;
        cout << "Knowledge: This is the max update: [";
        for(const auto & i : max_update_from_server) cout << i << ", ";
        cout << "]" << endl;
        print();

        cout << "Knowledge: current server = " << server << endl;
        cout << "Knowledge: current_members = [";
        for(int i : current_members) {
            cout << i << ", " ;
        }
        cout << "]" << endl;


        for(int j = 1; j <= TOTAL_SERVER_NUMBER; ++j ){
            for(int i : current_members){
                if(knowledge_vec[i][j] == max_update_from_server[j] && i == server) {
                    for(auto k = min_update_from_server[j]; k <= max_update_from_server[j]; ++k) {
                        ret.emplace_back(j, k);
                    }
                    break;
                }
            }
        }

    }

    bool is_update_needed(int other_server_id, const int64_t& timestamp) {
        cout << "Knowledge: checking if update is needed" << endl;
        if(knowledge_vec[server][other_server_id] >= timestamp) {
            cout << "This update timestamp " << timestamp
            << " is smaller than what I have from server " << other_server_id << endl;
            return false;
        }
        cout << "This update timestamp " << timestamp
             << " is bigger than what I have from server, so needed " << other_server_id << endl;
        return true;
    }

    void print() const {
//        cout << " server " << server << " knows that: \n server at [row] know the newest update from [col] server" << endl;
        cout << "============== The knowledge matrix =============== " << endl;
        for(int server_id = 1; server_id <= TOTAL_SERVER_NUMBER; ++server_id)
            cout <<"       " << server_id;
        cout << endl;
        for(int server_id = 1; server_id <= TOTAL_SERVER_NUMBER; ++server_id) {
            cout << server_id;
            for(int konws_server_id = 1; konws_server_id <= TOTAL_SERVER_NUMBER; ++konws_server_id) {
                cout << "       " << knowledge_vec[server_id][konws_server_id];
            }
            cout << endl;
        }
    }

    vector<vector<int64_t>> get_matrix() const {
        cout << "Knowledge: copy out a matrix" << endl;
        vector<vector<int64_t>> ret(TOTAL_SERVER_NUMBER + 1, vector<int64_t>(TOTAL_SERVER_NUMBER + 1, 0));
        for(int i = 0 ; i <= TOTAL_SERVER_NUMBER; ++i) {
            for (int j = 0; j <= TOTAL_SERVER_NUMBER; ++j) {
                ret[i][j] = knowledge_vec[i][j];
            }
        }
//        memcpy(&ret[0][0], knowledge_vec, (TOTAL_SERVER_NUMBER + 1) * (TOTAL_SERVER_NUMBER + 1) * sizeof (int64_t));
        return ret;
    }
private:
    void save_update_to_file(){
        cout << "Knowledge: saving to file" << endl;
        string knowledge_file_str = to_string(server) + KNOWLEDGE_FILE_SUFFIX;
        auto konwledge_file_ptr = fopen(knowledge_file_str.c_str(),"w");
        if (konwledge_file_ptr == nullptr) perror ("20 Error opening file");
        fwrite(knowledge_vec, (TOTAL_SERVER_NUMBER + 1)*(TOTAL_SERVER_NUMBER + 1) * sizeof (int64_t), 1, konwledge_file_ptr);
        fclose(konwledge_file_ptr);
    }


private:
    int64_t knowledge_vec[TOTAL_SERVER_NUMBER + 1][TOTAL_SERVER_NUMBER + 1] = {{0}};
    int server;
};

#endif //MAIL_SYSTEM_KNOWLEDGE_HPP
