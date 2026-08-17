#include <RTI/RangeBounds.h>

namespace rti1516_2025 {

RangeBounds::RangeBounds() : _lowerBound(0), _upperBound(0) {}

RangeBounds::RangeBounds(unsigned long lowerBound, unsigned long upperBound)
    : _lowerBound(lowerBound), _upperBound(upperBound) {}

RangeBounds::~RangeBounds() noexcept = default;

RangeBounds::RangeBounds(RangeBounds const& rhs)
    : _lowerBound(rhs._lowerBound), _upperBound(rhs._upperBound) {}

RangeBounds& RangeBounds::operator=(RangeBounds const& rhs) {
  if (this == &rhs) {
    return *this;
  }
  _lowerBound = rhs._lowerBound;
  _upperBound = rhs._upperBound;
  return *this;
}

unsigned long RangeBounds::getLowerBound() const {
  return _lowerBound;
}

unsigned long RangeBounds::getUpperBound() const {
  return _upperBound;
}

void RangeBounds::setLowerBound(unsigned long lowerBound) {
  _lowerBound = lowerBound;
}

void RangeBounds::setUpperBound(unsigned long upperBound) {
  _upperBound = upperBound;
}

}  // namespace rti1516_2025
