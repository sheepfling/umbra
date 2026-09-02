#include <RTI/LogicalTimeFactory.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>

#include <cassert>
#include <cstring>
#include <memory>

int main() {
  using namespace rti1516e;

  HLAfloat64TimeFactory factory;
  auto initial = factory.makeInitial();
  auto final = factory.makeFinal();
  auto epsilon = factory.makeEpsilon();
  assert(initial->isInitial());
  assert(final->isFinal());
  assert(epsilon->isEpsilon());
  assert(factory.getName() == HLAfloat64TimeName);

  auto time = factory.makeLogicalTime(1.25);
  auto interval = factory.makeLogicalTimeInterval(2.5);
  auto encoded = time->encode();
  const unsigned char expected[] = {0x3f, 0xf4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  assert(encoded.size() == sizeof(expected));
  assert(std::memcmp(encoded.data(), expected, sizeof(expected)) == 0);
  *time += *interval;
  assert(time->getTime() == 3.75);
  *time -= *interval;
  assert(time->getTime() == 1.25);
  interval->setToDifference(*factory.makeLogicalTime(5.5), *time);
  assert(interval->getInterval() == 4.25);

  auto decoded = factory.decodeLogicalTime(encoded);
  assert(dynamic_cast<HLAfloat64Time*>(decoded.get())->getTime() == 1.25);
  auto selected = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(HLAfloat64TimeName);
  assert(selected.get() != nullptr);
  assert(selected->getName() == HLAfloat64TimeName);

  bool rejected = false;
  try {
    decoded->decode(VariableLengthData("\xbf\xf0\0\0\0\0\0\0", 8));
  } catch (CouldNotDecode const&) {
    rejected = true;
  }
  assert(rejected);
  return 0;
}
