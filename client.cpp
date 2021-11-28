//
// Created by Junjie Lei on 11/27/21.
//

#include "client.h"
#include "include/sp.h"
#include <string>
#include <random>
#include <time.h>
#include "common_include.h"

#define SPREAD_NAME (10280)
#define SPREAD_PRIORITY (0)
#define RECEIVE_GROUP_MEMBERSHIP (1) // 1 if ture, 0 if false

int     To_exit = 0;
mailbox spread_mbox;
void	Bye();

int main(){
    // local variables
    srand(time(NULL));
    string  spread_name = to_string(SPREAD_NAME);
    string  spread_user = to_string(rand()% RAND_MAX);
    char    Private_group[MAX_GROUP_NAME];

    int ret = 0;
    // connect to Spread timeout
    sp_time test_timeout;
    test_timeout.sec = 5;
    test_timeout.usec = 0;

    ret = SP_connect_timeout(spread_name.c_str(), spread_user.c_str(), SPREAD_PRIORITY, RECEIVE_GROUP_MEMBERSHIP, &spread_mbox, Private_group, test_timeout);
    if( ret != ACCEPT_SESSION )
    {
        SP_error( ret );
        Bye();
    }
    cout << "Connected to Spread!" << endl;

    return 0;
}

void    Bye()
{
    To_exit = 1;
    printf("\nBye.\n");

    SP_disconnect( spread_mbox );

    exit( 0 );
}