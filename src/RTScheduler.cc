#include "RTScheduler.hh"
#include "RTSchedulable.hh"
#include <algorithm>
#include <limits>
#include <iterator>

namespace openmsx {

struct EqualRTSchedulable {
	EqualRTSchedulable(const RTSchedulable& schedulable_)
		: schedulable(schedulable_) {}
	bool operator()(const RTSyncPoint& sp) const {
		return sp.schedulable == &schedulable;
	}
	const RTSchedulable& schedulable;
};

void RTScheduler::add(uint64_t delta, RTSchedulable& schedulable)
{
	queue.insert(RTSyncPoint{Timer::getTime() + delta, &schedulable},
	             [](RTSyncPoint& sp) {
                             sp.time = std::numeric_limits<uint64_t>::max(); },
	             [](const RTSyncPoint& x, const RTSyncPoint& y) {
	                     return x.time < y.time; });
}

bool RTScheduler::remove(RTSchedulable& schedulable)
{
	return queue.remove(EqualRTSchedulable(schedulable));
}

bool RTScheduler::isPending(const RTSchedulable& schedulable) const
{
	return std::find_if(std::begin(queue), std::end(queue),
	                    EqualRTSchedulable(schedulable)) != std::end(queue);
}

void RTScheduler::scheduleHelper(uint64_t limit)
{
	// Process at most this many events to prevent getting stuck in an
	// infinite loop when a RTSchedulable keeps on rescheduling itself in
	// the (too) near future.
	auto count = queue.size();
	while (true) {
		auto* schedulable = queue.front().schedulable;
		queue.remove_front();

		schedulable->executeRT();

		// It's possible RTSchedulables are canceled in the mean time,
		// so we can't rely on 'count' to replace this empty check.
		if (queue.empty()) break;
		if (likely(queue.front().time > limit)) break;
		if (--count == 0) break;
	}
}

} // namespace openmsx
