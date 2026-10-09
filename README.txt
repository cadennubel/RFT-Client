*************************************************************************
Include your name, course number (471 or 560) and any build notes.
*************************************************************************

Caden Nubel CSCI 560

Use port number: 12566 I found -t 1500 works best.


Window Size, Timeout, and Throughput (CSCI 560)
-----------------------------------------------
Test setup: 20KB file, server on isengard with 3% corruption
One run per setting.

Varying window size (timeout fixed at 1500 ms):

  Window   Time (ms)   Throughput (bytes/s)
    2        17347          1416
    4        14335          1714
    6        12833          1914
    8        12837          1914
   10        15843          1551

Varying timeout (window fixed at 8):

  Timeout (ms)   Time (ms)   Throughput (bytes/s)
      10          259166            95
      50          239065           103
     100          217724           113
     250          146589           168
     500           18836          1304
    1000           10825          2269
    1250            9841          2496
    1500           18924          1298

Discussion:
Throughput rose as the window grew from 2 to 6, leveled off at 8, and dropped
at 10. The rise and plateau match the textbook: pipelining helps until the
pipe is full. The drop at 10 also fits the book's point that Go-Back-N resends
the whole window after one error, so bigger windows waste more work.

Timeout mattered far more. Timeouts of 10-250 ms fired before ACKs could return which gave a warning about not receiving the expected number. The sender
resent the window repeatedly, each duplicate cost the server another time delay,
and throughput collapsed to under 170 bytes/s. Timeouts of 1000-1250 ms were
best, reaching 2496 bytes/s, close to the ceiling. At 1500 ms throughput fell
again because the sender waited longer to recover from each real corruption.
This matches the book's guidance that a timeout should be just above the
round-trip time: too short causes needless resends, too long slows recovery.
