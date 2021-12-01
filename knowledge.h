//
// Created by Junjie Lei on 11/30/21.
//

#ifndef MAIL_SYSTEM_KNOWLEDGE_H
#define MAIL_SYSTEM_KNOWLEDGE_H

class Knowledge {
public:
    Knowledge(){
        // TODO: read from file and construct knowledge
    }
    ~Knowledge()= default;
    Knowledge& operator=(Knowledge &) = delete;
    Knowledge(Knowledge &) = delete;

    // Update on the current server’s knowledge matrix
    // using the received knowledge matrix from other servers.
    void update_knowledge(const string & server_id, const Knowledge & other_knowledge) {
        // TODO
    }

    // return all the updates that the current server should send to other server
    vector<shared_ptr<Update>> get_sending_updates(){
        // TODO
        return {};
    }

};

#endif //MAIL_SYSTEM_KNOWLEDGE_H
