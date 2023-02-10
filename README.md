This prog (fa_collect) collect the udp streams, one thread per device , 
push it to an additional thread, who mix the streams to a libera grouping protocol, 
which  feed the data to the  fa-archiver(Michael Abbott, Diamond Light Source Ltd.) 

Please note, there is no treatment if a configured device has no or lost connection.
Because no data avail, the pipe blocked,  the fa_archiver data stream stopped.
This will fixed  a.s.a.p..




