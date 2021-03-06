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
char    user_name[USER_NAME_LEN];
bool    connected;
char    spread_private_group[MAX_GROUP_NAME];
int     ret;
sp_time spread_connect_timeout;  // timeout for connecting to spread network
bool    has_user_name;
Message snd_buf;
Message rcv_buf;
string  server_name;
Header_List headers_buf;
vector<Mail_Header> headers;
int     read_idx;

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
            //check if the input is formatted correctly
            if( server_number < 1 || server_number > 5 )
            {
                printf("Please select a server from 1 to 5.\n");
                break;
            }

            if (server_number == server && connected) {
                cout << "You are already connected with server " << server << endl;
                break;
            }

            cout << " request to connect with server " << server_number << endl;

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
            snd_buf.type = Message::TYPE::NEW_CONNECTION;
            snd_buf.size = strlen(client_server_group.c_str());
            memcpy(&snd_buf.data, client_server_group.c_str(), strlen(client_server_group.c_str())); //email:   client_server_group + spread_private_group
            send_to_server();

            connected = false; // only where the server join the client-server group is when client is connected to server

            break;

        case 'u': // login as user
            ret = sscanf( &command[2], "%s", user_name);
            has_user_name = true;
            if( ret < 1 ) {
                cout << " invalid user name " << endl;
                break;
            }
            cout << " Hello " << user_name << "!, welcome back to your e-mails. :D"<< endl;
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
            snd_buf.type = Message::TYPE::LIST;
            memcpy(snd_buf.data, user_name, strlen(user_name));
            snd_buf.size = strlen(user_name);
            cout << "Sending username:" << snd_buf.data << " for emails list." << endl;
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            break;

        case 'm': // write a new email
        {
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }

            Email new_email;
            cout << "to: <user name>" << endl;
            cout << "to: ";
            cin.getline(new_email.header.to_user_name, USER_NAME_LEN);
            cout << "subject: <subject>" << endl;
            cout << "subject: ";
            cin.getline(new_email.header.subject, SUBJECT_LEN);
            cout << "mail content: <content>" << endl;
            cout << "mail content: ";
            cin.getline(new_email.msg_str, EMAIL_CONTENT_LEN);
            memcpy(new_email.header.from_user_name, user_name, strlen(user_name));
            new_email.header.sendtime = get_time();
            Update new_update;
            new_update.email = new_email;
            memcpy(snd_buf.data, &new_update, sizeof(Update));
            snd_buf.type = Message::TYPE::NEW_EMAIL;

            send_to_server();
            if( ret < 0 ) SP_error( ret );

            break;
        }


        case 'd': // delete an email
        {
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }

            int delete_idx;
            if( !(ss >> delete_idx) ) {
                cout << "please enter the delete idx." << endl;
                break;
            }

            if(delete_idx < 0 || delete_idx >= headers.size()) {
                if(headers.size() == 0) {
                    cout << "there is no mail to be deleted, try list headers" << endl;
                    break;
                }
                cout << "please enter the right idx to be deleted" << endl;
                break;
            }

            snd_buf.type = Message::TYPE::DELETE;
            char delete_mail_id[MAX_MAIL_ID_LEN];
            memcpy(delete_mail_id, headers[delete_idx].mail_id, strlen(headers[delete_idx].mail_id));
            memcpy(snd_buf.data, delete_mail_id, MAX_MAIL_ID_LEN);
            snd_buf.size = strlen(delete_mail_id);
            send_to_server();
            if( ret < 0 ) SP_error( ret );

            headers.clear();
            break;
        }

        case 'r': // read an email
        {
            if(!has_user_name) {
                cout << "please login first" << endl;
                break;
            }

            if(!connected) {
                cout << "Client has not connected to server " << endl;
                break;
            }

            if( !(ss >> read_idx) ) {
                cout << "please enter the read_idx idx." << endl;
                break;
            }

            if(read_idx < 0 || read_idx >= headers.size()) {
                cout << "please enter the correct read_idx idx "  << endl;
                break;
            }

            snd_buf.type = Message::TYPE::READ;
            memcpy(snd_buf.data, &headers[read_idx], sizeof (Mail_Header));
            send_to_server();
            if( ret < 0 ) SP_error( ret );
            break;
        }

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
                     &mess_type, &endian_mismatch, sizeof(Header_List), (char *) &headers_buf);
    if((Message::TYPE)mess_type == Message::TYPE::HEADER) {
        rcv_buf.type =  Message::TYPE::HEADER;
    } else {
        memcpy(&rcv_buf, &headers_buf, sizeof(Message));
    }
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
            case Message::TYPE::HEADER: {
                headers.clear();
                headers.resize(headers_buf.size);
                memcpy(&headers[0], headers_buf.data, headers_buf.size * sizeof(Mail_Header));

                sort(headers.begin(), headers.end(),[](const Mail_Header & a, const Mail_Header & b) {
                    return a.sendtime > b.sendtime;
                });
                cout << endl;
                cout << "=====================  Headers of received emails ====================" << endl;
                cout << "Email order: the newer emails go before later ones in this header list." << endl;
                cout << "Username: " << user_name << endl;
                cout << "Server: " << server << endl;
                cout << "Mailbox size: " << headers_buf.size << endl;
                cout << "index      from         read state      mail_id        subject" << endl;
                for(int i = 0; i < headers.size(); i++) {
                    cout << "  " << i << "       "<< headers[i].from_user_name  << "         " <<
                    ((headers[i].read_state) ? "read  " : "unread")  << "         " << headers[i].mail_id
                    << "            " <<headers[i].subject << endl;
                }
                cout << "=====================================================================" << endl;
                break;
            }
            case Message::TYPE::MEMBERSHIPS: {
                char rcvd[rcv_buf.size];
                memcpy(rcvd, rcv_buf.data, rcv_buf.size);
                string rcvd_str(rcvd);
                cout << "       These are the mail servers in the current mail server's network component: " << endl;
                cout << "[" ;
                for(int server_idx = 1; server_idx < rcvd_str.size(); server_idx++) {
                    if(rcvd_str[server_idx] == '1') {
                        cout << "       " << server_idx << ",";
                    }
                }
                cout << "   ]" << endl;
                break;
                }
            case Message::TYPE::READ: {
                Email received_email;
                memcpy(&received_email, &rcv_buf.data, sizeof(Email));
                cout << " This is the content of the email at idx:" << read_idx <<" you have requested" << endl;
                cout << " from        read state         subject" << endl;
                cout << string(received_email.header.from_user_name, strlen(received_email.header.from_user_name))  << "         "
                <<  ((received_email.header.read_state) ? "read  " : "unread") << "             "
                << string(received_email.header.subject, strlen(received_email.header.subject)) << endl;
                cout << " content: " << endl;
                string email_content_string(received_email.msg_str, strlen(received_email.msg_str));
                cout << " " << email_content_string << endl;
                break;
            }
            case Message::TYPE::NEW_EMAIL_SUCCESS: {
                cout << "   Email sending success: You email is sent" << endl;
                break;
            }

            case Message::TYPE::DELETE_EMAIL_SUCCESS: {
                cout << "   Email deletion success: Requested email is deleted" << endl;
                break;
            }

            default:
                cout << "from the server: Unprocessed type." << endl;
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
            if( Is_caused_join_mess( service_type ) )
            {
                // if server has joined client-server-group, then client and server are connected
                string joined_member_name_str(memb_info.changed_member);
                if(strcmp(sender_group, client_server_group.c_str()) == 0) {
                    if ( joined_member_name_str.find(server_name) != string::npos) {
                        connected = true;
                        cout << "connected to server:" << server << endl;
                    }
                } else {
                    cout << "Some strange member has joined the group " << sender_group << endl;
                }
            }else if( Is_caused_leave_mess( service_type ) ){
                printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                printf("1 received membership message that left group %s\n", sender_group );
                if( sender_group == client_server_group && connected) {
                    connected = false;
                    SP_leave(spread_mbox, client_server_group.c_str());
                    cout << "The server has crashed or is shut down, please switch to another mail server " << endl;
                }
            }else if( Is_caused_disconnect_mess( service_type ) ){
                if(string(memb_info.changed_member) != client_server_group && connected ) {
                    cout << "Server has been disconnected, please switch to another mail server " << endl;
                    SP_leave(spread_mbox, sender_group);
                    connected = false;
                }
            } else if( Is_caused_network_mess( service_type ) ){
                cout << "Network Changed." << endl;
            } else {
                cout << "!!!!Received other type of message, deal with it" << endl;
            }
        }else if( Is_transition_mess(   service_type ) ) {
            cout << "Transitional Message from the network." << endl;
            if( sender_group == client_server_group && connected) {
                connected = false;
                SP_leave(spread_mbox, client_server_group.c_str());
                cout << "The server is not reachable caused of the network, please switch to another mail server " << endl;
            }
        }else if( Is_caused_leave_mess( service_type ) ){
            if( sender_group == client_server_group && connected) {
                connected = false;
                SP_leave(spread_mbox, client_server_group.c_str());
                cout << "The server has crashed or is shut down, please switch to another mail server " << endl;
            }
        }else printf("  received incorrecty membership message of type 0x%x\n", service_type );
    } else printf(" received message of unknown message type 0x%x with ret %d\n", service_type, ret);
    printf("\nUser> ");
    fflush(stdout);
}

void show_menu(){
    printf("\n");
    printf("==========\n");
    printf("Email Client Menu:\n");
    printf("----------\n");
    if(connected) {
        printf("Current connected server: %d \n", server);
        printf("Current user name: %s \n", user_name);
    } else {
        printf("This client has not connected with server. \n");
    }
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

// local variables init
void variable_init(){
    srand(time(nullptr));
    spread_name = to_string(SPREAD_NAME);
    spread_user = to_string(rand()% RAND_MAX);
    connected = false;
    server = -1;
    ret = 0;
    has_user_name = false;
    spread_connect_timeout.sec = 5;
    spread_connect_timeout.usec = 0;
    read_idx = -1;
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