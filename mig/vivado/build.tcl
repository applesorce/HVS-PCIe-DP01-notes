# Synthesize + implement the project made by create_project.tcl, then write the
# .mmi file that updatemem needs to merge the MicroBlaze firmware into BRAM.
#
# Usage:
#   vivado -mode batch -source build.tcl [-tclargs <project_dir>]

set here     [file normalize [file dirname [info script]]]
set proj_dir [expr {[llength $argv] > 0 ? [file normalize [lindex $argv 0]] : "$here/build"}]

open_project $proj_dir/k7_160t_dp_mig.xpr

launch_runs synth_1 -jobs 4
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "synth_1 did not complete: [get_property STATUS [get_runs synth_1]]"
}

launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "impl_1 did not complete: [get_property STATUS [get_runs impl_1]]"
}

open_run impl_1
set impl_dir [get_property DIRECTORY [get_runs impl_1]]
write_mem_info -force $impl_dir/top_wrapper.mmi

puts "BUILD_DONE"
puts "bitstream: $impl_dir/top_wrapper.bit"
puts "mmi:       $impl_dir/top_wrapper.mmi"
