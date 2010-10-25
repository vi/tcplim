
static void bump_quotas(int milliseconds) {
    dpf("Bumping quotas for %d milliseconds\n", milliseconds);
    int fd;

    for(fd=0; fd<MAXFD; ++fd) {
	if(fdinfo[fd].status == 0 || fdinfo[fd].status == '.') {
	    continue;
	}

	int quota = fdinfo[fd].current_quota;
	int delta =  fdinfo[fd].speed_limit*milliseconds/1000;

	/* This is needed to handle very low speeds */
	int delta2 = fdinfo[fd].speed_limit*milliseconds%1000;
	if(delta2 > random()*1000/RAND_MAX) {
	    delta+=1;
	}

	fdinfo[fd].current_quota = quota + delta;

	if (quota==0 && fdinfo[fd].current_quota!=0) {
	    epoll_update(fd);
	}

    }
}
