clear
reset
set xlabel "input load (flit/cycle/node)"
set ylabel "packet latency (cycles)"
#set xrange [2000:2200]
set xrange [0:0.8]
set yrange [0:100]
#set ytics nomirror
#set size 0.8,0.5
#set style line 1 lt 1 pt 5 ps 2
#set style data lines
#set pointsize 1
plot 'topology_BC.txt' using 1:2 title 'Mesh' with linespoints, \
     '' using 1:3 title 'Torus' with linespoints, \
     '' using 1:4 title 'ECube' with linespoints, \
     '' using 1:5 title 'Fat Tree' with linespoints, \
     '' using 1:6 title 'FB' with linespoints
set term png small
set output "topology_latency_BC.png"
replot
set term postscript eps enhanced color
set size 0.5,0.5
set output "topology_latency_BC.eps"
replot
