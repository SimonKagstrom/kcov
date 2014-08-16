#pragma once

#include "../test.hh"

#include <collector.hh>
#include <string>

class MockCollector : public kcov::ICollector
{
public:
	MOCK_METHOD1(registerListener, void(kcov::ICollector::IListener &listener));
	MOCK_METHOD1(registerEventTickListener, void(kcov::ICollector::IEventTickListener &listener));
	MOCK_METHOD0(prepare, int());
	MOCK_METHOD1(run, int(const std::string &));
	MOCK_METHOD0(stop, void());

	void mockRegisterListener(IListener &listener)
	{
		m_listener = &listener;
	}

	IListener *m_listener;
};
