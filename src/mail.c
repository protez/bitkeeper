/*
 * Copyright (c) 2000, Andrew Chang
 */    
#include "bkd.h"

int
lconfig_main(int ac, char **av)
{
	char	from[MAXPATH], subject[MAXLINE], config_log[MAXPATH];
	char	url[] = BK_WEBMAIL_URL;
	char	*to = "config@openlogging.org";
	FILE	*f;
	int	pflag = 0, debug = 0, c, n, rc;
	remote	*r;
	MMAP	*m;

	while ((c = getopt(ac, av, "dp")) != -1) {
		switch (c) {
		    case 'd':	debug = 1; break;
		    case 'p':	pflag = 1; break;
		    default:	fprintf(stderr, "usage: bk _logConfig [-d]\n");
				return (1);
		}
	}

	if (sccs_cd2root(0, 0) == -1) return (1);
	if (pflag) {
		printf("Number of config logs pending: %d\n", logs_pending(1));
		return (0);
	}

	sprintf(from, "%s@%s", sccs_getuser(), sccs_gethost());
	sprintf(subject, "BitKeeper config: %s", package_name());

	gettemp(config_log, "config");
	unless (f = fopen(config_log, "wb")) return (1);
	fprintf(f, "To: %s\nFrom: %s\nSubject: %s\n\n", to, from, subject);
	status(0, f);
	config(f);
	fclose(f);

	/*
	 * WIN32 note: Win32 wish shell maps the console to a
	 * to a invisiable window, messages printed to tty will be invisiable.
	 * We therefore have to send it to stdout, which will be read and
	 * displayed by citool.
	 */
#ifndef	WIN32
	fclose(stdout); /* close stdout, so citool do'nt wait for us */
	usleep(0); /* release cpu, so citool can exit */
	fopen(DEV_TTY, "wb");
#endif
	r = remote_parse(url, 0);
	if (debug) r->trace = 1;
	assert(r);
	loadNetLib();
	http_connect(r, WEB_MAIL_CGI);
	r->isSocket = 1;
	m = mopen(config_log, "r");
	assert(m);
	rc = http_send(r, m->where, msize(m), 0, "webmail", WEB_MAIL_CGI);
	mclose(m);
	unless (rc) rc = get_ok(r, 0, 0);
	disconnect(r, 2);
	unlink(config_log);
	updLogMarker(1);
	return (rc);
}


int
mail_main(int ac, char **av)
{
	pid_t pid;
	int status;

	/*
	 * WIN32 note: Win32 wish shell maps the console to a
	 * to a invisiable window, messages printed to tty will be invisiable.
	 * We therefore have to send it to stdout, which will be read and
	 * displayed by citool.
	 */
#ifndef	WIN32
	fclose(stdout); /* close stdout, so citool do'nt wait for us */
	usleep(0); /* release cpu, so citool can exit */
	fopen(DEV_TTY, "wb");
#endif
	if (ac != 4) {
		printf("usage: bk mail mailbox subject file\n");
		return (1);
	}

	pid = mail(av[1], av[2], av[3]); /* send via email */
	if (pid != (pid_t) -1) waitpid(pid, &status, 0);
	unlink(av[3]);
	return (status);
}