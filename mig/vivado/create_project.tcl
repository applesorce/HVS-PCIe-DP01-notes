# K7-160T-DP: MicroBlaze + MIG 7 Series (DDR3) project generator.
#
# Usage:
#   vivado -mode batch -source create_project.tcl [-tclargs <project_dir>]
#
# Default <project_dir> is ./build next to this script. The MIG configuration
# (mig_a_x16_gui_verified.prj) is embedded inside top_bd.tcl, so no other file
# is needed to recreate the block design.

set here     [file normalize [file dirname [info script]]]
set proj_dir [expr {[llength $argv] > 0 ? [file normalize [lindex $argv 0]] : "$here/build"}]

create_project k7_160t_dp_mig $proj_dir -part xc7k160tffg676-1 -force
set_property board_part {} [current_project]

# Block design "top": MicroBlaze, AXI UART Lite, AXI GPIO (calib status input,
# heartbeat LED output), Clocking Wizard (50MHz -> 400MHz/200MHz), MIG.
source $here/top_bd.tcl

set bd [get_files top.bd]
make_wrapper -files $bd -top -import
set_property top top_wrapper [current_fileset]

add_files -fileset constrs_1 -norecurse $here/constraints.xdc
update_compile_order -fileset sources_1

puts "CREATE_PROJECT_DONE: $proj_dir"
