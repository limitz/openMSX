// $Id$

#ifndef CLICOMM_HH
#define CLICOMM_HH

#include "EventListener.hh"
#include "Semaphore.hh"
#include "noncopyable.hh"
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace openmsx {

class EventDistributor;
class GlobalCommandController;
class CliConnection;
class UpdateCmd;

class CliComm : private EventListener, private noncopyable
{
public:
	enum LogLevel {
		INFO,
		WARNING
	};
	enum ReplyStatus {
		OK,
		NOK
	};
	enum UpdateType {
		LED,
		BREAK,
		RESUME,
		SETTING,
		HARDWARE,
		PLUG,
		UNPLUG,
		MEDIA,
		STATUS,
		NUM_UPDATES // must be last
	};

	CliComm(GlobalCommandController& commandController,
	        EventDistributor& eventDistributor);
	virtual ~CliComm();

	void addConnection(std::auto_ptr<CliConnection> connection);

	void startInput(const std::string& option);

	void log(LogLevel level, const std::string& message);
	void update(UpdateType type, const std::string& name, const std::string& value);

	// convenience methods
	void printInfo(const std::string& message) {
		log(INFO, message);
	}
	void printWarning(const std::string& message) {
		log(WARNING, message);
	}

private:
	// EventListener
	virtual bool signalEvent(shared_ptr<const Event> event);

	const std::auto_ptr<UpdateCmd> updateCmd;

	std::map<std::string, std::string> prevValues[NUM_UPDATES];

	GlobalCommandController& commandController;
	EventDistributor& eventDistributor;

	bool xmlOutput;
	typedef std::vector<CliConnection*> Connections;
	Connections connections;
	Semaphore sem; // lock access to connections member
};

} // namespace openmsx

#endif
