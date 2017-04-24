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

	TEST(escapeJson)
	{
		std::string a = "var kalle=1;";
		std::string b = "var kalle=\"1\";";
		std::string c = "var a=\"\\hej\";";
		std::string d = "var kalle=\tX;";
		std::string e = "var kalle='X';";

		std::string s;

		s = escape_json(a);
		ASSERT_TRUE(s == a);

		s = escape_json(b);
		ASSERT_TRUE(s == "var kalle=\\\"1\\\";");

		s = escape_json(c);
		ASSERT_TRUE(s == "var a=\\\"\\\\hej\\\";");

		s = escape_json(d);
		ASSERT_TRUE(s == "var kalle=\\tX;");

		s = escape_json(e);
		ASSERT_TRUE(s == "var kalle=\\'X\\';");
	}

	TEST(can_concatenate_directory_and_file_correctly)
	{
		std::string empty = "";
		std::string root = "/";
		std::string singleNoSlash = "/kalle";
		std::string singleSlash = "/kalle/";
		std::string singleDoubleSlashes = "/kalle//";
		std::string doubleNoSlashes = "/kalle/manne";
		std::string doubleDoubleSlashes = "/kalle/manne//";

		ASSERT_TRUE(dir_concat(empty, "hej") == "hej");
		ASSERT_TRUE(dir_concat(root, "hej") == "/hej");
		ASSERT_TRUE(dir_concat(singleNoSlash, "hej") == "/kalle/hej");
		ASSERT_TRUE(dir_concat(singleSlash, "hej") == "/kalle/hej");
		ASSERT_TRUE(dir_concat(singleDoubleSlashes, "hej") == "/kalle/hej");
		ASSERT_TRUE(dir_concat(doubleNoSlashes, "hej") == "/kalle/manne/hej");
		ASSERT_TRUE(dir_concat(doubleDoubleSlashes, "hej") == "/kalle/manne/hej");

		// Filename with slash
		ASSERT_TRUE(dir_concat(singleDoubleSlashes, "/hej") == "/kalle/hej");
	}
}
