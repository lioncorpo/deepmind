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

#ifndef COURIER_PLATFORM_GRPC_UTILS_H_
#define COURIER_PLATFORM_GRPC_UTILS_H_

#include <memory>

#include "grpcpp/security/credentials.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/support/channel_arguments.h"
#include "absl/strings/string_view.h"

namespace courier {

std::shared_ptr<grpc::ServerCredentials> MakeServerCredentials();

std::shared_ptr<grpc::ChannelCredentials> MakeChannelCredentials();

std::shared_ptr<grpc::ChannelInterface> CreateCustomGrpcChannel(
    absl::string_view target,
    const std::shared_ptr<grpc::ChannelCredentials>& credentials,
    const grpc::ChannelArguments& channel_arguments);

}  // namespace courier

#endif  // COURIER_PLATFORM_GRPC_UTILS_H_
