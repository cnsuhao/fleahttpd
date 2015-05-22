# Introduction #

FleaHttpd is an http daemon written from scratch in C. When working as a static file server, data show that under certain condition, speed for static file retrieving can be three times faster than Apache2.

Only minimal CGI functionalities are supported in the current version of FleaHttpd.

# How-to #

**To compile:**

gcc fleahttpd.c -Wall -O3 -o fleahttpd

**To run:**

./fleahttpd -p port -r wwwroot

wwwroot is the root directory of website, example:

fleahttpd -p 8080 -r /var/www/

**Configuration:**

1 If there's an index.html under wwwroot, it will be loaded when fleahttpd starts, otherwise a default index.html will be provided. A default 404 page is also available.

2 Change support\_cgi=0 to 1 to turn on CGI support.