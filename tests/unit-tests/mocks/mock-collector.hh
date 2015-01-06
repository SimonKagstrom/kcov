#pragma once

#include "../test.hh"

#include <collector.hh>
#include <string>

class MockCollector : public kcov::ICollector
{
public:
	MAKE_MOCK1(registerListener, void(kcov::ICollector::IListener &listener));
	MAKE_MOCK1(registerEventTickListener, void(kcov::ICollector::IEventTickListener &listener));
	MAKE_MOCK0(prepare, int());
	MAKE_MOCK1(run, int(const std::string &));
	MAKE_MOCK0(stop, void());

	void mockRegisterListener(IListener &listener)
	{
		m_listener = &listener;
	}

	IListener *m_listener;
};
