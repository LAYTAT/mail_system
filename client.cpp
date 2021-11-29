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

void	Bye();
void    user_command();
void    response_to_spread();
void    show_menu();
void    connect_to_spread();
void    event_system_bind();
void    timer_start();
void    variable_init();

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

    switch( command[0] )
    {
        case 'c':
            //check if the input is formatted correctly
            if( command[2] < '1' || command[2] > '5' )
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
            server = command[2] - '0';
            client_server_group = spread_user + to_string(server);
            cout << "join with client_server_group: " << client_server_group << endl;
            ret = SP_join( spread_mbox,  client_server_group.c_str());
            if( ret < 0 ) SP_error( ret );
            // TODO: send this client-server-group to the server
            connected = true;
            break;

        case 'u': // login as user
            ret = sscanf( &command[2], "%s", user_name);
            if( ret < 1 ) {
                cout << " invalid user name " << endl;
                break;
            }
            break;

        case 'l': // list all mails' headers
            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }
            Message msg;
            msg.type = Message::TYPE::LIST;
            ret= SP_multicast(spread_mbox, AGREED_MESS, SERVER_PUBLIC_GROUPS[server].c_str(), (short int)Message::TYPE::LIST, sizeof(Message), (const char *) &msg);
            if( ret < 0 ) SP_error( ret );

            break;

        case 'm': // write a new email
            cout << "write a new email" << endl;
            break;

        case 'd': // delete an email
            cout << "delete an email" << endl;
            break;

        case 'r': // read an email
            cout << "read an email" << endl;
            break;

        case 'v': // print available servers
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
    printf("\nUser> ");
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
    Message             mess;
    membership_info     memb_info;
    int                 num_vs_sets;
    vs_set_info         vssets[MAX_VSSETS];
    unsigned int        my_vsset_index;
    char                members[MAX_MEMBERS][MAX_GROUP_NAME];

    ret = SP_receive(spread_mbox, &service_type, sender_group, MAX_GROUP_SIZE, &num_groups, target_groups,
                     &mess_type, &endian_mismatch, sizeof(mess), (char *) &mess);
    if( ret < 0 )
    {
        if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
            service_type = DROP_RECV;
            printf("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( spread_mbox, &service_type, sender_group, MAX_MEMBERS, &num_groups, target_groups,
                              &mess_type, &endian_mismatch, sizeof(Message), (char *) &mess );
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
        switch (mess.type) {
            case Message::TYPE::LIST: {
                // todo: print out list of headers
                break;
            }
            case Message::TYPE::MEMBERSHIPS: {
                // todo: print out the servers
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
            ret = SP_get_memb_info( (const char *)&mess, service_type, &memb_info );
            if (ret < 0) {
                printf("BUG: membership message does not have valid body\n");
                SP_error( ret );
                exit( 1 );
            }
        if( Is_reg_memb_mess( service_type ) )
        {
            printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                   sender_group, num_groups, mess_type );
            if( Is_caused_join_mess( service_type ) )
            {
                printf("JOIN of %s\n", memb_info.changed_member );
                // TODO: if server has joined SERVER_PUBLIC_GROUPS, then client and server are connected
            }else if( Is_caused_leave_mess( service_type ) ){
                printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                printf("2 received membership message that left group %s\n", sender_group );
                if( sender_group == client_server_group ) {
                    cout << "The server has crashed or caused by network, please switch to another mail server " << endl;
                    // TODO: connected = false
                }
            }
        }else if( Is_caused_leave_mess( service_type ) ){
            printf("1 received membership message that left group %s\n", sender_group );
            if( sender_group == client_server_group ) {
                cout << "The server has crashed or caused by network, please switch to another mail server " << endl;
            }
        }else printf("received incorrecty membership message of type 0x%x\n", service_type );
    }
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
    spread_connect_timeout.sec = 5;
    spread_connect_timeout.usec = 0;
}

void event_system_bind(){
    // bind keyboard event with user input function
    E_attach_fd(KEYBOARD_INPUT_FD, READ_FD, reinterpret_cast<void (*)(int, int, void *)>(user_command), 0, nullptr, LOW_PRIORITY );

    // bind spread message with message processing function
    E_attach_fd(spread_mbox, READ_FD, reinterpret_cast<void (*)(int, int, void *)>(response_to_spread), 0, nullptr, HIGH_PRIORITY );
}