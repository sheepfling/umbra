#define main hla_rti_cpp_tck_original_main
#include "main.cpp"
#undef main

namespace {

constexpr char automaticResignDirectiveDeleteObjectsId[] =
    "cpp-tck.automatic-resign-directive-delete-objects";
constexpr char automaticResignDirectiveDeleteObjectsContractId[] =
    "cpp-tck.automatic-resign-directive-delete-objects-contract";
constexpr char publicHandleDecodingId[] = "cpp-tck.public-handle-decoding";
constexpr char publicHandleDecodingContractId[] =
    "cpp-tck.public-handle-decoding-contract";

void scenarioAutomaticResignDirectiveDeleteObjects(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic resignation on connection loss requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "automatic-resign-directive-delete-objects");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const lostClass = lost.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivorClass = survivor.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const lostAttribute = lost.rtiAmbassador().getAttributeHandle(
      lostClass,
      options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      survivorClass.isValid() && lostClass.isValid() &&
          survivorAttribute.isValid() && lostAttribute.isValid(),
      "automatic resign directive lookup returned an invalid standard handle");

  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  lost.rtiAmbassador().publishObjectClassAttributes(
      lostClass,
      lostAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      object.isValid(),
      "automatic resign directive registration returned an invalid object handle");
  auto const objectName = lost.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "automatic resign directive registration returned an empty object name");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic resign directive object discovery");
  lost.rtiAmbassador().setAutomaticResignDirective(rti::DELETE_OBJECTS);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() == rti::DELETE_OBJECTS,
      "automatic resign directive did not round-trip before connection loss");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "automatic resign directive discovery did not preserve object-name lookup");

  survivor.recorder().clearRemovals();
  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic resign directive delete-object cleanup");
  waitFor(
      survivor,
      [&] {
        return survivor.recorder().hasRemoval(
            object,
            {},
            lost.federateHandle());
      },
      options,
      "automatic resign directive delete-object removal");

  auto const removals = survivor.recorder().removals();
  require(
      removals.size() == 1U,
      "automatic resign directive delivered a duplicate object removal callback");
  require(
      removals.front().object == object &&
          removals.front().tag.empty() &&
          removals.front().producer == lost.federateHandle(),
      "automatic resign directive returned the wrong removal metadata");
  requireException(
      [&] {
        static_cast<void>(survivor.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up an automatically deleted object");

  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioAutomaticResignDirectiveDeleteObjectsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutomaticResignDirectiveDeleteObjects(options, model);
}

void scenarioPublicHandleDecoding(Options const& options, rti::CallbackModel model) {
  scenarioHandleLookups(options, model);
  scenarioHandleWireFormats(options, model);
}

void scenarioPublicHandleDecodingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPublicHandleDecoding(options, model);
}

int runAutomaticResignDirectiveScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != automaticResignDirectiveDeleteObjectsId &&
          scenario != automaticResignDirectiveDeleteObjectsContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      try {
        if (scenario == automaticResignDirectiveDeleteObjectsId) {
          scenarioAutomaticResignDirectiveDeleteObjects(options, callback.second);
        } else {
          scenarioAutomaticResignDirectiveDeleteObjectsContract(
              options,
              callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasAutomaticResignDirectiveScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == automaticResignDirectiveDeleteObjectsId ||
        scenario == automaticResignDirectiveDeleteObjectsContractId) {
      return true;
    }
  }
  return false;
}

int runPublicHandleDecodingScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != publicHandleDecodingId &&
          scenario != publicHandleDecodingContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      try {
        if (scenario == publicHandleDecodingId) {
          scenarioPublicHandleDecoding(options, callback.second);
        } else {
          scenarioPublicHandleDecodingContract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasPublicHandleDecodingScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == publicHandleDecodingId ||
        scenario == publicHandleDecodingContractId) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (hasAutomaticResignDirectiveScenario(argc, argv)) {
      return runAutomaticResignDirectiveScenarios(argc, argv);
    }
    if (hasPublicHandleDecodingScenario(argc, argv)) {
      return runPublicHandleDecodingScenarios(argc, argv);
    }
    return hla_rti_cpp_tck_original_main(argc, argv);
  } catch (std::exception const& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
