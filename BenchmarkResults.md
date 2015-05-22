Benchmark results by Apache's ab tool.

# FleaHttpd #
```
Server Software:
Server Hostname:        limiao.net
Server Port:            8080

Document Path:          /
Document Length:        37103 bytes

Concurrency Level:      1
Time taken for tests:   1.606444 seconds
Complete requests:      10000
Failed requests:        0
Write errors:           0
Total transferred:      371470000 bytes
HTML transferred:       371030000 bytes
Requests per second:    6224.93 [#/sec] (mean)
Time per request:       0.161 [ms] (mean)
Time per request:       0.161 [ms] (mean, across all concurrent requests)
Transfer rate:          225817.40 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.0      0       0
Processing:     0    0   0.0      0       1
Waiting:        0    0   0.0      0       0
Total:          0    0   0.0      0       1
```

# Apache #
```
Server Software:        Apache
Server Hostname:        limiao.net
Server Port:            80

Document Path:          /gpl.html
Document Length:        37103 bytes

Concurrency Level:      1
Time taken for tests:   4.945393 seconds
Complete requests:      10000
Failed requests:        0
Write errors:           0
Total transferred:      373550000 bytes
HTML transferred:       371030000 bytes
Requests per second:    2022.08 [#/sec] (mean)
Time per request:       0.495 [ms] (mean)
Time per request:       0.495 [ms] (mean, across all concurrent requests)
Transfer rate:          73764.41 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       5
Processing:     0    0   0.8      0      14
Waiting:        0    0   0.7      0      14
Total:          0    0   0.8      0      14
```