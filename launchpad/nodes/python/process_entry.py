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

"""Entry of a PythonNode worker."""


import contextlib
import json
import sys

from absl import app
from absl import flags
from absl import logging
import cloudpickle
import six


FLAGS = flags.FLAGS

flags.DEFINE_integer(
    'lp_task_id', None, 'a list index deciding which '
    'worker to run. given a list of workers (obtained from the'
    ' data_file)')
flags.DEFINE_string('data_file', '', 'Pickle file location')
flags.DEFINE_string('flags_to_populate', '{}', '')

_FLAG_TYPE_MAPPING = {
    str: flags.DEFINE_string,
    six.text_type: flags.DEFINE_string,
    float: flags.DEFINE_float,
    int: flags.DEFINE_integer,
    bool: flags.DEFINE_boolean,
    list: flags.DEFINE_list,
}


def _populate_flags():
  """Populate flags that cannot be passed directly to this script."""
  FLAGS(sys.argv, known_only=True)

  flags_to_populate = json.loads(FLAGS.flags_to_populate)
  for name, value in flags_to_populate.items():
    value_type = type(value)
    if value_type in _FLAG_TYPE_MAPPING:
      flag_ctr = _FLAG_TYPE_MAPPING[value_type]
      logging.info('Defining flag %s with default value %s', name, value)
      flag_ctr(
          name,
          value,
          'This flag has been auto-generated.',
          allow_override=True)


def _get_task_id():
  """Returns current task's id."""
  return FLAGS.lp_task_id


def main(_):
  functions = cloudpickle.load(open(FLAGS.data_file, 'rb'))
  task_id = _get_task_id()
  with contextlib.suppress():  # no-op context manager
    functions[task_id]()


if __name__ == '__main__':
  _populate_flags()
  app.run(main)
