#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

static void send_fd(int fd, int target)
{
  char anc[CMSG_SPACE(sizeof(fd))];
  struct msghdr msg;
  struct cmsghdr *cmsg;
  struct iovec iov;
  int *fd_ptr, pid = getpid();

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  iov = ((struct iovec) { iov_base : &pid,
			  iov_len :  sizeof(pid) });
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_flags = 0;
  msg.msg_control = anc;
  msg.msg_controllen = sizeof(anc);

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

  fd_ptr = (int *) CMSG_DATA(cmsg);
  *fd_ptr = fd;

  msg.msg_controllen = cmsg->cmsg_len;

  if(sendmsg(target, &msg, 0) < 0){
    perror("sendmsg");
    exit(1);
  }
}

static int open_socket(char *name)
{
  struct sockaddr_un sock;
  int fd;

  if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
    perror("socket");
    exit(1);
  }

  sock.sun_family = AF_UNIX;
  memset(sock.sun_path, 0, sizeof(sock.sun_path));
  sprintf(&sock.sun_path[1], "%5d", getpid());

  if(bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0){
    perror("bind");
    exit(1);
  }

  snprintf(sock.sun_path, sizeof(sock.sun_path), "%s", name);
  if(connect(fd, (struct sockaddr *) &sock, sizeof(sock))){
    perror("connect");
    exit(1);
  }

  return(fd);
}

int main(int argc, char **argv)
{
  int fd;

  if((argc > 1) && !strcmp(argv[1], "-uml-socket")) fd = open_socket(argv[2]);
  else fd = 3;

  signal(SIGHUP, SIG_IGN);
  if(ioctl(0, TIOCNOTTY, 0) < 0)
    perror("TIOCNOTTY failed");
  send_fd(0, fd);
  pause();
  return(0);
}
