# NNG_homework
The program was developed under Debian Linux.

Compile:

g++ main.cpp -o main -l sqlite3

Run:

./main 2

The specification states, all input files match the track_*.csv reg. exp.; in the example above, track_2.csv will be used.

BE AWARE of the char. encoding; I had problems with the end line characters during coding.

Error codes:

1. Irreal speed;
2. Too far even from the closest point.

I've added a column decisec to table GPS_TRACK_POINTS; later it seemed to be unnecessary. 
