#include <limits.h>
#include "VERSION"

static void parse_argv(int argc, char* argv[]) {
    if (argc != 10) {
	printf
	    (
	     "tcplim version " VERSION "\n"
	     "Usage: tcplim 0.0.0.0 1236 80.83.124.150 2222 65536 65536 32768 32768 50\n"
	     "              bind_ip port connect_ip connect_port total_upload_limit t_download_l per_connection_upload_limit p_c_download_l timetick\n"
	     "\n"
	     "limits are in bytes per second. 0 means Umlimited [converted to INT_MAX internally]\nTimetick is in milliseconds.\n"
	     "\n"
	     "If you specify REDIRECT as connect address or port, I will get addresses using SO_ORIGINAL_DST\n"
	     "(useful with \"iptables -t nat ... -j REDIRECT\")\n"
	     "Parameters will be: tcplim 0.0.0.0 1236 REDIRECT REDIRECT ...\n"
	     "Example of iptables command that will redirect all connections to tcplim:\n"
	     "iptables -t nat -I OUTPUT -m ! uid-owner tcplim_user -p tcp -j REDIRECT --to 1236\n"
	     "(You should run \"tcplim\" from a separate user in REDIRECT mode)\n"
	     "\n"
	     "tcplim accepts commands from console. Type '?' command to see the list, commands are usually one-letter.\n"
	     );
	exit(1);
    }

    /* Setup the command line arguments */
    bind_ip = argv[1];
    bind_port = atoi(argv[2]);
    
    need_address_redirection = !strcmp(argv[3], "REDIRECT");
    need_port_redirection = !strcmp(argv[4], "REDIRECT");

    if (!need_address_redirection) {
	connect_ip = argv[3];
    }
    if (!need_port_redirection) {
	connect_port = atoi(argv[4]);
    }

    total_upload_limit = atoi(argv[5]);
    total_download_limit = atoi(argv[6]);
    fd_upload_limit = atoi(argv[7]);
    fd_download_limit = atoi(argv[8]);
    timetick = atoi(argv[9]);

    #define DEF(x, val)\
    if(!x) x = INT_MAX;
    DEF(total_upload_limit, INT_MAX);
    DEF(total_download_limit, INT_MAX);
    DEF(fd_upload_limit, INT_MAX);
    DEF(fd_download_limit, INT_MAX);
    DEF(timetick, 50);
}
