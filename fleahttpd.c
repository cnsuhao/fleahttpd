/* Author: Hui Chen <usa.chen[at]gmail[dot]com>
 *
 * FleaHttpd: A fast Httpd as small as a flea
 * Published under GNU Public License version 3
 *
 */
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ERROR   1
#define MAX_MSG 1000
#define MSG_LEN_LIMIT 4000
#define PAGE_LEN_LIMIT 10000000
#define DEFAULT_PAGE_LEN_LIMIT 1000000

inline int readline (int, char *, size_t, int);
inline void print_usage ();

int
main (int argc, char *argv[])
{

  int i, j, k, n;
  int p[2];
  FILE *ptr;
  int pid;
  int page_len = 0, default_page_len, page_404_len;
  int content_length;
  int use_default, use_404;
  int sd, newSd, server_port = 80;
  char server_port_c[20];
  socklen_t cliLen;
  struct sockaddr_in cliAddr, servAddr;
  char *pi, *pos;
  char filename[2560], script_name[2560], script_query[2560],
    line[MAX_MSG + 1];
  int script_query_len;
  char *message = malloc (MSG_LEN_LIMIT);;
  char *root_dir = NULL;
  int support_cgi = 0;
  int http_method;		//0 GET, 1 POST, 2 PUT, 3 others
  char *myenviron[100];
  extern char **environ;
  environ = myenviron;

  if (argc == 1)
    print_usage ();
  else
    {
      if ((argc - 1) % 2)
	print_usage ();
      i = 1;
      while (i < argc)
	{
	  if (argv[i][0] != '-')
	    print_usage ();
	  switch (argv[i][1])
	    {
	    case 'p':
	      if ((i + 1) >= argc)
		print_usage ();
	      if (!(k = atoi (argv[i + 1])))
		print_usage ();
	      server_port = k;
	      break;
	    case 'r':
	      if ((i + 1) >= argc)
		print_usage ();
	      root_dir = argv[i + 1];
	      break;
	    default:
	      print_usage ();
	    }
	  i += 2;
	}
      if (chdir (root_dir))
	printf ("Can't change to directory %s!\n", root_dir);
    }

  setenv ("SERVER_SOFTWARE", "FleaHttpd/0.0.1", 1);
  setenv ("GATEWAY_INTERFACE", "CGI/1.1", 1);
  setenv ("SERVER_PROTOCOL", "HTTP/1.1", 1);
  setenv ("SERVER_NAME", "localhost", 1);
  sprintf (server_port_c, "%d", server_port);
  setenv ("SERVER_PORT", server_port_c, 1);


  int content_type = 0;		//0 plain, 1 html, 2 jpeg, 3 png, 4 gif, 5 css, 6 bmp, 7 cgi
  char header_0[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n";
  char header_1[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
  char header_2[] =
    "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nConnection: close\r\n\r\n";
  char header_3[] =
    "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n";
  char header_4[] =
    "HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\nConnection: close\r\n\r\n";
  char header_5[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nConnection: close\r\n\r\n";
  char header_6[] =
    "HTTP/1.1 200 OK\r\nContent-Type: image/bmp\r\nConnection: close\r\n\r\n";
  char header_7[] = "HTTP/1.1 200 OK\r\n";
  char *header[] =
    { header_0, header_1, header_2, header_3, header_4, header_5, header_6,
    header_7
  };
  char header_notfound[] =
    "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
  int header_len_404 = strlen (header_notfound);
  int header_len[8];
  for (i = 0; i < 8; i++)
    header_len[i] = strlen (header[i]);

  char *default_page = malloc (DEFAULT_PAGE_LEN_LIMIT);
  char buildin_default_page[] = "<h2>FleaHttpd is online!</h2>";
  char *page_404 = malloc (DEFAULT_PAGE_LEN_LIMIT);
  char buildin_page_404[] = "<h2>404</h2>\nThe page cannot be found.";
  char *page = malloc (PAGE_LEN_LIMIT);
  FILE *fp, *fp1;
  if (!(fp = fopen ("index.html", "r")))
    {
      free (default_page);
      default_page = buildin_default_page;
      default_page_len = strlen (buildin_default_page);
    }
  else
    {
      memcpy (default_page, header[1], header_len[1]);
      default_page_len =
	header_len[1] + fread (default_page + header_len[1], 1,
			       DEFAULT_PAGE_LEN_LIMIT, fp);
      if (default_page_len == 0 || default_page_len >= DEFAULT_PAGE_LEN_LIMIT)
	{
	  printf ("Size of index.html is illegal! sizeof(file) = %d\n",
		  default_page_len);
	  return ERROR;
	}
      fclose (fp);
    }
  if (!(fp = fopen ("404.html", "r")))
    {
      free (page_404);
      page_404 = buildin_page_404;
      page_404_len = strlen (buildin_page_404);
    }
  else
    {
      memcpy (page_404, header_notfound, header_len_404);
      page_404_len =
	header_len_404 + fread (page_404 + header_len_404, 1,
				DEFAULT_PAGE_LEN_LIMIT, fp);
      if (page_404_len == 0 || page_404_len >= DEFAULT_PAGE_LEN_LIMIT)
	{
	  printf ("Size of 404.html is illegal! sizeof(file) = %d\n",
		  page_404_len);
	  return ERROR;
	}
      fclose (fp);
    }

  sd = socket (AF_INET, SOCK_STREAM, 0);
  if (sd < 0)
    {
      printf ("Can't open socket!\n");
      return ERROR;
    }
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servAddr.sin_port = htons (server_port);
  if (bind (sd, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
    {
      printf ("Can't bind port %d!\n", server_port);
      return ERROR;
    }
  listen (sd, 1000);

  while (1)
    {
      cliLen = sizeof (cliAddr);
      newSd = accept (sd, (struct sockaddr *) &cliAddr, &cliLen);
      if (newSd < 0)
	{
	  continue;
	}
      pos = message;
      content_length = 0;
      *filename = 0;
      use_default = 0;
      use_404 = 1;
      content_type = 0;
      http_method = 3;
      script_query[0] = 0;
      script_query_len = 0;
      while ((n = readline (newSd, line, MAX_MSG, 0)) > 0)
	{
	  if (line[n - 2] == '\r')
	    {
	      n = n - 2;
	      line[n] = 0;
	    }
	  if (n == 0)
	    break;

	  switch (*line)
	    {
	    case 'G':
	      if (line[1] == 'E')
		http_method = 0;
	    case 'P':
	      if (line[1] == 'O')
		http_method = 1;
	      else if (line[1] == 'U')
		http_method = 2;

	      for (pi = line + (http_method == 1 ? 5 : 4);
		   (*pi) == '.' && (*pi) == '/'; pi++);
	      pi++;
	      script_name[0] = '/';
	      for (i = 0; pi[i] != ' ' && pi[i] != '?'; i++)
		{
		  filename[i] = pi[i];
		  script_name[i + 1] = pi[i];
		}
	      if (!i)
		{
		  use_default = 1;
		  break;
		}
	      filename[i] = 0;
	      script_name[i + 1] = 0;
	      if (pi[i] == '?')
		{
		  for (i += 1; pi[i] != ' '; i += 1)
		    {
		      script_query[script_query_len] = pi[i];
		      script_query_len++;
		    }
		  script_query[script_query_len] = 0;
		}

	      for (j = i - 1;
		   j >= 0 && filename[j] != '/' && filename[j] != '.'; j--);
	      if (filename[j] == '.')
		{
		  j++;
		  if (filename[j] == 'h' && filename[j + 1] == 't'
		      && filename[j + 2] == 'm' && filename[j + 3] == 'l')
		    content_type = 1;
		  else if (filename[j] == 'H' && filename[j + 1] == 'T'
			   && filename[j + 2] == 'M'
			   && filename[j + 3] == 'L')
		    content_type = 1;
		  else if (filename[j] == 'j' && filename[j + 1] == 'p'
			   && filename[j + 2] == 'g')
		    content_type = 2;
		  else if (filename[j] == 'J' && filename[j + 1] == 'P'
			   && filename[j + 2] == 'G')
		    content_type = 2;
		  else if (filename[j] == 'j' && filename[j + 1] == 'p'
			   && filename[j + 2] == 'e'
			   && filename[j + 3] == 'g')
		    content_type = 2;
		  else if (filename[j] == 'J' && filename[j + 1] == 'P'
			   && filename[j + 2] == 'E'
			   && filename[j + 3] == 'G')
		    content_type = 2;
		  else if (filename[j] == 'p' && filename[j + 1] == 'n'
			   && filename[j + 2] == 'g')
		    content_type = 3;
		  else if (filename[j] == 'P' && filename[j + 1] == 'N'
			   && filename[j + 2] == 'G')
		    content_type = 3;
		  else if (filename[j] == 'g' && filename[j + 1] == 'i'
			   && filename[j + 2] == 'f')
		    content_type = 4;
		  else if (filename[j] == 'G' && filename[j + 1] == 'I'
			   && filename[j + 2] == 'F')
		    content_type = 4;
		  else if (filename[j] == 'c' && filename[j + 1] == 's'
			   && filename[j + 2] == 's')
		    content_type = 5;
		  else if (filename[j] == 'C' && filename[j + 1] == 'S'
			   && filename[j + 2] == 'S')
		    content_type = 5;
		  else if (filename[j] == 'b' && filename[j + 1] == 'm'
			   && filename[j + 2] == 'p')
		    content_type = 6;
		  else if (filename[j] == 'B' && filename[j + 1] == 'M'
			   && filename[j + 2] == 'P')
		    content_type = 6;
		  else if (filename[j] == 'c' && filename[j + 1] == 'g'
			   && filename[j + 2] == 'i')
		    content_type = 7;
		  else if (filename[j] == 'C' && filename[j + 1] == 'G'
			   && filename[j + 2] == 'I')
		    content_type = 7;
		}
	      use_default = 0;
	      use_404 = 0;

	      break;
	    default:
	      break;
	    }
	  if (http_method == 1 && *line == 'C' && line[7] == '-'
	      && line[8] == 'L')
	    content_length = atoi (line + 16);
	}

      if (http_method == 1 && content_length)
	{
	  n = readline (newSd, line, MAX_MSG, 1);
	  if (n)
	    {
	      n--;
	      if (n != content_length)
		{
		  close (newSd);
		  continue;
		}
	      line[n] = 0;
	      if (script_query_len)
		{
		  script_query[script_query_len] = '&';
		  script_query_len++;
		}
	      for (i = 0; i < n; i += 1)
		{
		  script_query[script_query_len] = line[i];
		  script_query_len++;
		}
	      script_query[script_query_len] = 0;
	    }
	}
      if (support_cgi && script_query_len)
	setenv ("QUERY_STRING", script_query, 1);
      else
	unsetenv ("QUERY_STRING");

      if (!use_default && http_method < 3)
	{
	  if (support_cgi && content_type == 7)	//CGI
	    {
	      if (pipe (p) < 0 || !(fp1 = fopen (filename, "r")))
		{
		  use_default = 0;
		  use_404 = 1;
		}
	      else
		{
		  fclose (fp1);
		  if ((pid = fork ()), pid == 0)
		    {
		      setenv ("REQUEST_METHOD", "GET", 1);
		      setenv ("SCRIPT_NAME", script_name, 1);
		      setenv ("REMOTE_ADDR", inet_ntoa (cliAddr.sin_addr), 1);
		      close (1);
		      dup (p[1]);
		      close (p[0]);
		      close (p[1]);
		      if (execl (filename, filename, NULL))
			exit (0);
		    }
		  close (p[1]);
		  if (!(ptr = fdopen (p[0], "r")))
		    {
		      use_default = 0;
		      use_404 = 1;
		    }
		  else
		    {
		      memcpy (page, header[content_type],
			      header_len[content_type]);
		      page_len =
			header_len[content_type] + fread (page +
							  header_len
							  [content_type], 1,
							  PAGE_LEN_LIMIT,
							  ptr);
		      fclose (ptr);
		      use_default = 0;
		      use_404 = 0;
		    }
		}
	    }
	  else
	    {
	      if (!(fp = fopen (filename, "r")))
		{
		  use_default = 0;
		  use_404 = 1;
		}
	      else
		{
		  memcpy (page, header[content_type],
			  header_len[content_type]);
		  page_len =
		    header_len[content_type] + fread (page +
						      header_len
						      [content_type], 1,
						      PAGE_LEN_LIMIT, fp);
		  if (page_len == 0 || page_len == PAGE_LEN_LIMIT)
		    {
		      use_default = 0;
		      use_404 = 1;
		    }
		  else
		    {
		      use_default = 0;
		      use_404 = 0;
		      fclose (fp);
		    }
		}
	    }
	}

      if (use_default)
	send (newSd, default_page, default_page_len, 0);
      else if (use_404)
	send (newSd, page_404, page_404_len, 0);
      else
	send (newSd, page, page_len, 0);
      close (newSd);
    }
  free (message);
  free (default_page);
  free (page);
  free (page_404);
  close (sd);

  return 0;
}

inline int
readline (int fd, char *bufptr, size_t len, int flush)
{
  char *bufx = bufptr;
  static char *bp;
  static int cnt = 0;
  static char b[MSG_LEN_LIMIT];
  char c;
  int r_l;

  if (flush && cnt > 0)
    {
      r_l = cnt > len ? len : cnt;
      memcpy (bufptr, bp, r_l);
      cnt -= r_l;
      return r_l;
    }

  while (--len > 0)
    {
      if (--cnt <= 0)
	{
	  cnt = recv (fd, b, sizeof (b), 0);
	  if (cnt < 0)
	    return -1;
	  if (cnt == 0)
	    return 0;
	  bp = b;
	}
      c = *bp++;
      *bufptr++ = c;
      if (c == '\n')
	{
	  *bufptr = '\0';
	  return bufptr - bufx;
	}
    }
  return -1;
}

inline void
print_usage ()
{
  printf
    ("Usage: \n\tfleahttpd -p port -r wwwroot\nExample: \n\tfleahttpd -p 80 -r /var/www/fleahttpd\n");
  exit (0);
}
