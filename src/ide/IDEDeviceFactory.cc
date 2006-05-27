// $Id$

#include "IDEDeviceFactory.hh"
#include "IDEHD.hh"
#include "IDECDROM.hh"
#include "XMLElement.hh"
#include "MSXException.hh"

using std::string;

namespace openmsx {

IDEDevice* IDEDeviceFactory::create(MSXMotherBoard& motherBoard,
                                    const XMLElement& config,
                                    const EmuTime& time, const string& name)
{
	const string& type = config.getChildData("type");
	if (type == "IDEHD") {
		return new IDEHD(motherBoard, config, time, name);
	} else if (type == "IDECDROM") {
		return new IDECDROM(motherBoard, config, time);
	}
	throw MSXException("Unknown IDE device: " + type);
}

} // namespace openmsx
