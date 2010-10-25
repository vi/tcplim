

static void bump_quotas(int milliseconds) {
    quotas_are_full = 1;
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
	int rand =  (random()*1000LL)/RAND_MAX;
	dpf("    fd=%d quota=%d delta=%d delta2=%d rand=%d\n", fd, quota, delta, delta2, rand);
	if(delta2 >rand) {
	    dpf("    stochastically adding one byte\n");
	    delta+=1;
	}

	fdinfo[fd].current_quota = quota + delta;

	if(fdinfo[fd].current_quota >= fdinfo[fd].speed_limit * 2) {
	    fdinfo[fd].current_quota = fdinfo[fd].speed_limit * 2;
	} else {
	    quotas_are_full = 0;
	    memcpy(&fdinfo[fd].last_quota_bump_time, &time_last, sizeof time_last);
	    dpf("    quotas are not full\n");
	}

	if (quota==0 && fdinfo[fd].current_quota!=0) {
	    epoll_update(fd);
	}

    }
}
