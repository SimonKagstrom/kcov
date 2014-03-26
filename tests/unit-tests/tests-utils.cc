#include "test.hh"

#include <utils.hh>
#include <string>

TESTSUITE(utils)
{
	TEST(escapeHtml)
	{
		char buf[8192];

		memset(buf, '&', sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';

		std::string s = escape_html(buf);
		// Should have been truncated
		ASSERT_TRUE(s.size() < 4096);

		ASSERT_TRUE(s.find("&amp;") != std::string::npos);
	}
}
