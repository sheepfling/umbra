#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/auth/HLAnoCredentials.h>

#include <pybind11/pybind11.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace py = pybind11;
namespace rti = rti1516_2025;

namespace {

PyObject* native_rti_error_type = nullptr;

std::string utf8(std::wstring const& value) {
  return py::cast(value).cast<std::string>();
}

std::wstring wide(std::string const& value) {
  return py::str(value).cast<std::wstring>();
}

class PythonFederateAmbassador final : public rti::NullFederateAmbassador {
 public:
  explicit PythonFederateAmbassador(py::object callback_target)
      : callback_target_(std::move(callback_target)) {}

  void connectionLost(std::wstring const& fault_description) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("connectionLost")(utf8(fault_description));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python connectionLost callback failed");
    }
  }

  void reportFederationExecutions(
      rti::FederationExecutionInformationVector const& report) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const information_type = api.attr("FederationExecutionInformation");
      auto const information_set_type = api.attr("FederationExecutionInformationSet");
      py::list records;
      for (auto const& information : report) {
        records.append(information_type(
            utf8(information.federationExecutionName),
            utf8(information.logicalTimeImplementationName)));
      }
      callback_target_.attr("reportFederationExecutions")(information_set_type(records));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python reportFederationExecutions callback failed");
    }
  }

 private:
  py::object callback_target_;
};

rti::CallbackModel callback_model_from(std::string const& value) {
  if (value == "immediate") {
    return rti::HLA_IMMEDIATE;
  }
  if (value == "evoked") {
    return rti::HLA_EVOKED;
  }
  throw rti::UnsupportedCallbackModel(L"callback model must be 'immediate' or 'evoked'");
}

std::string additional_settings_result_from(rti::AdditionalSettingsResultCode code) {
  switch (code) {
    case rti::SETTINGS_IGNORED:
      return "ignored";
    case rti::SETTINGS_FAILED_TO_PARSE:
      return "failed_to_parse";
    case rti::SETTINGS_APPLIED:
      return "applied";
  }
  throw std::logic_error("unknown AdditionalSettingsResultCode");
}

struct NativeConfigurationResult {
  bool configuration_used;
  bool address_used;
  std::string additional_settings_result;
  std::string message;
};

class NativeAmbassador {
 public:
  NativeAmbassador() : ambassador_(rti::RTIambassadorFactory().createRTIambassador()) {}

  NativeConfigurationResult connect(
      py::object callback_target,
      std::string const& callback_model,
      std::string const& configuration_name,
      std::string const& rti_address,
      std::string const& additional_settings,
      bool has_configuration,
      bool has_no_credentials) {
    auto callback = std::make_unique<PythonFederateAmbassador>(std::move(callback_target));
    auto const model = callback_model_from(callback_model);
    rti::ConfigurationResult result;
    if (has_configuration) {
      auto configuration = rti::RtiConfiguration::createConfiguration()
                               .withConfigurationName(wide(configuration_name))
                               .withRtiAddress(wide(rti_address))
                               .withAdditionalSettings(wide(additional_settings));
      if (has_no_credentials) {
        result = ambassador_->connect(*callback, model, configuration, rti::HLAnoCredentials());
      } else {
        result = ambassador_->connect(*callback, model, configuration);
      }
    } else if (has_no_credentials) {
      result = ambassador_->connect(*callback, model, rti::HLAnoCredentials());
    } else {
      result = ambassador_->connect(*callback, model);
    }
    federate_ambassador_ = std::move(callback);
    return {
        result.configurationUsed,
        result.addressUsed,
        additional_settings_result_from(result.additionalSettingsResult),
        utf8(result.message),
    };
  }

  void disconnect() {
    ambassador_->disconnect();
    federate_ambassador_.reset();
  }

  bool evoke_callback(double approximate_minimum_time_seconds) {
    return ambassador_->evokeCallback(approximate_minimum_time_seconds);
  }

  bool evoke_multiple_callbacks(
      double approximate_minimum_time_seconds,
      double approximate_maximum_time_seconds) {
    return ambassador_->evokeMultipleCallbacks(
        approximate_minimum_time_seconds, approximate_maximum_time_seconds);
  }

  void enable_callbacks() { ambassador_->enableCallbacks(); }
  void disable_callbacks() { ambassador_->disableCallbacks(); }
  void list_federation_executions() { ambassador_->listFederationExecutions(); }
  void create_federation_execution(
      std::string const& federation_name,
      std::string const& fom_module,
      std::string const& logical_time_implementation_name) {
    ambassador_->createFederationExecution(
        wide(federation_name), wide(fom_module), wide(logical_time_implementation_name));
  }
  void destroy_federation_execution(std::string const& federation_name) {
    ambassador_->destroyFederationExecution(wide(federation_name));
  }

 private:
  std::unique_ptr<rti::RTIambassador> ambassador_;
  std::unique_ptr<PythonFederateAmbassador> federate_ambassador_;
};

}  // namespace

PYBIND11_MODULE(_native, module) {
  module.doc() = "Narrow pybind11 bridge to the Umbra C++ RTI foundation.";
  native_rti_error_type = PyErr_NewException(
      "umbra._native.rti1516_2025._native.NativeRtiError", PyExc_RuntimeError, nullptr);
  module.add_object("NativeRtiError", py::reinterpret_steal<py::object>(native_rti_error_type));

  py::register_exception_translator([](std::exception_ptr exception) {
    try {
      if (exception) {
        std::rethrow_exception(exception);
      }
    } catch (rti::Exception const& error) {
      auto const message = utf8(error.name()) + ": " + utf8(error.what());
      PyErr_SetString(native_rti_error_type, message.c_str());
    }
  });

  py::class_<NativeConfigurationResult>(module, "NativeConfigurationResult")
      .def_readonly("configuration_used", &NativeConfigurationResult::configuration_used)
      .def_readonly("address_used", &NativeConfigurationResult::address_used)
      .def_readonly("additional_settings_result", &NativeConfigurationResult::additional_settings_result)
      .def_readonly("message", &NativeConfigurationResult::message);

  py::class_<NativeAmbassador>(module, "NativeAmbassador")
      .def(py::init<>())
      .def(
          "connect",
          &NativeAmbassador::connect,
          py::arg("federate_ambassador"),
          py::arg("callback_model"),
          py::arg("configuration_name") = "",
          py::arg("rti_address") = "",
          py::arg("additional_settings") = "",
          py::arg("has_configuration") = false,
          py::arg("has_no_credentials") = false)
      .def("disconnect", &NativeAmbassador::disconnect)
      .def("evoke_callback", &NativeAmbassador::evoke_callback)
      .def("evoke_multiple_callbacks", &NativeAmbassador::evoke_multiple_callbacks)
      .def("enable_callbacks", &NativeAmbassador::enable_callbacks)
      .def("disable_callbacks", &NativeAmbassador::disable_callbacks)
      .def("list_federation_executions", &NativeAmbassador::list_federation_executions)
      .def("create_federation_execution", &NativeAmbassador::create_federation_execution)
      .def("destroy_federation_execution", &NativeAmbassador::destroy_federation_execution);

  module.def("rti_name", [] { return utf8(rti::rtiName()); });
  module.def("rti_version", [] { return utf8(rti::rtiVersion()); });
}
