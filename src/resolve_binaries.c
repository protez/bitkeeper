/*
 * resolve_binary.c - resolver for content conflicts for binaries
 */
#include "resolve.h"

b_help(resolve *rs)
{
	int	i;

	fprintf(stderr,
"---------------------------------------------------------------------------\n\
Binary: %s\n\n\
New work has been modified locally and remotely and must be merged.\n\
Because this is a binary, you have to choose either the local or remote.\n\n\
GCA:    %s\n\
Local:  %s\n\
Remote: %s\n\
---------------------------------------------------------------------------\n",
	    rs->d->pathname, rs->revs->gca, rs->revs->local, rs->revs->remote);
	fprintf(stderr, "Commands are:\n\n");
	for (i = 0; rs->funcs[i].spec; i++) {
		fprintf(stderr, "  %-4s - %s\n", 
		    rs->funcs[i].spec, rs->funcs[i].help);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Typical command sequence: 'e' 'C';\n");
	fprintf(stderr, "Difficult merges may be helped by 'p'.\n");
	fprintf(stderr, "\n");
	return (0);
}

int
b_explain(resolve *rs)
{
	system("bk help merge-binaries");
	return (0);
}

int
b_commit(resolve *rs)
{
	if (rs->opts->debug) fprintf(stderr, "commit(%s)\n", rs->s->gfile);

	unless (exists(rs->s->gfile) && writable(rs->s->gfile)) {
		fprintf(stderr, "%s has not been merged\n", rs->s->gfile);
		return (0);
	}

	/*
	 * If in text only mode, then check in the file now.
	 * Otherwise, leave it for citool.
	 */
	if (rs->opts->textOnly) {
		unless (sccs_hasDiffs(rs->s, 0, 0)) {
			do_delta(rs->opts, rs->s, SCCS_MERGE);
		} else {
			do_delta(rs->opts, rs->s, 0);
		}
	}
	rs->opts->resolved++;
	return (1);
}

int
b_ascii(resolve *rs)
{
	extern	rfuncs	c_funcs[];

	return (resolve_loop("resolve_contents", rs, c_funcs));
}

/* An alias for !cp $BK_LOCAL $BK_MERGE */
int
b_ul(resolve *rs)
{
	names	*n = rs->tnames;

	unless (sys("cp", "-f", n->local, rs->s->gfile, SYS)) {
		return (b_commit(rs));
	}
	return (0);
}

/* An alias for !cp $BK_REMOTE $BK_MERGE */
int
b_ur(resolve *rs)
{
	names	*n = rs->tnames;

	unless (sys("cp", "-f", n->remote, rs->s->gfile, SYS)) {
		return (b_commit(rs));
	}
	return (0);
}

rfuncs	b_funcs[] = {
    { "?", "help", "print this help", b_help },
    { "!", "shell", "escape to an interactive shell", c_shell },
    { "a", "abort", "abort the patch, DISCARDING all merges", res_abort },
    { "cl", "clear", "clear the screen", res_clear },
    { "C", "commit", "commit the merged file", b_commit },
    { "h", "history", "revision history of all changes", res_h },
    { "hl", "hist local", "revision history of the local changes", res_hl },
    { "hr", "hist remote", "revision history of the remote changes", res_hr },
    { "H", "helptool", "show merge help in helptool", c_helptool },
    { "p", "revtool", "graphical picture of the file history", c_revtool },
    { "q", "quit", "immediately exit resolve", c_quit },
    { "t", "text", "go to the text file resolver", b_ascii },
    { "ul", "use local", "use the local version of the file", b_ul },
    { "ur", "use remote", "use the remote version of the file", b_ur },
    { "x", "explain", "explain the choices", b_explain },
    { 0, 0, 0, 0 }
};

/*
 * Given an SCCS file, resolve the contents.
 * Set up the list of temp files,
 * get the various versions into the temp files,
 * do the resolve,
 * and then clean them up.
 */
int
resolve_binary(resolve *rs)
{
	names	*n = calloc(1, sizeof(*n));
	delta	*d;
	char	*nm = basenm(rs->s->gfile);
	int	ret;
	char	buf[MAXPATH];

	d = sccs_getrev(rs->s, rs->revs->local, 0, 0);
	assert(d);
	sprintf(buf, "BitKeeper/tmp/%s_%s@%s", nm, d->user, d->rev);
	n->local = strdup(buf);
	d = sccs_getrev(rs->s, rs->revs->gca, 0, 0);
	assert(d);
	sprintf(buf, "BitKeeper/tmp/%s_%s@%s", nm, d->user, d->rev);
	n->gca = strdup(buf);
	d = sccs_getrev(rs->s, rs->revs->remote, 0, 0);
	assert(d);
	sprintf(buf, "BitKeeper/tmp/%s_%s@%s", nm, d->user, d->rev);
	n->remote = strdup(buf);
	rs->tnames = n;
	rs->prompt = rs->s->gfile;
	rs->res_contents = 1;
	if (get_revs(rs, n)) {
		rs->opts->errors = 1;
		freenames(n, 1);
		return (-1);
	}
	unless (IS_LOCKED(rs->s)) {
		if (edit(rs)) return (-1);
	}
	if (sameFiles(n->local, n->remote)) {
		automerge(rs, n);
		ret = 1;
	} else {
		ret = resolve_loop("resolve_binaries", rs, b_funcs);
	}
	unlink(n->local);
	unlink(n->gca);
	unlink(n->remote);
	freenames(n, 1);
	return (ret);
}