#!/usr/bin/python

import os;

####################################################################
exe_program = '~/noc_cpp/bin/noc'

load_increase = 0.02
load_start = 0.02
#load_start = 0.41
#load_end = 0.40 + 1.0E-9
load_end = 0.60 + 1.0E-9
# we add a very small number (1.0E-9) to overcome the floating point number problem in Python.

wkld_type = 'SYNTH'
#wkld_synth_spatials = ['UR', 'TP', 'BC', 'NN']
#wkld_synth_spatials = ['UR', 'TP', 'BC']
wkld_synth_spatials = ['TP']
wkld_synth_flits_per_pkt = 5

num_cores_in_dim = 4
# (topology, routing)
net_tuples = [
              ('mesh',    'xy',              'SAMQ'),
#              ('mesh',    'xy',              'DAMQ_P'),
#              ('mesh',    'min_adaptive_vc', 'SAMQ'),
#              ('mesh',    'min_adaptive_vc', 'DAMQ_P'),
             ]
net_num_cores = num_cores_in_dim * num_cores_in_dim

router_inbuf_depth = 4
router_vc = 2

link_width = 64

sim_warm_cycle = 20000
sim_end_cycle = 40000
####################################################################

for net_tuple in net_tuples:
  net_topology = net_tuple[0]
  net_routing = net_tuple[1]
  router_buffer = net_tuple[2]
  dir_name = net_topology + '_' + net_routing + '_' + router_buffer

  try:
    os.makedirs(dir_name)
  except:
    pass

  for wkld_synth_spatial in wkld_synth_spatials:
    load_cur = load_start
    while load_cur <= load_end:

      output_file_name = dir_name + '/p' + str(net_num_cores) + '_' + wkld_synth_spatial + '_load' + "%.2f" % load_cur + '.out'

      command_str = exe_program
      command_str += ' -wkld:type ' + wkld_type
      command_str += ' -wkld:synth:spatial ' + wkld_synth_spatial
      command_str += ' -wkld:synth:load ' + str(load_cur)
      command_str += ' -wkld:synth:flits_per_pkt ' + str(wkld_synth_flits_per_pkt)
      command_str += ' -net:topology ' + net_topology
      command_str += ' -net:core_num ' + str(net_num_cores)
      command_str += ' -net:routing ' + net_routing
      command_str += ' -router:inbuf_depth ' + str(router_inbuf_depth)
      command_str += ' -router:vc ' + str(router_vc)
      command_str += ' -router:buffer ' + router_buffer
      command_str += ' -router:power_model ' + 'orion_call'
      command_str += ' -link:width ' + str(link_width)
      command_str += ' -sim:warm_cycle ' + str(sim_warm_cycle)
      command_str += ' -sim:end_cycle ' + str(sim_end_cycle)
      command_str += ' -sim:progress ' + 'Y'
      command_str += ' -sim:progress_interval ' + '5000'
      command_str += ' -sim:outfile ' + output_file_name
      # command_str += ' &'

      # print command_str
      os.system(command_str)

      load_cur += load_increase
