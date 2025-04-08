open_project -reset sync_prj
set_top bds_capture
add_files bds_capture.cpp
add_files -tb tb.cpp

open_solution -reset "solution1" -flow_target vivado
set_part {xcvu9p-flga2104-2-i}
create_clock -period 10 -name default

set CSIM 1
set CSYNTH 1
set COSIM 1

if {$CSIM == 1} {
  puts "Starting Csim..."
  csim_design
}

if {$CSYNTH == 1} {
  puts "Starting Csynth..."
  csynth_design
}

if {$COSIM == 1} {
  puts "Starting Cosim..."
  config_cosim -trace_level all
  cosim_design -trace_level all
}


exit
