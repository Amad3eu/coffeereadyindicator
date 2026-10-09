#include "infrastructure/esp32/NvsConfigStore.h"

#include <Preferences.h>

namespace coffee {

namespace {
constexpr const char* kKeyLightOn = "lightOn";
constexpr const char* kKeyLightOff = "lightOff";
constexpr const char* kKeyCycleMs = "cycleMs";
constexpr const char* kKeyReadyMs = "readyMs";
constexpr const char* kKeySensorMax = "sensorMax";
}  // namespace

bool NvsConfigStore::load(CoffeeConfig& out) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return false;
    }
    const bool stored = prefs.isKey(kKeyLightOn);
    if (stored) {
        CoffeeConfig defaults;
        out.lightOnThreshold = prefs.getUShort(kKeyLightOn, defaults.lightOnThreshold);
        out.lightOffThreshold = prefs.getUShort(kKeyLightOff, defaults.lightOffThreshold);
        out.cycleStartConfirmationMs = prefs.getULong(kKeyCycleMs, defaults.cycleStartConfirmationMs);
        out.readyConfirmationMs = prefs.getULong(kKeyReadyMs, defaults.readyConfirmationMs);
        out.sensorMaxValue = prefs.getUShort(kKeySensorMax, defaults.sensorMaxValue);
    }
    prefs.end();
    return stored;
}

bool NvsConfigStore::save(const CoffeeConfig& config) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    bool ok = true;
    ok = prefs.putUShort(kKeyLightOn, config.lightOnThreshold) > 0 && ok;
    ok = prefs.putUShort(kKeyLightOff, config.lightOffThreshold) > 0 && ok;
    ok = prefs.putULong(kKeyCycleMs, config.cycleStartConfirmationMs) > 0 && ok;
    ok = prefs.putULong(kKeyReadyMs, config.readyConfirmationMs) > 0 && ok;
    ok = prefs.putUShort(kKeySensorMax, config.sensorMaxValue) > 0 && ok;
    prefs.end();
    return ok;
}

bool NvsConfigStore::clear() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return false;
    }
    const bool ok = prefs.clear();
    prefs.end();
    return ok;
}

}  // namespace coffee
