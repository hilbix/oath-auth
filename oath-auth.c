#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <dlfcn.h>
#include <security/pam_appl.h>

static int		have;
static int		ok;
static const char	*lib = GOOGLE_PAM_LIB;
static const char	*sym = "pam_sm_authenticate";

static const char	* const HEX = "0123456789ABCDEF";

static void
vOOPS(int err, const char *s, va_list list)
{
  fprintf(stderr, "OOPS: ");
  if (err>=0)
    fprintf(stderr, "dynamic library %s:\n", lib);
  vfprintf(stderr, s, list);
  if (err>0)
    fprintf(stderr, ": %s", strerror(err));
  fprintf(stderr, "\n");
  exit(23);
  abort();
  for (;;) sleep(1);	/* really never return ever	*/
}

static void
OOPS(const char *s, ...)
{
  int		e = errno;
  va_list	list;

  va_start(list, s);
  vOOPS(e, s, list);
  va_end(list);	/* never reached	*/
}

static void *
checkdl(void *ptr, const char *explain, ...)
{
  const char	*err;
  va_list	list;

  if (ptr && (err = dlerror())==0)
    return ptr;

  va_start(list, explain);
  vOOPS(0, explain, list);
  va_end(list);	/* never reached	*/

  return NULL;
}

static int
pamok(int ret, const char *explain, ...)
{
  va_list	list;

  if (ret == PAM_SUCCESS)
    return 1;

  va_start(list, explain);
  vOOPS(0, explain, list);
  va_end(list);	/* never reached	*/

  return 0;
}

#define	DLSET(PTR)	(*(void **) (&(PTR)))	/* POSIX.1-2003/-2008 workaround	*/

static void
escape(FILE *out, const char *prefix, const char *msg)
{
  char	c;

  fprintf(out, "%s: ", prefix);
  for (; (c= *msg++)!=0; fputc(c, out))
    {
      if (isprint(c))
	continue;
      fputc('\\', out);
      switch (*msg)
	{
	case '\a':	c='a'; continue;
	case '\b':	c='b'; continue;
	case '\033':	c='e'; continue;
	case '\f':	c='f'; continue;
	case '\n':	c='n'; continue;
	case '\r':	c='r'; continue;
	case '\t':	c='t'; continue;
	case '\v':	c='v'; continue;
	}
      fputc('\\', out);
      fputc('x', out);
      fputc(HEX[(c>>4)&0xf], out);
      c	= HEX[c&0xf];
    }
}

struct google_auth_data
  {
    const char	*pw;
    int		use;
  };

static int
conv(int n, const struct pam_message **m, struct pam_response **_r, void *app)
{
  struct pam_response		*r;
  struct google_auth_data	*d = app;
  int				i, ret;

  r	= calloc(n, sizeof *r);
  if (!r)
    goto mem;

  if (n < 1 || n > 1 /*PAM_MAX_NUM_MSG*/)
    {
      OOPS("more than one PAM message is not supported");
      goto fail;
    }

  for (i=0; i<n; i++)
    {
      r[i].resp		= NULL;
      r[i].resp_retcode	= 0;
      switch (m[i]->msg_style)
	{
	case PAM_PROMPT_ECHO_OFF:
	case PAM_PROMPT_ECHO_ON:
	  d->use++;
	  r[i].resp	= strdup(d->pw);
	  if (!r[i].resp)
	    goto mem;
	  break;

	case PAM_ERROR_MSG:
	  escape(stderr, "error", m[i]->msg);
	  break;

	case PAM_TEXT_INFO:
	  escape(stderr, "info", m[i]->msg);
	  break;

	default:
	  goto fail;
	}
    }
  *_r	= r;
  return PAM_SUCCESS;

mem:
  ret = PAM_BUF_ERR;
  if (0)
fail:
  ret = PAM_CONV_ERR;

  *_r = 0;
  if (r)
    {
      for (i=n; --i>=0; )
        if (r[i].resp)
          {
            memset(r[i].resp, 0, strlen(r[i].resp));
            free(r[i].resp);
          }
      free(r);
    }
  return ret;
}

static int
issetarg(const char *s)
{
  return *s && strcmp(s, "-");
}

static struct google_auth_data	appdata;
static struct pam_conv		pamconv = { conv, &appdata };
static int 			(*fn)(pam_handle_t *pamh, int flags, int argc, const char **argv);

static void
run(const char *pw, int argc, const char **argv)
{
  int		flags, err;
  const char	*state, *user = getenv("USER");
  pam_handle_t	*pam;

  appdata.pw	= pw;
  appdata.use	= 0;

  errno		= 0;
#if 1
  flags		= PAM_SILENT;
#else
  flags		= 0;
#endif

  pamok(pam_start("oath-auth", user, &pamconv, &pam), "initializing PAM");
  err		= (*fn)(pam, flags, argc, argv);
  pamok(pam_end(pam, err), "closing PAM");

  switch (err)
    {
    default:				OOPS("function %s returns unknown code: %d", sym, err);
    case PAM_CRED_INSUFFICIENT:		OOPS("PAM_CRED_INSUFFICIENT: insufficient credentials");
    case PAM_USER_UNKNOWN:		OOPS("PAM_USER_UNKNOWN: user unknown: USER=%s", user);
    case PAM_MAXTRIES:			OOPS("PAM_MAXTRIES: max retries exceeded");
    case PAM_AUTHINFO_UNAVAIL:		OOPS("PAM_AUTHINFO_UNAVAIL: authentication information unavailable");
    case PAM_BAD_ITEM:			OOPS("PAM_BAD_ITEM: unknown item passed to pam_*_item(), probably not supported security module", lib);
    case PAM_SESSION_ERR:		/* I really do not know why this comes	*/
      state	= "err";
      ok	= 0;
      break;

    case PAM_AUTH_ERR:
      state	= "fail";
      ok	= 0;
      break;

    case PAM_SUCCESS:
      state	= "ok";
      have	= 1;
      break;
    }
   printf("%s %d %s\n", state, appdata.use, appdata.pw);
}

int
main(int argc, const char **argv)
{
  void		*dl;
  int		args;

  args = 2;
  if (argc>2)
    {
      if (issetarg(argv[2]))
        lib	= argv[2];
      args	= 3;
    }

  if (argc<2)
    OOPS("Usage: %s TOKEN [PAM.so [args]]\n"
#if 0
         "\tIf TOKEN is - it reads tokens from stdin.\n"
#endif
         "\tThis calls %s() in\n\t%s\n\twith the given args"
         , argv[0], sym, lib);

  dlerror();

#if 1
  dl		= checkdl(dlopen(lib, RTLD_NOW|RTLD_DEEPBIND), "load failed");
#else
  dl		= checkdl(dlmopen(LM_ID_NEWLM, lib, RTLD_NOW|RTLD_DEEPBIND), "load failed");
#endif
  DLSET(fn)	= checkdl(dlsym(dl, sym), "sym not found: %s", sym);

  have		= 0;
  ok		= 1;
  run(argv[1], argc-args, argv+args);

  dlclose(dl);

  return have && ok ? 0 : 1;
}

