clear
reset
set xlabel "input load (flit/cycle/node)"
set ylabel "packet latency (cycles)"
#set xrange [2000:2200]
set xrange [0:0.3]
set yrange [0:100]
#set ytics nomirror
set size 0.7,0.7
#set style line 1 lt 1 pt 5 ps 2
#set style data lines
#set pointsize 1
plot 'routing_BC.txt' using 1:2 title 'XY' with linespoints, \
     '' using 1:3 title 'YX' with linespoints, \
     '' using 1:4 title 'adaptive' with linespoints, \
     '' using 1:5 title 'oblivious' with linespoints

set term png small
set output "routing_BC.png"
replot
