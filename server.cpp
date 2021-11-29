//
// Created by Junjie Lei on 11/25/21.
//
#include "server.h"

// local variables for server program
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
int     server_id;

// functions
void process_client_request();
void reconcile();
void update_knowledge();
void stamp_on_sending_buffer();
void connect_to_spread();
void Bye();

int main(int argc, char * argv[]){
    if( argc != 2 or !isdigit(*argv[1]) or !((*argv[1]) >= '1' and (*argv[1]) <= '5') ) {
        cout << "please enter the server id as : ./server <server_id>" << endl;
        cout << "server_id is one of {1,2,3,4,5}" << endl;
        return 0;
    } else {
        //TODO: delete this after debugging
        cout << "input server successfully" << endl;
    }
    stringstream s1(argv[1]);
    s1 >> server_id;
    // TODO: connect to spread
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
    cout << "Email Client: connected to "<< spread_name <<" with private group " <<  spread_private_group << endl;
}

void Bye(){
    // TODO: bye
}