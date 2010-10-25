
static void update_rates(int milliseconds) {
    int fd;
    for (fd=0; fd<MAXFD; ++fd) {
	fdinfo[fd].rate = (fdinfo[fd].total_read - fdinfo[fd].total_read_last) * 1000LL / milliseconds;
	fdinfo[fd].total_read_last = fdinfo[fd].total_read;
    }
}
