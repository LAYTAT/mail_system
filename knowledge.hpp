//
// Created by Junjie Lei on 11/30/21.
//

#ifndef MAIL_SYSTEM_KNOWLEDGE_HPP
#define MAIL_SYSTEM_KNOWLEDGE_HPP

#include <set>

class Knowledge {
public:
    Knowledge(int server_id = -1):server(server_id){
    }
    ~Knowledge()= default;
    Knowledge& operator=(Knowledge &) = delete;

    // Update on the current server’s knowledge matrix
    // using the received knowledge matrix from other servers.
    void update_my_knowledge(const Knowledge & other_knowledge) {
        const auto other_matrix = other_knowledge.get_matrix();
        for(int i = 1; i <= TOTAL_SERVER_NUMBER ; ++i ) {
            if(i == server)
                continue;
            for(int j = 1; j <= TOTAL_SERVER_NUMBER ; ++ j) {
                if(other_matrix[i][j] > knowledge_vec[i][j]) {
                    knowledge_vec[i][j] = other_matrix[i][j];
                }
            }
        }
    }

    int get_server() const{
        return server;
    }

    // return all the updates that the current server should send to other server
    // for updates from each server, the one who knows the most and has the smaller
    // server_id will be the one who sends all these needed updates
    vector<pair<int, int64_t>> get_sending_updates(const set<int> & current_members){
        vector<pair<int, int64_t>> ret;
        vector<int64_t> max_update_from_server(TOTAL_SERVER_NUMBER + 1, 0);
        vector<int64_t> min_update_from_server(TOTAL_SERVER_NUMBER + 1, 0);
        for(int i : current_members){
            for(int j = 1; j <= TOTAL_SERVER_NUMBER; ++j ){
                max_update_from_server[j] = max(max_update_from_server[j], knowledge_vec[i][j]);
                min_update_from_server[j] = min(min_update_from_server[j], knowledge_vec[i][j]);
            }
        }
        for(int i : current_members){
            for(int j = 1; j <= TOTAL_SERVER_NUMBER; ++j ){
                if(knowledge_vec[i][j] == max_update_from_server[j] && i == server) {
                    for(auto k = min_update_from_server[j]; k <= max_update_from_server[j]; ++k) {
                        ret.emplace_back(j, k);
                    }
                }
            }
        }
    }

    bool is_update_needed(int other_server_id, const int64_t& timestamp) {
        if(knowledge_vec[server][other_server_id] >= timestamp) {
            cout << "This update timestamp " << timestamp
            << " is smaller than what I have from server " << other_server_id << endl;
            return false;
        }
        cout << "This update timestamp " << timestamp
             << " is bigger than what I have from server, so needed " << other_server_id << endl;
        return true;
    }

    void print(){
        cout << " server " << server << " knows that: server at [row] know the newest update from [col] server" << endl;
        cout << "============== The knowledge matrix =============== " << endl;
        for(int server_id = 1; server_id <= TOTAL_SERVER_NUMBER; ++server_id)
            cout <<"         update from server " << server_id;
        cout << endl;
        for(int server_id = 1; server_id <= TOTAL_SERVER_NUMBER; ++server_id) {
            cout << "server " << server_id << endl;
            for(int konws_server_id = 1; konws_server_id <= TOTAL_SERVER_NUMBER; ++konws_server_id) {
                cout << "                      " << knowledge_vec[server_id][konws_server_id];
            }
        }
    }

    vector<vector<int64_t>> get_matrix() const {
        vector<vector<int64_t>> ret(TOTAL_SERVER_NUMBER + 1, vector<int64_t>(TOTAL_SERVER_NUMBER + 1, 0));
        memcpy(&ret[0][0], knowledge_vec, (TOTAL_SERVER_NUMBER + 1) * (TOTAL_SERVER_NUMBER + 1));
        return ret;
    }

private:
    int64_t knowledge_vec[TOTAL_SERVER_NUMBER + 1][TOTAL_SERVER_NUMBER + 1] = {{0}};
    int server;
};

#endif //MAIL_SYSTEM_KNOWLEDGE_HPP
