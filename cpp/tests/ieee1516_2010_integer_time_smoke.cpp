#include <RTI/VariableLengthData.h>
#include <RTI/LogicalTimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

#include <cassert>
#include <cstring>
#include <memory>

int main() {
  using namespace rti1516e;

  HLAinteger64TimeFactory factory;
  auto initial = factory.makeInitial();
  auto final = factory.makeFinal();
  auto epsilon = factory.makeEpsilon();
  assert(initial->isInitial());
  assert(final->isFinal());
  assert(epsilon->isEpsilon());
  assert(factory.getName() == HLAinteger64TimeName);

  auto time = factory.makeLogicalTime(5);
  auto interval = factory.makeLogicalTimeInterval(2);
  auto const encoded = time->encode();
  assert(encoded.size() == 8U);
  assert(static_cast<unsigned char const*>(encoded.data())[7] == 5U);

  *time += *interval;
  assert(time->getTime() == 7);
  *time -= *interval;
  assert(time->getTime() == 5);
  interval->setToDifference(*time, *initial);
  assert(interval->getInterval() == 5);

  auto decoded = factory.decodeLogicalTime(encoded);
  assert(dynamic_cast<HLAinteger64Time*>(decoded.get())->getTime() == 5);
  bool rejected = false;
  try {
    decoded->decode(VariableLengthData("\x80\0\0\0\0\0\0\0", 8));
  } catch (CouldNotDecode const&) {
    rejected = true;
  }
  assert(rejected);

  auto selected = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  assert(selected.get() != nullptr);
  assert(selected->getName() == HLAinteger64TimeName);
  auto fedtime = LogicalTimeFactoryFactory::makeLogicalTimeFactory(HLAinteger64TimeName);
  assert(fedtime.get() != nullptr);
  return 0;
}
