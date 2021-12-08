//
// Created by Junjie Lei on 11/25/21.
//
#include "server.h"

// local variables for server program
int     To_exit = 0;
mailbox spread_mbox;
string  spread_name;
string  spread_user;
char    user_name[80];
bool    connected;
char    spread_private_group[MAX_GROUP_NAME];
int     ret;
sp_time spread_connect_timeout;  // timeout for connecting to spread network
int     server_id;
int     service_type;
char    sender_group[MAX_GROUP_NAME];
int     num_groups;
char    target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
set<int> servers_group_member_set;
int16_t mess_type;
int     endian_mismatch;
Message rcv_buf;
Message snd_buf;
Header_List header_snd_buf;
membership_info  memb_info;
vs_set_info      vssets[MAX_VSSETS];
unsigned int     my_vsset_index;
int      num_vs_sets;
char     members[MAX_MEMBERS][MAX_GROUP_NAME];
string  servers_group_str(SERVERS_GROUP);
State*   server_state;
Log*     server_log;
int64_t server_timestamp;
Message snd_to_servers_grp_buf;
vector<Knowledge> knowledge_collection;
int64_t full_member_update_counter;

// server functions
void reconcile_start();
void connect_to_spread();
void Bye();
bool command_input_check(int , char * []);
void variable_init();
void create_server_public_group();
void join_servers_group();
void send_to_client(const char * client);
void send_headers_client(const char * client);
shared_ptr<Update> get_log_update();
void send_to_other_servers();
void garbage_collection(); // TODO: exchange knowledge when group size is 5 for a while

int main(int argc, char * argv[]){
    if(!command_input_check(argc, argv))
        return 0;

    variable_init();

    connect_to_spread(); // the program will exit if connection to spread has failed

    create_server_public_group();

    join_servers_group();

    while(true){
        ret = SP_receive(spread_mbox, &service_type, sender_group, MAX_MEMBERS, &num_groups, target_groups, &mess_type, &endian_mismatch, sizeof(Message), (char *)&rcv_buf);
        if( ret < 0 )
        {
            if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
                service_type = DROP_RECV;
                printf("\n========Buffers or Groups too Short=======\n");
                ret = SP_receive( spread_mbox, &service_type, sender_group, MAX_MEMBERS, &num_groups, target_groups,
                                  &mess_type, &endian_mismatch, sizeof(Message), (char *)&rcv_buf );
            }
        }
        if (ret < 0 )
        {
            if( ! To_exit )
            {
                SP_error( ret );
                printf("\n============================\n");
                printf("\nBye.\n");
            }
            exit( 0 );
        }

        if (Is_regular_mess( service_type )) {
            switch (rcv_buf.type) {
                case Message::TYPE::NEW_CONNECTION: {
                    cout << "NEW_CONNECTION: new connection from client." << endl;
                    char client_server_group[rcv_buf.size];
                    memcpy(client_server_group, rcv_buf.data, rcv_buf.size);
                    cout << "NEW_CONNECTION: I am server " << server_id << " and I am gonna join group: "
                    << client_server_group << " now." << endl;
                    ret = SP_join(spread_mbox, client_server_group);
                    while(ret < 0) {
                        cout << "NEW_CONNECTION: Cannot join group with client: "
                             << client_server_group << " trying again now." << endl;
                        ret = SP_join(spread_mbox, client_server_group);
                    }
                    break;
                }

                case Message::TYPE::LIST : { // send back headers for a user
                    cout << "LIST: " << sender_group << " has request a LIST." << endl;
                    char read_request_user_name[rcv_buf.size];
                    memcpy(read_request_user_name, rcv_buf.data, rcv_buf.size);
                    string read_request_user_name_str(read_request_user_name, rcv_buf.size);
                    cout << "User " << read_request_user_name << " has request to list his email" << endl;
                    auto headers_to_return = server_state->get_header_list(read_request_user_name_str);
                    header_snd_buf.size = headers_to_return.size();
                    cout << "LIST:   return list size = " << headers_to_return.size();
                    cout << "LIST:   memcpy size = " << (sizeof(Mail_Header) * headers_to_return.size());
                    memcpy(header_snd_buf.data, headers_to_return.data(), (sizeof(Mail_Header) * headers_to_return.size()));
                    vector<Mail_Header> ret_headers;
                    ret_headers.resize(header_snd_buf.size);
                    memcpy(&ret_headers[0], header_snd_buf.data, header_snd_buf.size * sizeof(Mail_Header));
                    cout << "LIST:   Headers to return:" << endl;
                    cout << "Index      from        subject" << endl;
                    for(int i = 0; i < ret_headers.size(); i++) {
                        cout << "  " << i << "       " << ret_headers[i].from_user_name << "     " << ret_headers[i].subject << endl;
                    }
                    send_headers_client(sender_group);
                    break;
                }

                case Message::TYPE::NEW_EMAIL: { // add a new email in one user's mailbox and send this update to other servers
                    cout << "NEW_EMAIL:" << sender_group << " has request a NEW_EMAIL." << endl;

                    // receive
                    auto rcvd_new_update = get_log_update();

                    // save
                    server_log->add_to_log(rcvd_new_update);

                    // process
                    server_state->update(rcvd_new_update);

                    // response to client
                    snd_buf.type = Message::TYPE::NEW_EMAIL_SUCCESS;
                    send_to_client(sender_group);

                    // share the update with other servers
                    memcpy(snd_to_servers_grp_buf.data, rcvd_new_update.get(), sizeof(Update));
                    snd_to_servers_grp_buf.type = Message::TYPE::UPDATE;
                    send_to_other_servers();
                    break;
                }

                case Message::TYPE::READ : { // mark email as read and send back the email content
                    cout << "READ: " << sender_group << " has request a READ." << endl;

                    // receive
                    auto ret_update = get_log_update();

                    // save
                    server_log->add_to_log(ret_update);

                    // process
                    server_state->update(ret_update);

                    // response to client
                    memcpy(snd_buf.data, &ret_update->email, sizeof(Email));
                    snd_buf.type = Message::TYPE::READ;
                    cout << "READ: This is the email content to return " << endl;
                    ((Email*)snd_buf.data)->print();
                    send_to_client(sender_group);

                    // share the update with other servers
                    memcpy(snd_to_servers_grp_buf.data, ret_update.get(), sizeof(Update));
                    snd_to_servers_grp_buf.type = Message::TYPE::UPDATE;
                    send_to_other_servers();

                    break;
                }

                case Message::TYPE::DELETE : { // delete email
                    cout << "DELETE: " << sender_group << " has request a DELETE." << endl;

                    // receive
                    auto new_update = get_log_update();

                    // save
                    server_log->add_to_log(new_update);

                    // process
                    server_state->update(new_update);

                    // response to client
                    snd_buf.type = Message::TYPE::DELETE_EMAIL_SUCCESS;
                    send_to_client(sender_group);

                    // share the update with other servers
                    memcpy(snd_to_servers_grp_buf.data, new_update.get(), sizeof(Update));
                    snd_to_servers_grp_buf.type = Message::TYPE::UPDATE;
                    send_to_other_servers();

                    break;
                }

                case Message::TYPE::MEMBERSHIPS : { // user request for listing membership
                    cout << "MEMBERSHIPS" << sender_group << " has request a MEMBERSHIPS." << endl;
                    string ret_str = "000000";
                    for(auto server_idx : servers_group_member_set) {
                        ret_str[server_idx] = '1';
                    }
                    cout << "MEMBERSHIPS:     replying to " << sender_group << " with " << ret_str << endl;
                    int ret_str_len = strlen(ret_str.c_str());
                    snd_buf.size = ret_str_len;
                    snd_buf.type = Message::TYPE::MEMBERSHIPS;
                    memcpy(snd_buf.data, ret_str.c_str(), ret_str_len);
                    send_to_client(sender_group);
                    break;
                }

                case Message::TYPE::UPDATE : { // process update from the servers_group
                    cout << sender_group << "UPDATE: received an Update." << endl;
                    // receive
                    auto rcvd_update = make_shared<Update>();
                    memcpy(rcvd_update.get(), rcv_buf.data, sizeof (Update));
                    cout << "from server " << rcvd_update->server_id
                    << ", timestamp = " << rcvd_update->timestamp << endl;

                    garbage_collection();

                    if(!server_state->is_update_needed(rcvd_update)) {
                        cout << "UPDATE: update not needed." << endl;
                        break;
                    }

                    // if my own update
                    if(rcvd_update->server_id == server_id) {
                        cout << "UPDATE: Received my own update, dismissed." << endl;
                        break;
                    }

                    // save
                    server_log->add_to_log(rcvd_update);

                    // process
                    server_state->update(rcvd_update);

                    break;
                }

                case Message::TYPE::KNOWLEDGE_EXCHANGE: {
                    Knowledge rcvd_knowledge;
                    memcpy(&rcvd_knowledge, rcv_buf.data, sizeof (Knowledge));

                    cout << "Knowledge: Received knowledge from server " << rcvd_knowledge.get_server() << endl;
                    knowledge_collection.push_back(rcvd_knowledge);
                    cout << "Knowledge: Current collection size = " << knowledge_collection.size() << endl;

                    if(servers_group_member_set.size() > 1 && knowledge_collection.size() == servers_group_member_set.size()) {
                        cout << "Reconcile: My knowledge is enough to reconcile now! " << endl;
                        server_state->update_knowledge(knowledge_collection);
                        server_log->log_file_cleanup_according_to_knowledge(server_state->get_knowledge());
                        auto updates_to_be_sent_range = server_state->get_sending_updates_range(servers_group_member_set);

                        for(const auto & update_to_send_info : updates_to_be_sent_range) {
                            auto server_    = get<0>(update_to_send_info);
                            auto start_     = get<1>(update_to_send_info);
                            auto end_       = get<2>(update_to_send_info);
                            cout << "Reconcile: need to send update [" << start_ << ","
                            << end_ << "] from server " << server_<< endl;

                            const auto updates_to_send = server_log->get_updates_of_server(server_, start_);
                            for(const auto& update_to_send : updates_to_send) {
                                snd_to_servers_grp_buf.type = Message::TYPE::UPDATE;
                                cout << "Reconcile: Sending update from " << update_to_send->server_id << " with timestamp " << update_to_send->timestamp << endl;
                                memcpy(snd_to_servers_grp_buf.data, update_to_send.get(), sizeof(Update));
                                send_to_other_servers();
                            }
                        }
                        
                        if(updates_to_be_sent_range.empty()) {
                            cout << "Reconcile: No need to send anything." << endl;
                        }

                        knowledge_collection.clear();
                    }

                    break;
                }
                default:
                    cout << "Received am undealt Message type." << endl;
                    break;
            }
        }else if( Is_membership_mess( service_type ) )
        {
            ret = SP_get_memb_info((const char *)&rcv_buf, service_type, &memb_info );
            if (ret < 0) {
                printf("BUG: membership message does not have valid body\n");
                SP_error( ret );
                exit( 1 );
            }
            if( Is_reg_memb_mess( service_type ) )
            {
                printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                       sender_group, num_groups, mess_type );

                if(strcmp(sender_group, SERVERS_GROUP) == 0) { // if membership message from servers group
                    cout << "Membership change in servers_group: " << endl;
                    cout << "   New members in the servers_group : " << endl;
                    set<int> cur_member_in_servers_group;
                    set<int> new_member_in_servers_group;
                    for(int i=0; i < num_groups; i++ ) {
                        string member_in_servers_group(target_groups[i]);
                        int find_idx = member_in_servers_group.find(SERVER_USER_NAME_FOR_SPREAD);
                        find_idx += strlen(SERVER_USER_NAME_FOR_SPREAD);
                        int server_id_tmp = member_in_servers_group[find_idx] - '0';
                        cur_member_in_servers_group.insert(server_id_tmp);
                        if(servers_group_member_set.count(server_id_tmp) == 0) // new group member
                        {
                            cout << "       server_id:" << server_id_tmp << endl;
                            new_member_in_servers_group.insert(server_id_tmp);
                        }
                    }
                    servers_group_member_set = cur_member_in_servers_group;
                    cout << "   Current members in the group " << endl;
                    for(auto idx : servers_group_member_set) {
                        cout << "       " << idx << endl;
                    }
                    reconcile_start();
                }

                printf("    grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

                if( Is_caused_join_mess( service_type ) )
                {
                    cout << "==================== JOIN ======================" << endl;
                    cout << " Group: " << sender_group << ", joined member = " << memb_info.changed_member << endl;
                    string joined_member_name(memb_info.changed_member);
                    cout << "   joined_member_name = " << joined_member_name << endl;
                }else if( Is_caused_leave_mess( service_type ) ){
                    printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                    if(sender_group != servers_group_str ) { // client has leaved
                        cout << "A client has left group " << sender_group << endl;
                        cout << "   This server is also leaving group " << sender_group << endl;
                        SP_leave(spread_mbox, sender_group);
                    }
                }else if( Is_caused_disconnect_mess( service_type ) ){
                    if(sender_group != servers_group_str ) {// client has disconnected
                        cout << "A client has been disconnected from the group " << sender_group << endl;
                        cout << "   This server is also leaving group " << sender_group << endl;
                        SP_leave(spread_mbox, sender_group);
                    }
                    printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
                }else if( Is_caused_network_mess( service_type ) ){
                    printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
                    num_vs_sets = SP_get_vs_sets_info((const char *)&rcv_buf, &vssets[0], MAX_VSSETS, &my_vsset_index );
                    if (num_vs_sets < 0) {
                        printf("BUG: membership message has more than %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
                        SP_error( num_vs_sets );
                        exit( 1 );
                    }
                    for(int i = 0; i < num_vs_sets; i++ )
                    {
                        printf("%s VS set %d has %u members:\n",
                               (i  == my_vsset_index) ?
                               ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
                        ret = SP_get_vs_set_members((const char *)& rcv_buf, &vssets[i], members, MAX_MEMBERS);
                        if (ret < 0) {
                            printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
                            SP_error( ret );
                            exit( 1 );
                        }
                        for(int j = 0; j < vssets[i].num_members; j++ )
                            printf("\t%s\n", members[j] );
                    }
                }
            }else if( Is_transition_mess(   service_type ) ) {
                printf("received TRANSITIONAL membership for group %s\n", sender_group );
            }else if( Is_caused_leave_mess( service_type ) ){
                printf("received membership message that left group %s\n", sender_group );
            }else printf("received incorrecty membership message of type 0x%x\n", service_type );
        }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
    }

    delete server_state;
    delete server_log;
    return 0;
}

void reconcile_start(){
    knowledge_collection.clear();

    // send knowledge to other servers
    snd_to_servers_grp_buf.type = Message::TYPE::KNOWLEDGE_EXCHANGE;
    auto my_knowledge = server_state->get_knowledge_copy();
    memcpy(snd_to_servers_grp_buf.data, &my_knowledge, sizeof(Knowledge));

    send_to_other_servers();
}

void connect_to_spread(){
    ret = SP_connect_timeout(spread_name.c_str(), spread_user.c_str(), SPREAD_PRIORITY, RECEIVE_GROUP_MEMBERSHIP, &spread_mbox, spread_private_group, spread_connect_timeout);
    if( ret != ACCEPT_SESSION )
    {
        SP_error( ret );
        Bye();
    }
    cout << "Email Server: connected to "<< spread_name <<" with private group " <<  spread_private_group << endl;
}

void Bye(){
    To_exit = 1;
    printf("\nBye.\n");

    SP_disconnect( spread_mbox );

    exit( 0 );
}

bool command_input_check(int argc, char * argv[]){
    if( argc != 2 or !isdigit(*argv[1])) {
        cout << "please enter the server id as : ./server <server_id>" << endl;
        cout << "server_id is one of {1,2,3,4,5}" << endl;
        return 0;
    }

    stringstream s1(argv[1]);
    s1 >> server_id;

    if(server_id < 1 || server_id > 5) {
        cout << "server_id is one of {1,2,3,4,5}" << endl;
        return false;
    }
    return true;
}

void variable_init(){
    // local variables init
    srand(time(nullptr));
    spread_name = to_string(SPREAD_NAME);
    spread_user = SERVER_USER_NAME_FOR_SPREAD + to_string(server_id);
    connected = false;
    ret = 0;
    service_type = 0;
    spread_connect_timeout.sec = 5;
    spread_connect_timeout.usec = 0;
    server_timestamp = 0;
    server_state = new State(server_id);
    server_log = new Log(server_id);
    full_member_update_counter = 0;
}

void create_server_public_group(){
    ret = SP_join( spread_mbox,  SERVER_PUBLIC_GROUPS[server_id].c_str());
    if( ret < 0 ) SP_error( ret );
}

void join_servers_group(){
    ret = SP_join(spread_mbox, SERVERS_GROUP);
    if( ret < 0 ) SP_error( ret );
}

void send_to_client(const char * client) {
    ret = SP_multicast(spread_mbox, AGREED_MESS, client, (short int)snd_buf.type, sizeof(Message), (const char *)&snd_buf);
    memset(&snd_buf, 0 , sizeof(Message));
}

void send_headers_client(const char * client) {
    cout << " sending the headers to the client " << endl;
    ret = SP_multicast(spread_mbox, AGREED_MESS, client, (short int)Message::TYPE::HEADER, sizeof(Header_List), (const char *)&header_snd_buf);
    memset(&header_snd_buf, 0 , sizeof(Header_List));
}

void send_to_other_servers(){
    cout << " sending my" <<   (snd_to_servers_grp_buf.type == Message::TYPE::UPDATE ? "update" : "knowledge" )  << " to the other servers." << endl;
    ret = SP_multicast(spread_mbox, AGREED_MESS, servers_group_str.c_str(), (short int)snd_to_servers_grp_buf.type, sizeof(Message), (const char *)&snd_to_servers_grp_buf);
}

// init an update and stamp it with the server stamp
shared_ptr<Update> get_log_update(){
    auto new_update = make_shared<Update>();

    switch (rcv_buf.type) {
        case Message::TYPE::READ: {
            cout << "   requested read on mail with mail_id " << new_update->email.header.mail_id << endl;
            new_update->server_id = server_id;
            new_update->timestamp = server_state->get_server_timestamp();
            new_update->type = Update::TYPE::READ;
            memcpy(&new_update->email.header, rcv_buf.data, sizeof(Mail_Header));
            memcpy(new_update->mail_id, new_update->email.header.mail_id, strlen(new_update->email.header.mail_id)); //used for retrieval
            new_update->email.header.read_state = true;
            break;
        }
        case Message::TYPE::DELETE: {
            memcpy(new_update->email.header.mail_id, rcv_buf.data, rcv_buf.size);
            memcpy(new_update->mail_id, rcv_buf.data, rcv_buf.size);
            cout << "   requested delete on mail with mail_id " << new_update->email.header.mail_id << endl;
            new_update->server_id = server_id;
            new_update->timestamp = server_state->get_server_timestamp();
            new_update->type = Update::TYPE::DELETE;
            break;
        }
        case Message::TYPE::NEW_EMAIL: {
            memcpy(new_update.get(), rcv_buf.data, sizeof(Update));
            new_update->email.print();
            new_update->server_id = server_id;
            new_update->timestamp = server_state->get_server_timestamp();
            string mail_id_str_to_be_assign = to_string(server_id) + to_string(new_update->timestamp);
            memcpy(new_update->email.header.mail_id, mail_id_str_to_be_assign.c_str(), strlen(mail_id_str_to_be_assign.c_str()));
            new_update->email.header.mail_id[strlen(mail_id_str_to_be_assign.c_str())] = 0; // null character
            cout << "       Server " << server_id
                 << " put on it logical time stamp "
                 << new_update->timestamp << endl;
            new_update->type = Update::TYPE::NEW_EMAIL;
            break;
        }
        default:
            cout << "received a type that is not processed." << endl;
            break;
    }
    return new_update;
}

void garbage_collection(){
    if(servers_group_member_set.size() == TOTAL_SERVER_NUMBER) {
        full_member_update_counter++;
    } else {
        full_member_update_counter = 0;
    }
    if(full_member_update_counter == REGULAR_KNOWLEDGE_EXCHANGE_FREQUENCY ) {
            cout << "GC: regular garbage collection starts." << endl;
        // conduct knowledge exchange for garbage collection on log updates
        if(server_id == 1) {
            ret = SP_leave( spread_mbox, SERVERS_GROUP );
            if( ret < 0 ) SP_error( ret );
            ret = SP_join( spread_mbox, SERVERS_GROUP );
            if( ret < 0 ) SP_error( ret );
        }

        // conduct aux cleanup
        server_state->aux_cleanup();

        full_member_update_counter = 0;
        cout << "GC: regular garbage collection ends." << endl;
    }
}