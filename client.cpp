//
// Created by Junjie Lei on 11/27/21.
//

#include "client.h"

// local variables for client program
int     To_exit = 0;
mailbox spread_mbox;
string  spread_name;
int     server;  // 1 - 5
string  spread_user;
string  client_server_group;
char    user_name[80];
bool    connected;
char    spread_private_group[MAX_GROUP_NAME];
int     ret;
sp_time spread_connect_timeout;  // timeout for connecting to spread network
bool    has_user_name;
Message snd_buf;
Message rcv_buf;
string  server_name;

void	Bye();
void    user_command();
void    response_to_spread();
void    show_menu();
void    connect_to_spread();
void    event_system_bind();
void    timer_start();
void    variable_init();
void    send_to_server();

int main(){

    variable_init();

    connect_to_spread();

    // initialize event system
    E_init();

    event_system_bind();

    // show menu to the client user
    show_menu();

    E_handle_events();

    return 0;
}

void user_command()
{
    char	command[130];
    char	mess[MAX_MESSLEN];
    char	group[80];
    char	groups[10][MAX_GROUP_NAME];
    int	num_groups;
    unsigned int	mess_len;
    int	ret;
    int	i;

    for( i=0; i < sizeof(command); i++ ) command[i] = 0;
    if( fgets( command, 130, stdin ) == nullptr )
        Bye();

    stringstream ss;
    ss << command;
    string command_str;
    ss >> command_str;
    switch( command[0] )
    {
        case 'c':
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            int server_number;
            if( !(ss >> server_number) ) {
                cout << "please enter the server id." << endl;
                break;
            }
            cout << " request to connect with server " << server_number << endl;
            //check if the input is formatted correctly
            if( server_number < 1 || server_number > 5 )
            {
                printf("Please select a server from 1 to 5.\n");
                break;
            }

            //if already connected to a server then disconnect first before connecting to a new server
            if (server != -1) {
                //disconnect from previous server
                ret = SP_leave(spread_mbox, client_server_group.c_str());
                if( ret < 0 ) SP_error( ret );
            }

            //connect to a specified server
            server = server_number;
            server_name = SERVER_USER_NAME_FOR_SPREAD + to_string(server);
            client_server_group = spread_user + "_" +to_string(server);
            cout << "join with client_server_group: " << client_server_group << endl;
            ret = SP_join( spread_mbox,  client_server_group.c_str());
            if( ret < 0 ) SP_error( ret );
            // TODO: send this client-server-group to the server
            snd_buf.type = Message::TYPE::NEW_CONNECTION;
            snd_buf.size = strlen(client_server_group.c_str());
            memcpy(&snd_buf.data, client_server_group.c_str(), strlen(client_server_group.c_str())); //data:   client_server_group + spread_private_group
            send_to_server();
            break;

        case 'u': // login as user
            ret = sscanf( &command[2], "%s", user_name);
            has_user_name = true;
            if( ret < 1 ) {
                cout << " invalid user name " << endl;
                break;
            }
            break;

        case 'l': // list all mails' headers
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }
            // TODO: add the request content
            snd_buf.type = Message::TYPE::LIST;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            break;

        case 'm': // write a new email
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }

            cout << "write a new email" << endl;
            // TODO: add the request content
            snd_buf.type = Message::TYPE::NEW_EMAIL;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            break;

        case 'd': // delete an email
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }
            // TODO: add the request content
            snd_buf.type = Message::TYPE::DELETE;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            cout << "delete an email" << endl;
            break;

        case 'r': // read an email
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }
            // TODO: add the request content
            snd_buf.type = Message::TYPE::READ;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            cout << "read an email" << endl;
            break;

        case 'v': // print available servers
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }

            snd_buf.type = Message::TYPE::MEMBERSHIPS;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            cout << "print available servers" << endl;
            break;

        case 'q':
            Bye();
            break;

        default:
            printf("\nUnknown commnad\n");
            show_menu();

            break;
    }
    cout << "\nUser> ";
    fflush(stdout);
}

void response_to_spread(){
    int                 ret;
    int                 service_type = 0;
    char	            sender_group[MAX_GROUP_NAME];
    static const int    MAX_GROUP_SIZE = 100;
    int                 num_groups;
    char                target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    int16_t             mess_type;
    int                 endian_mismatch;
    membership_info     memb_info;
    int                 num_vs_sets;
    vs_set_info         vssets[MAX_VSSETS];
    unsigned int        my_vsset_index;
    char                members[MAX_MEMBERS][MAX_GROUP_NAME];

    ret = SP_receive(spread_mbox, &service_type, sender_group, MAX_GROUP_SIZE, &num_groups, target_groups,
                     &mess_type, &endian_mismatch, sizeof(Message), (char *) &rcv_buf);
    if( ret < 0 )
    {
        if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
            service_type = DROP_RECV;
            printf("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( spread_mbox, &service_type, sender_group, MAX_MEMBERS, &num_groups, target_groups,
                              &mess_type, &endian_mismatch, sizeof(Message), (char *) &rcv_buf );
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

    // response to regular messages
    if( Is_regular_mess( service_type ) )
    {
        switch (rcv_buf.type) {
            case Message::TYPE::LIST: {
                // todo: print out list of headers
                break;
            }
            case Message::TYPE::MEMBERSHIPS: {
                // todo: print out the servers
                cout << "received membership reply from server " << endl;
                char rcvd[rcv_buf.size];
                memcpy(rcvd, rcv_buf.data, rcv_buf.size);
                string rcvd_str(rcvd);
                cout << "   These are the mail servers in the current mail server's network component: " << endl;
                for(int server_idx = 1; server_idx < rcvd_str.size(); server_idx++) {
                    if(rcvd_str[server_idx] == '1') {
                        cout << "       " << server_idx << endl;
                    }
                }
                break;
                }
            case Message::TYPE::READ: {
                // todo: print out the email content
                break;
            }
            default:
                break;
        }
    } else if (Is_membership_mess( service_type)){
            ret = SP_get_memb_info( (const char *)&rcv_buf, service_type, &memb_info );
            if (ret < 0) {
                printf("BUG: membership message does not have valid body\n");
                SP_error( ret );
                exit( 1 );
            }
        if( Is_reg_memb_mess( service_type ) )
        {
            cout << "Received REGULAR membership for group" << sender_group << "with " << num_groups  <<" members, where I am member " << mess_type << endl;
            if( Is_caused_join_mess( service_type ) )
            {
                // if server has joined client-server-group, then client and server are connected
                string joined_member_name_str(memb_info.changed_member);
                printf("Due to the JOIN of %s\n", memb_info.changed_member );
                cout << "New member: " << joined_member_name_str << " has joined the group " << sender_group << endl;
                if(strcmp(sender_group, client_server_group.c_str()) == 0 && joined_member_name_str.find(server_name) != string::npos) {
                    connected = true;
                    cout << "connected to server " << endl;
                } else {
                    cout << "Some strange member has joined the group" << endl;
                }
            }else if( Is_caused_leave_mess( service_type ) ){
                printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                printf("1 received membership message that left group %s\n", sender_group );
                if( sender_group == client_server_group && connected) {
                    connected = false;
                    SP_leave(spread_mbox, client_server_group.c_str());
                    cout << "The server has crashed or caused by network, please switch to another mail server " << endl;
                    // TODO: connected = false
                }
            }
        }else if( Is_caused_leave_mess( service_type ) ){
            printf("2 received membership message that left group %s\n", sender_group );
            if( sender_group == client_server_group && connected) {
                connected = false;
                SP_leave(spread_mbox, client_server_group.c_str());
                cout << "The server has crashed or caused by network, please switch to another mail server " << endl;
            }
        }else printf("received incorrecty membership message of type 0x%x\n", service_type );
    } else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
    show_menu();
}

void show_menu(){
    printf("\n");
    printf("==========\n");
    printf("Email Client Menu:\n");
    printf("----------\n");
    printf("\n");
    printf("\tu <user_name> -- login with a user name.\n");
    printf("\tc <server_number> -- connect with a server.\n");
    printf("\n");
    printf("\tl -- list headers of received E-mails\n");
    printf("\tm -- write an E-mail\n");
    printf("\td <mail_number> -- delete the mail_number th message in the list\n");
    printf("\n");
    printf("\tr <mail_number> -- read the mail_number th message in the list \n");
    printf("\tv -- print the all available server\n");
    printf("\n");
    printf("\tq -- quit\n");
    fflush(stdout);
    printf("\nUser> ");
    fflush(stdout);
}

void    Bye()
{
    To_exit = 1;
    printf("\nBye.\n");

    SP_disconnect( spread_mbox );

    exit( 0 );
}

void connect_to_spread(){
    ret = SP_connect_timeout(spread_name.c_str(), spread_user.c_str(), SPREAD_PRIORITY, RECEIVE_GROUP_MEMBERSHIP, &spread_mbox, spread_private_group, spread_connect_timeout);
    if( ret != ACCEPT_SESSION )
    {
        SP_error( ret );
        Bye();
    }
    cout << "Email Client: connected to "<< spread_name <<" with private group " <<  spread_private_group << endl;
}

void variable_init(){
    // local variables init
    srand(time(nullptr));
    spread_name = to_string(SPREAD_NAME);
    spread_user = to_string(rand()% RAND_MAX);
    connected = false;
    server = -1;
    ret = 0;
    has_user_name = false;
    spread_connect_timeout.sec = 5;
    spread_connect_timeout.usec = 0;
}

void event_system_bind(){
    // bind keyboard event with user input function
    E_attach_fd(KEYBOARD_INPUT_FD, READ_FD, reinterpret_cast<void (*)(int, int, void *)>(user_command), 0, nullptr, LOW_PRIORITY );

    // bind spread message with message processing function
    E_attach_fd(spread_mbox, READ_FD, reinterpret_cast<void (*)(int, int, void *)>(response_to_spread), 0, nullptr, HIGH_PRIORITY );
}

void send_to_server() {
    ret = SP_multicast(spread_mbox, AGREED_MESS, SERVER_PUBLIC_GROUPS[server].c_str(), (short int)snd_buf.type, sizeof(Message), (const char *)&snd_buf);
    memset(&snd_buf, 0 , sizeof(Message));
}