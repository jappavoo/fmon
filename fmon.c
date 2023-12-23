#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <assert.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/timerfd.h>

const int MS_PER_NS  = 1000000;
const int SEC_PER_MS = 1000;

void usage(char *name)
{
  fprintf(stderr,
	  "USAGE: %s [-f ms] [-l n] [<file>...] \n" \
	  "   samples n lines from every ms milliseconds from the end of the files\n",
	  name);
}

struct Args {
  int freqms;
  int lines;
  int verbose;
} Args = {
  .freqms = 1000,
  .lines = 1,
  .verbose = 0
};

inline int verbose() { return Args.verbose; }

void
dumpArgs(FILE *f)
{
  fprintf(f, "Args: freqms:%d lines:%d verbose:%d\n",
	  Args.freqms, Args.lines, Args.verbose);
}

bool
processArgs(int argc, char **argv, int *i)
{
  char opt;
  while ((opt = getopt(argc, argv, "f:l:v")) != -1) {
    switch(opt) {
    case 'f':
      Args.freqms = atoll(optarg);
      break;
    case 'l':
      Args.lines = atoll(optarg);
      break;
    case 'v':
      Args.verbose++;
      break;
    default:
      usage(argv[0]);
      return false;
      }
  }
    
  if ((argc - optind) < 1) {
    usage(argv[0]);
    return false;
  }
  
  if (verbose()) {
    dumpArgs(stderr);
  }
  
  *i = optind;
  return true;
}

typedef struct {
  char *path;
  size_t cnt;
  int fd;
} src_t;

void
printSrc(src_t *s, FILE *f)
{
  assert(s);
  fprintf(f, "src:%p path:%s cnt:%ld fd:%d\n", s,
	  s->path, s->cnt, s->fd);
}

int main(int argc, char **argv)
{
  int i,num;
  src_t *src;
  struct itimerspec timerfreq;
  int tfd;
  uint64_t exp;
  
  if (!processArgs(argc,argv,&i)) return -1;

  num = argc-i;
  src = malloc(sizeof(src_t)*num);
  
  if (verbose()) {
    fprintf(stderr, "monitoring %d files\n", num);
  }

  
  for (int si=0; i<argc; i++,si++) {
    src[si].path = argv[i];
    src[si].cnt  = 0;
    src[si].fd   = open(src[si].path, O_RDONLY, 0);
    if (src[si].fd == -1) err(EXIT_FAILURE, "%s", src[si].path);
    if (verbose()) {
      fprintf(stderr, "%d: ",si);
      printSrc(&src[si],stderr);
    }
  }
  
  //  timerfreq.it_interval.tv_sec = 0;
  timerfreq.it_interval.tv_sec = Args.freqms / SEC_PER_MS;
  timerfreq.it_interval.tv_nsec = 0;
  timerfreq.it_value.tv_sec = Args.freqms / SEC_PER_MS;
  timerfreq.it_value.tv_nsec = 0;

  tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (tfd == -1) err(EXIT_FAILURE, "timerfd_create");

  if (timerfd_settime(tfd, 0, &timerfreq, NULL) == -1) {
    err(EXIT_FAILURE, "timerfd_settime");
  }

  while (1) {
    int s  = read(tfd, &exp, sizeof(uint64_t));
    if (s != sizeof(uint64_t))
      err(EXIT_FAILURE, "read");
    fprintf(stderr, "exp:%lu\n", exp);
    
  }
  return 0;
}
