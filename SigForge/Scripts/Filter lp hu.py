DataIn(DATASERIES0, datavartr, 12 13); 

CreateFilter(Filter1, butt hp, 2 1000, 0.1, 0, 0);
Filter(datavartr, Filter1);
IProd(datavartr, -1);

Copy(datavartr2, datavartr);

CreateFilter(Filter2, butt lp, 2 1000, 45, 0, 0);
Filter(datavartr2, Filter2);

DisplayData(datavartr);
DisplayData(datavartr2);

DataAq(8, COM11, 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000 1000, 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1, 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15, 200, 4, FileName04_1000.bdf, orig_signal);