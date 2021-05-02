# Lint as: python3
# Copyright 2020 DeepMind Technologies Limited. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Launches a Launchpad program using multiple processes."""

import subprocess
import threading
from typing import Any, Mapping

from launchpad import context
from launchpad import program as lp_program
from launchpad.launch import signal_handling
from launchpad.launch.run_locally import run_locally



def launch(program: lp_program.Program,
           local_resources: Mapping[str, Any] = None,
           terminal: str = 'gnome-terminal'):
  """Launches a program using multiple processes."""
  # Set up the launch context (launch type & launch config) for all nodes
  local_resources = local_resources or {}
  for label, nodes in program.groups.items():
    launch_config = local_resources.get(label, None)
    for node in nodes:
      node._launch_context.initialize(  
          context.LaunchType.LOCAL_MULTI_PROCESSING,
          launch_config=launch_config)

  # Notify the input handles
  for label, nodes in program.groups.items():
    for node in nodes:
      for handle in node._input_handles:  
        handle.connect(node, label)

  # Bind addresses
  for node in program.get_all_nodes():
    node.bind_addresses()

  commands = []
  for label, nodes in program.groups.items():
    # to_executables() is a static method, so we can call it from any of the
    # nodes in this group.
    
    # pytype: disable=wrong-arg-count
    commands.extend(nodes[0].to_executables(nodes, label,
                                            nodes[0]._launch_context))
    
    # pytype: enable=wrong-arg-count

  signal_handling.exit_gracefully_on_sigint()
  signal_handling.exit_gracefully_on_sigquit()

  run_locally.run_commands_locally(commands, terminal)

  # The following prevents the main process from exiting, while the child
  # processes are still running, but still allow post-launching actions
  threading.Thread(target=subprocess.Popen(['sleep', 'infinity']).wait).start()
