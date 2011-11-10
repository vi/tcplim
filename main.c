/* Simple Linux epoll-based single thread TCP bandwidth limiting relay. LGPL. Written by Vitaly "_Vi" Shukela. */

/* We are not using EPOLLET, but use EPOLLONESHOT.
 * We store we_should_epoll_for_{reads,writes} and EPOLL_CTL_MODding almost every time we do something.
 * http://stackoverflow.com/questions/4003804/how-to-read-multiple-file-descriptors-using-epoll-select-with-epollet
 *
 * Out-of-band data is not processed.
 * Half-shutdown connections are expected to work properly.
 *
 * It also supports retrieving destination address with SO_ORIGINAL_DST
 */

#include "main.h"


struct fdinfo_t fdinfo[MAXFD] = { [0 ... MAXFD - 1] = {0, 0, 0}};

int debug_output;
int kdpfd; /* epoll fd */
int ss; /* server socket */
    
const char *bind_ip;
int bind_port;
const char *connect_ip;
int connect_port;

int need_address_redirection;
int need_port_redirection;

int total_upload_limit;
int total_download_limit;
int fd_upload_limit;
int fd_download_limit;
int timetick;

int quotas_are_full; /* if all quota buffers are not drained, we can epoll_wait without a timeout */
struct timeval time_last;



int main(int argc, char *argv[])
{
    parse_argv(argc, argv);
    
    listen_socket_and_setup_epoll();

    gettimeofday(&time_last, NULL);

    int millisecond_accumulator_for_update_rates=0;

    struct epoll_event events[MAX_EPOLL_EVENTS_AT_ONCE];
    /* Main event loop */
    for (;;) {
	int nfds;
	if (quotas_are_full) {
	    dpf("quotas are full, so waiting as long as we want\n");
	    nfds = epoll_wait(kdpfd, events, MAX_EPOLL_EVENTS_AT_ONCE, -1);
	} else {
	    nfds = epoll_wait(kdpfd, events, MAX_EPOLL_EVENTS_AT_ONCE, timetick);
	}

	{
	    struct timeval time;
	    int milliseconds;
	    gettimeofday(&time, NULL);

	    milliseconds = (time.tv_sec - time_last.tv_sec)*1000 + (time.tv_usec - time_last.tv_usec)/1000;

	    if(milliseconds) {
		memcpy(&time_last, &time, sizeof time);
		bump_quotas(milliseconds);
	    }

	    millisecond_accumulator_for_update_rates += milliseconds;

	    if(millisecond_accumulator_for_update_rates >= 1000) {
		update_rates(millisecond_accumulator_for_update_rates);
		millisecond_accumulator_for_update_rates=0;
	    }

	}



	if (nfds == -1) {
	    if (errno == EAGAIN || errno == EINTR) {
		continue;
	    }
	    perror("epoll_pwait");
	    exit(EXIT_FAILURE);
	}

	int n;
	for (n = 0; n < nfds; ++n) {
	    if (events[n].data.fd == ss) {

		process_accept(ss);

	    } else if (events[n].data.fd == 0) {
		
                process_stdin();

	    } else { /* Handling the sends and recvs here */

		int fd = events[n].data.fd;
		int ev = events[n].events;

		if(fd < 0 || fd >= MAXFD) {
		    fprintf(stderr, "BAD FD %d\n", fd);
		    continue;
		}

                if (fdinfo[fd].status=='.') {
		    continue; /* happens when fails to connect */
		}

		if (ev & EPOLLOUT) {
		    dpf("%d becomes ready for writing\n", fd);
		    fdinfo[fd].writeready = 1;
		    fdinfo[fd].we_should_epoll_for_writes=0; /* as it is one shot event */
                    epoll_update(fd);

		    fdinfo[fdinfo[fd].peerfd].we_should_epoll_for_reads = 1;
                    epoll_update(fdinfo[fd].peerfd);
		}           
		if (ev & EPOLLIN) {
		    dpf("%d becomes ready for reading\n", fd);
		    fdinfo[fd].readready = 1;
		    fdinfo[fd].we_should_epoll_for_reads=0; /* as it is one shot event */
		    epoll_update(fd);
		}
		if (ev & (EPOLLERR|EPOLLHUP) ) {
		    dpf("%d HUP or ERR\n", fd);
		    dpf("    %d and %d are to be closed\n", fd, fdinfo[fd].peerfd);
		    close_fd(fd);
		}

		if(fdinfo[fd].readready && 
			fdinfo[fdinfo[fd].peerfd].writeready && 
			fdinfo[fdinfo[fd].peerfd].debt==0 && 
			fdinfo[fd].current_quota > 0 &&
			(fdinfo[fd].status=='|' || fdinfo[fd].status=='r') ) {

		    process_read(fd);

		}
		
		if(fdinfo[fd].writeready && 
			fdinfo[fd].debt>0 && 
			fdinfo[fd].buff!=NULL && 
			(fdinfo[fd].status=='|' || fdinfo[fd].status=='s') ) {

		    process_debt(fd);
		}
	    }
	} 
    }

    fprintf(stderr, "Probably abnormal termination\n");
}

