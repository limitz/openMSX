// $Id$

#include <cassert>
#include "xmlx.hh"
#include "File.hh"
#include "FileContext.hh"
#include "HardwareConfig.hh"

namespace openmsx {

HardwareConfig::HardwareConfig()
	: MSXConfig("hardware")
{
}

HardwareConfig::~HardwareConfig()
{
}

HardwareConfig& HardwareConfig::instance()
{
	static HardwareConfig oneInstance;
	return oneInstance;
}

void HardwareConfig::loadHardware(XMLElement& root, const string& path,
                                  const string& hwName)
{
	SystemFileContext context;
	string filename;
	try {
		filename = context.resolve(
			path + '/' + hwName + ".xml");
	} catch (FileException& e) {
		filename = context.resolve(
			path + '/' + hwName + "/hardwareconfig.xml");
	}
	File file(filename);
	XMLDocument doc(file.getLocalName());

	// get url
	string url(file.getURL());
	string::size_type pos = url.find_last_of('/');
	assert(pos != string::npos);	// protocol must contain a '/'
	url = url.substr(0, pos);
	PRT_DEBUG("Hardware config: url "<<url);
	
	// TODO get user name
	string userName;
	
	ConfigFileContext context2(url + '/', hwName, userName);
	handleDoc(root, doc, context2);
}

} // namespace openmsx
