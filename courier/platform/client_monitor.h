// Copyright 2020 DeepMind Technologies Limited.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COURIER_PLATFORM_CLIENT_MONITOR_H_
#define COURIER_PLATFORM_CLIENT_MONITOR_H_

#include <memory>

#include "grpcpp/channel.h"

namespace courier {

class MonitoredCallScope {
 public:
  virtual ~MonitoredCallScope() {}
};

std::unique_ptr<MonitoredCallScope> BuildCallMonitor(
    grpc::ChannelInterface* channel, const std::string& method_name,
    const std::string& server_address);

void ClientCreation();

}  // namespace courier

#endif  // COURIER_PLATFORM_CLIENT_MONITOR_H_
