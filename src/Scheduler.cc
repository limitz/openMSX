// $Id$

#include "Scheduler.hh"
#include "MSXCPU.hh"
#include "HotKey.hh"
#include "ConsoleSource/ConsoleManager.hh"
#include "ConsoleSource/CommandController.hh"
#include "Mixer.hh"
#include <cassert>
#include <algorithm>
#include "EventDistributor.hh"
#include "Schedulable.hh"


const EmuTime Scheduler::ASAP;

Scheduler::Scheduler()
{
	paused = false;
	noSound = false;
	needBlock = false;
	exitScheduler = false;
	cpu = MSXCPU::instance();
	
	EventDistributor::instance()->registerEventListener(SDL_QUIT, this);
	CommandController::instance()->registerCommand(quitCmd, "quit");
	CommandController::instance()->registerCommand(muteCmd, "mute");
	HotKey::instance()->registerHotKeyCommand(SDLK_F12, "quit");
	HotKey::instance()->registerHotKeyCommand(SDLK_F11, "mute");
}

Scheduler::~Scheduler()
{
	HotKey::instance()->unregisterHotKeyCommand(SDLK_F12, "quit");
	HotKey::instance()->unregisterHotKeyCommand(SDLK_F11, "mute");
	CommandController::instance()->unregisterCommand("quit");
	CommandController::instance()->unregisterCommand("mute");
	EventDistributor::instance()->unregisterEventListener(SDL_QUIT, this);
}

Scheduler* Scheduler::instance()
{
	if (oneInstance == NULL ) {
		oneInstance = new Scheduler();
	}
	return oneInstance;
}
Scheduler *Scheduler::oneInstance = NULL;


void Scheduler::setSyncPoint(const EmuTime &timestamp, Schedulable* device, int userData)
{
	EmuTime time(timestamp);
	
	schedMutex.grab();
	
	PRT_DEBUG("Sched: registering " << device->getName() << " for emulation at " << time);
	PRT_DEBUG("Sched:  CPU is at " << cpu->getCurrentTime());

	if (time == ASAP)
		time = cpu->getCurrentTime();
	//assert (time >= cpu->getCurrentTime());
	if (time < cpu->getTargetTime())
		cpu->setTargetTime(time);
	syncPoints.push_back(SynchronizationPoint (time, device, userData));
	push_heap(syncPoints.begin(), syncPoints.end());

	pauseCond.signal();
	schedMutex.release();
}

bool Scheduler::removeSyncPoint(Schedulable* device, int userData)
{
	bool result;
	schedMutex.grab();

	std::vector<SynchronizationPoint>::iterator i;
	for (i=syncPoints.begin(); i!=syncPoints.end(); i++) {
		if (((*i).getDevice() == device) &&
		     (*i).getUserData() == userData) {
			syncPoints.erase(i);
			make_heap(syncPoints.begin(), syncPoints.end());
			result = true;
			break;
		}
	}
	result = false;
	
	schedMutex.release();
	return result;
}

void Scheduler::stopScheduling()
{
	exitScheduler = true;
	reschedule();
	unpause();
}

void Scheduler::reschedule()
{
	// TODO
	// Reschedule ASAP. We must give a device, choose MSXCPU.
	EmuTime zero;
	setSyncPoint(zero, cpu);
}

void Scheduler::scheduleEmulation()
{
	while (!exitScheduler) {
		schedMutex.grab();
		if (syncPoints.empty()) {
			// nothing scheduled, emulate CPU
			schedMutex.release();
			if (!paused) {
				PRT_DEBUG ("Sched: Scheduling CPU till infinity");
				const EmuTime infinity = EmuTime(EmuTime::INFTY);
				cpu->executeUntilTarget(infinity);
			} else {
				needBlock = true;
			}
		} else {
			const SynchronizationPoint sp = *(syncPoints.begin());
			const EmuTime &time = sp.getTime();
			if (cpu->getCurrentTime() < time) {
				schedMutex.release();
				// emulate CPU till first SP, don't immediately emulate
				// device since CPU could not have reached SP
				if (!paused) {
					PRT_DEBUG ("Sched: Scheduling CPU till " << time);
					cpu->executeUntilTarget(time);
				} else {
					needBlock = true;
				}
			} else {
				// if CPU has reached SP, emulate the device
				pop_heap(syncPoints.begin(), syncPoints.end());
				syncPoints.pop_back();
				schedMutex.release();
				Schedulable *device = sp.getDevice();
				int userData = sp.getUserData();
				PRT_DEBUG ("Sched: Scheduling " << device->getName() << " till " << time);
				device->executeUntilEmuTime(time, userData);
				
			}
		}
		if (needBlock) {
			pauseCond.wait();
		}
	}
}

void Scheduler::unpause()
{
	if (paused) {
		paused = false;
		needBlock = false;
		Mixer::instance()->pause(noSound);
		PRT_DEBUG("Unpaused");
		pauseCond.signal();
	}
}
void Scheduler::pause()
{
	if (!paused) {
		paused = true;
		Mixer::instance()->pause(true);
		PRT_DEBUG("Paused");
	}
}
bool Scheduler::isPaused()
{
	return paused;
}


void Scheduler::signalEvent(SDL_Event &event) {
	if (event.type == SDL_QUIT) {
		stopScheduling();
	} else {
		assert(false);
	}
}


void Scheduler::QuitCmd::execute(const std::vector<std::string> &tokens)
{
	Scheduler::instance()->stopScheduling();
}
void Scheduler::QuitCmd::help   (const std::vector<std::string> &tokens)
{
	ConsoleManager::instance()->print("Use this command to stop the emulator");
}

//TODO this command belongs in Mixer instead of Scheduler
void Scheduler::MuteCmd::execute(const std::vector<std::string> &tokens)
{
	Scheduler *sch = Scheduler::instance();
	switch (tokens.size()) {
		case 1:
			sch->noSound = !sch->noSound;
			break;
		case 2:
			if (tokens[1] == "on") {
				sch->noSound = true;
				break;
			}
			if (tokens[1] == "off") {
				sch->noSound = false;
				break;
			}
			// fall through
		default:
			throw CommandException("Syntax error");
	}
	Mixer::instance()->pause(sch->noSound||sch->isPaused());
}
void Scheduler::MuteCmd::help   (const std::vector<std::string> &tokens)
{
	ConsoleManager::instance()->print("Use this command to mute/unmute the emulator");
	ConsoleManager::instance()->print(" mute:     toggle mute");
	ConsoleManager::instance()->print(" mute on:  set muted");
	ConsoleManager::instance()->print(" mute off: set unmuted");
}

