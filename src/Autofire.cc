// $Id$

#include "Autofire.hh"
#include "EmuTime.hh"

namespace openmsx {

Autofire::Autofire(unsigned newMinInts, unsigned newMaxInts, const string& name)
	: min_ints(newMinInts), max_ints(newMaxInts),
	  speedSetting(name, "controls the speed of this autofire circuit",
	               0, 0, 100)
{
	if (min_ints < 1) {
		min_ints = 1;
	}
	if (max_ints <= min_ints) {
		max_ints = min_ints + 1;
	}

	speedSetting.addListener(this);
}

Autofire::~Autofire()
{
	speedSetting.removeListener(this);
}

void Autofire::update(const SettingLeafNode* setting) throw()
{
	assert(setting == &speedSetting);
	int speed = speedSetting.getValue();
	freq_divider = (MAIN_FREQ * (max_ints - (speed * (max_ints - min_ints)) / 100)) / (2 * 50 * 60);
}

byte Autofire::getSignal(const EmuTime &time)
{
	if (speedSetting.getValue() != 0) {
		return (time.time / freq_divider) & 1;
	} else {
		return 0;
	}
}

} // namespace openmsx
