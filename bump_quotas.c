int cmp_fds(const void* en1, const void* en2) {
    const int *e1 = en1, *e2 = en2;
    if(e1==e2) return 0;
    if(fdinfo[*e1].nice < fdinfo[*e2].nice) return -1;
    if(fdinfo[*e1].nice > fdinfo[*e2].nice) return 1;
    if(fdinfo[*e1].last_quota_bump_time.tv_sec < fdinfo[*e2].last_quota_bump_time.tv_sec) return -1;
    if(fdinfo[*e1].last_quota_bump_time.tv_sec > fdinfo[*e2].last_quota_bump_time.tv_sec) return 1;
    if(fdinfo[*e1].last_quota_bump_time.tv_usec < fdinfo[*e2].last_quota_bump_time.tv_usec) return -1;
    if(fdinfo[*e1].last_quota_bump_time.tv_usec > fdinfo[*e2].last_quota_bump_time.tv_usec) return 1;
    return 0;
}

static void bump_quotas(int milliseconds) {
    quotas_are_full = 1;
    dpf("Bumping quotas for %d milliseconds\n", milliseconds);
    int fd,i;

    int quota_bump_queue[MAXFD];
    int n;

    for(fd=0,i=0; fd<MAXFD; ++fd) {
	if(fdinfo[fd].status == 0 || fdinfo[fd].status == '.') {
	    continue;
	}
	quota_bump_queue[i++] = fd;
    }
    n=i;

    qsort(&quota_bump_queue, n, sizeof quota_bump_queue[0], cmp_fds);

    for(i=0; i<n; ++i) {
	fd = quota_bump_queue[i];

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
