//
// Created by Junjie Lei on 11/25/21.
//
#include "server.h"

// local variables for server program
int     To_exit = 0;
mailbox spread_mbox;
string  spread_name;
string  spread_user;
string  client_server_group;
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

    while(1){
        // sp_receive
        cout << "before SP_receive" << endl;
        ret = SP_receive(spread_mbox, &service_type, sender_group, MAX_GROUP, &num_groups, target_groups, &mess_type, &endian_mismatch, sizeof(Message), (char *)&rcv_buf);
        cout << "client private group : " << sender_group << endl;
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

}