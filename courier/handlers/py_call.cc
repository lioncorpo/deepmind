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

#include "courier/handlers/py_call.h"

#include <pybind11/pybind11.h>

#include <memory>
#include <string>

#include "courier/platform/default/py_utils.h"
#include "absl/base/casts.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "courier/handlers/interface.h"
#include "courier/platform/logging.h"
#include "courier/platform/status_macros.h"
#include "courier/platform/tensor_conversion.h"
#include "courier/serialization/py_serialize.h"
#include "courier/serialization/serialization.pb.h"

namespace courier {
namespace {

absl::StatusCode PythonExceptionErrorCode() {
  if (!PyErr_Occurred()) {
    return absl::StatusCode::kUnknown;
  }
  if (PyErr_ExceptionMatches(PyExc_ValueError) ||
      PyErr_ExceptionMatches(PyExc_TypeError)) {
    return absl::StatusCode::kInvalidArgument;
  }
  if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
    return absl::StatusCode::kOutOfRange;
  }
  if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
    return absl::StatusCode::kResourceExhausted;
  }
  if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
    return absl::StatusCode::kUnimplemented;
  }
  if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
    return absl::StatusCode::kAborted;
  }
  if (PyErr_ExceptionMatches(PyExc_SystemError) ||
      PyErr_ExceptionMatches(PyExc_SyntaxError)) {
    return absl::StatusCode::kInternal;
  }
  if (PyErr_ExceptionMatches(PyExc_LookupError)) {
    return absl::StatusCode::kNotFound;
  }
  return absl::StatusCode::kUnknown;
}

class PyCallHandler : public HandlerInterface {
 public:
  explicit PyCallHandler(PyObject* py_func) : py_func_(py_func) {
    Py_INCREF(py_func_);
  }

  ~PyCallHandler() override {
    pybind11::gil_scoped_acquire gil;
    Py_DECREF(py_func_);
  }

  absl::StatusOr<courier::CallResult> Call(
      absl::string_view endpoint,
      const courier::CallArguments& arguments) override {
    // Converting TensorProto to Tensor does not require the GIL so we perform
    // this (potentially slow) conversion before acquiring the GIL.
    COURIER_ASSIGN_OR_RETURN(auto lookup, CreateTensorLookup(arguments));

    pybind11::gil_scoped_acquire gil;

    // Deserialize args.
    courier::SafePyObjectPtr py_args(PyTuple_New(arguments.args_size()));
    for (int i = 0; i < arguments.args_size(); i++) {
      COURIER_ASSIGN_OR_RETURN(courier::SafePyObjectPtr py_arg,
                               DeserializePyObject(arguments.args(i), lookup));
      PyTuple_SET_ITEM(py_args.get(), i, py_arg.release());
    }

    // Deserialize kwargs.
    courier::SafePyObjectPtr py_kwargs(PyDict_New());
    for (const auto& pair : arguments.kwargs()) {
      COURIER_ASSIGN_OR_RETURN(courier::SafePyObjectPtr py_value,
                               DeserializePyObject(pair.second, lookup));
      PyDict_SetItemString(py_kwargs.get(), pair.first.data(), py_value.get());
    }

    courier::SafePyObjectPtr py_result(
        PyObject_Call(py_func_, py_args.get(), py_kwargs.get()));

    if (py_result) {
      courier::CallResult result;
      COURIER_RETURN_IF_ERROR(
          SerializePyObject(py_result.get(), result.mutable_result()));
      return result;
    } else {
      std::string error_prefix = "Python exception was raised on the server";
      absl::StatusCode status_code = PythonExceptionErrorCode();
      std::string exception;
      if (PythonUtils::FetchPendingException(&exception)) {
        std::string error_message =
            absl::StrCat(error_prefix, ":\n", exception);
        COURIER_LOG(COURIER_ERROR) << error_message;
        return absl::Status(status_code, error_message);
      }
      return absl::InternalError(absl::StrCat(
          error_prefix, " but the exception message could not be caught.   "));
    }
  }

 private:
  PyObject* py_func_;
};

}  // namespace

std::unique_ptr<HandlerInterface> BuildPyCallHandler(PyObject* py_func) {
  return absl::make_unique<PyCallHandler>(py_func);
}

}  // namespace courier
