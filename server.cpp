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
int16_t mess_type;
int     endian_mismatch;
Message rcv_buf;
membership_info  memb_info;
vs_set_info      vssets[MAX_VSSETS];
unsigned int     my_vsset_index;
int      num_vs_sets;
char     members[MAX_MEMBERS][MAX_GROUP_NAME];

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

int main(int argc, char * argv[]){
    if(!command_input_check(argc, argv))
        return 0;

    variable_init();

    connect_to_spread();

    create_server_public_group();

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
                    cout << "new connection." << endl;
                    char client_server_group[rcv_buf.size];
                    memcpy(client_server_group, rcv_buf.data, rcv_buf.size);
                    cout << "I am server " << server_id << " and I am gonna join group: " << client_server_group << " now." << endl;
                    SP_join(spread_mbox, client_server_group);
                    break;
                }

                case Message::TYPE::NEW_EMAIL: { // add a new email in one user's mailbox and send this update to other servers
                    cout << sender_group << " has request a NEW_EMAIL." << endl;
                    // TODO: process the request
                    break;
                }

                case Message::TYPE::LIST : { // send back headers for a user
                    cout << sender_group << " has request a LIST." << endl;
                    // TODO: process the request and get the infomations
                    break;
                }

                case Message::TYPE::READ : { // mark email as read and send back the email content
                    cout << sender_group << " has request a READ." << endl;
                    // TODO: process the request and get the infomations
                    break;
                }

                case Message::TYPE::DELETE : { // delete email
                    cout << sender_group << " has request a DELETE." << endl;
                    // TODO: process the request
                    break;
                }

                case Message::TYPE::MEMBERSHIPS : { // user request for listing membership
                    cout << sender_group << " has request a MEMBERSHIPS." << endl;
                    // TODO: process the request and get the infomations
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
                for(int i=0; i < num_groups; i++ )
                    printf("\t%s\n", &target_groups[i][0] );
                printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

                if( Is_caused_join_mess( service_type ) )
                {
                    string joined_member_name(memb_info.changed_member);
                    if ( joined_member_name.find(spread_user) != string::npos ) // not the server itself
                    {
                        printf("Due to the JOIN of %s\n", memb_info.changed_member );
                        cout << "Now we start reconcile with " << memb_info.changed_member << endl;
                        reconcile();
                    }
                }else if( Is_caused_leave_mess( service_type ) ){
                    printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                }else if( Is_caused_disconnect_mess( service_type ) ){
                    printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
                }else if( Is_caused_network_mess( service_type ) ){
                    printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
                    num_vs_sets = SP_get_vs_sets_info((const char *)&rcv_buf, &vssets[0], MAX_VSSETS, &my_vsset_index );
                    if (num_vs_sets < 0) {
                        printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
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
    // TODO: bye
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
}

void create_server_public_group(){
    //TODO: server create a public group with just itself in it.
    ret = SP_join( spread_mbox,  SERVER_PUBLIC_GROUPS[server_id].c_str());
}