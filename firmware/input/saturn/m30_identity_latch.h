#pragma once

class M30IdentityLatch {
 public:
  void observe(bool qualifiedMega6, bool homePressed, bool starPressed) {
    if (qualifiedMega6 && (homePressed || starPressed)) {
      identified_ = true;
    }
  }

  bool identified() const {
    return identified_;
  }

  void reset() {
    identified_ = false;
  }

 private:
  bool identified_ = false;
};
