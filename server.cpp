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
unordered_set<int> servers_group_member_set;
int16_t mess_type;
int     endian_mismatch;
Message rcv_buf;
Message snd_buf;
Header_List header_buf;
membership_info  memb_info;
vs_set_info      vssets[MAX_VSSETS];
unsigned int     my_vsset_index;
int      num_vs_sets;
char     members[MAX_MEMBERS][MAX_GROUP_NAME];
string  servers_group_str(SERVERS_GROUP);
State   server_state;
Log     server_log;
int64_t server_timestamp;

// functions
void process_client_request();
void reconcile();
void update_knowledge();
void stamp_on_sending_buffer();
void connect_to_spread();
void Bye();
bool command_input_check(int , char * []);
void variable_init();
void create_server_public_group();
void join_servers_group();
void send_to_client(const char * client);
int64_t get_server_timestamp();
void send_headers_client(const char * client);

int main(int argc, char * argv[]){
    if(!command_input_check(argc, argv))
        return 0;

    variable_init();

    connect_to_spread(); // the programm will exit if connection to spread has failed

    create_server_public_group();

    join_servers_group();

    while(1){
        // sp_receive
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
                case Message::TYPE::NEW_CONNECTION: { //
                    cout << "new connection from client." << endl;
                    char client_server_group[rcv_buf.size];
                    memcpy(client_server_group, rcv_buf.data, rcv_buf.size);
                    cout << "   where I am server " << server_id << " and I am gonna join group: " << client_server_group << " now." << endl;
                    SP_join(spread_mbox, client_server_group);
                    break;
                }

                case Message::TYPE::NEW_EMAIL: { // add a new email in one user's mailbox and send this update to other servers
                    cout << sender_group << " has request a NEW_EMAIL." << endl;
                    // TODO: process the request

                    // TODO: store update to file
                    Update rcvd_new_update;
                    memcpy(&rcvd_new_update, rcv_buf.data, sizeof(Update));
                    rcvd_new_update.email.print();

                    // if this server is the one to receive this update first
                    // put on server time stamp
                    if(rcvd_new_update.server_id == -1) {
                        rcvd_new_update.server_id = server_id;
                        rcvd_new_update.timestamp = get_server_timestamp();
                        string mail_id_str_to_be_assign = to_string(server_id) + to_string(rcvd_new_update.timestamp);
                        memcpy(rcvd_new_update.email.header.mail_id, mail_id_str_to_be_assign.c_str(), strlen(mail_id_str_to_be_assign.c_str()));
                        rcvd_new_update.email.header.mail_id[strlen(mail_id_str_to_be_assign.c_str())] = 0; // null character
                        cout << "       Server " << server_id
                        << " put on it logicaltime stamp "
                        << rcvd_new_update.timestamp << endl;
                    }

                    // add update to log and email to state
                    server_log.add_to_log(make_shared<Update>(rcvd_new_update));
                    server_state.update(rcvd_new_update, Message::TYPE::NEW_EMAIL);

                    snd_buf.type = Message::TYPE::NEW_EMAIL_SUCCESS;
                    send_to_client(sender_group);
                    break;
                }

                case Message::TYPE::LIST : { // send back headers for a user
                    cout << sender_group << " has request a LIST." << endl;
                    // TODO: process the request and get the infomations
                    char read_request_user_name[rcv_buf.size];
                    memcpy(read_request_user_name, rcv_buf.data, rcv_buf.size);
                    string read_request_user_name_str(read_request_user_name);
                    cout << "User " << read_request_user_name << " has request to read his email" << endl;
                    auto headers_to_return = server_state.get_header_list(read_request_user_name_str);
                    header_buf.size = headers_to_return.size();
                    cout << "   return list size = " << headers_to_return.size();
                    cout << "   memcpy sisze = " << (sizeof(vector<Mail_Header>) + (sizeof(Mail_Header) * headers_to_return.size()));
                    memcpy(header_buf.data, headers_to_return.data(), (sizeof(vector<Mail_Header>) + (sizeof(Mail_Header) * headers_to_return.size())));
                    send_headers_client(sender_group);
                    break;
                }

                case Message::TYPE::READ : { // mark email as read and send back the email content
                    cout << sender_group << " has request a READ." << endl;
                    // TODO: process the request and get the infomations
                    char read_reques_user_name[rcv_buf.size];
                    memcpy(read_reques_user_name, rcv_buf.data, rcv_buf.size);
                    cout << "User " << read_reques_user_name << " has request to read his email" << endl;
                    break;
                }

                case Message::TYPE::DELETE : { // delete email
                    cout << sender_group << " has request a DELETE." << endl;
                    // TODO: process the request
                    break;
                }

                case Message::TYPE::MEMBERSHIPS : { // user request for listing membership
                    cout << sender_group << " has request a MEMBERSHIPS." << endl;
                    string ret_str = "000000";
                    for(auto server_idx : servers_group_member_set) {
                        ret_str[server_idx] = '1';
                    }
                    cout << "     replying to " << sender_group << " with " << ret_str << endl;
                    int ret_str_len = strlen(ret_str.c_str());
                    snd_buf.size = ret_str_len;
                    snd_buf.type = Message::TYPE::MEMBERSHIPS;
                    memcpy(snd_buf.data, ret_str.c_str(), ret_str_len);
                    send_to_client(sender_group);
                    break;
                }

                case Message::TYPE::UPDATE : { // process update from the servers_group
                    cout << sender_group << " has request a Update." << endl;
                    // TODO: process the update on the state
                    break;
                }

                default:
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
                    unordered_set<int> cur_member_in_servers_group;
                    unordered_set<int> new_member_in_servers_group; // TODO: reconcile on this
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
                }

                printf("    grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

                if( Is_caused_join_mess( service_type ) )
                {
                    cout << "==================== JOIN ======================" << endl;
                    cout << " Group: " << sender_group << ", joined member = " << memb_info.changed_member << endl;
                    string joined_member_name(memb_info.changed_member);
                    cout << "   joined_member_name = " << joined_member_name << endl;
                    /*if (strcmp(sender_group, SERVERS_GROUP) == 0 ) //from the servers group
                    {
                        cout << "==================== servers group ======================" << endl;
                        if(joined_member_name.find(spread_user) == string::npos) { //ot the server itself
                            cout << "Another server has join the servers_group ======= " << endl;
                            cout << "The current members in the group : " << endl;
                            string new_member_in_servers_group(memb_info.changed_member);
                            cout << "Now we start reconcile with " << new_member_in_servers_group << endl;
                            reconcile();
                        } else {
                            cout << "Current server " << spread_user << " has join the servers_group !";
                        }
                    }*/

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
    return 0;
}

void process_client_request(){
    //TODO
}

void reconcile(){
    cout << "reconciling!" << endl;
    //TODO
}

void update_knowledge(){
    //TODO: after updates delete unneeded updates
}

void stamp_on_sending_buffer(){
    //TODO
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
}

void create_server_public_group(){
    //TODO: server create a public group with just itself in it.
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
    cout << " send_headers_client " << endl;
    ret = SP_multicast(spread_mbox, AGREED_MESS, client, (short int)Message::TYPE::HEADER, sizeof(Header_List), (const char *)&header_buf);
    memset(&header_buf, 0 , sizeof(Header_List));
}

int64_t get_server_timestamp(){
    server_timestamp++;
    return server_timestamp;
}