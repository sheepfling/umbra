#include <memory>

#include <RTI/RTI1516.h>

int main() {
  rti1516_2025::RTIambassadorFactory factory;
  std::unique_ptr<rti1516_2025::RTIambassador> rti = factory.createRTIambassador();
  if (!rti || rti1516_2025::rtiName().empty() || rti1516_2025::rtiVersion().empty()) {
    return 1;
  }

  try {
    rti->listFederationExecutions();
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  } catch (rti1516_2025::NotConnected const&) {
    return 0;
#else
  } catch (rti1516_2025::RTIinternalError const&) {
    return 0;
#endif
  } catch (...) {
    return 2;
  }
  return 3;
}
