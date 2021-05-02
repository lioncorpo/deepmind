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

"""Python server bindings for Courier RPCs.

Example usage:
server = courier.Server('my_server')
server.Bind('my_function', lambda a, b: a + b)
server.Start()

client = courier.Client('my_server')
result = client.my_function(4, 7)  # 11, evaluated on the server.
"""

from typing import Optional


from courier.handlers.python import pybind
from courier.python import router
from courier.python import server
import portpicker
from six.moves import map
import tensorflow.compat.v1 as tf
import tree as nest



class Server:
  """Server class for hosting Courier RPCs.

  This provides a convenience wrapper around the CLIF bindings. The thread pool
  size determines how many method handlers can be executed concurrently.

  Server start and termination. No RPCs are served before a call of Start is
  entered or after a call of Join or Stop has returned. A server may be started
  at most once. The functions Stop and Join may block if they need to wait for
  a concurrent Start to complete.
  """

  def __init__(
      self,
      name: Optional[str] = None,
      port: Optional[int] = None,
      thread_pool_size: int = 16,
  ):
    if port is None:
      port = portpicker.pick_unused_port()
    self._port = port
    self._thread_pool_size = thread_pool_size
    self._router = router.Router()
    self._server = None

  @property
  def port(self) -> int:
    return self._port

  @property
  def address(self) -> str:
    return f'localhost:{self._port}'

  def Bind(self, method_name: str, py_func):
    self._router.Bind(method_name, pybind.BuildPyCallHandler(py_func))

  def BindCacher(self,
                 target_server_address,
                 poll_interval,
                 stale_after,
                 max_cached_endpoints=16):
    """Binds a Cacher handler to all endpoints of this server.

    Cacher forwards calls to a target server and caches the response. The cached
    response is used for serving incoming client calls. The endpoint on the
    target server is repeatedly polled to refresh the cache. If the call request
    arguments are not empty, the order in which the unnamed arguments are given
    is important for resolving the cached endpoint. Currently supported argument
    types are int, bool, byte, string, or lists/structs/maps of these.

    Args:
      target_server_address: Address of the Courier server to forward to. Can be
        a Courier server name.
      poll_interval: [timedelta] Interval in which to refresh the cache.
      stale_after: [timedelta] Specifies a timeout after which an item in the
        cache is considered stale and will no longer be served to clients
      max_cached_endpoints: [int] Number of simultaneous cached endpoints that
        the Cacher could serve.
    """
    self._router.Bind(
        '*',
        pybind.BuildCacherHandler(target_server_address, poll_interval,
                                  stale_after, max_cached_endpoints))

  def Join(self):
    if not self._server:
      raise ValueError('Server not started')
    self._server.Join()

  def Start(self):
    """Starts the Courier server."""
    if self._server:
      raise ValueError('Server already started')


    self._server = server.BuildAndStart(self._router, self._port,
                                        self._thread_pool_size)

  def Stop(self):
    """Stops the Courier server."""
    if not self._server:
      raise ValueError('Server not started yet')
    self._server.Stop()

  def Unbind(self, method_name):
    self._router.Unbind(method_name)

  @property
  def has_started(self):
    """Returns True if the method `Start` has already been called.

    This method is not thread safe with regards to Start().
    """
    return self._server is not None


