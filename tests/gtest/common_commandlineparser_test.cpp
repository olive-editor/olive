#include <gtest/gtest.h>

#include "common/commandlineparser.h"

TEST(CommonCommandLineParser, OptionWithoutArgument)
{
	CommandLineParser parser;
	const CommandLineParser::Option *opt =
		parser.AddOption({ QStringLiteral("help"), QStringLiteral("h") },
						 QStringLiteral("Show help"), false);

	parser.Process({ QStringLiteral("app"), QStringLiteral("-help") });

	EXPECT_TRUE(opt->IsSet());
}

TEST(CommonCommandLineParser, ShortOption)
{
	CommandLineParser parser;
	const CommandLineParser::Option *opt =
		parser.AddOption({ QStringLiteral("help"), QStringLiteral("h") },
						 QStringLiteral("Show help"), false);

	parser.Process({ QStringLiteral("app"), QStringLiteral("-h") });

	EXPECT_TRUE(opt->IsSet());
}

TEST(CommonCommandLineParser, OptionWithArgument)
{
	CommandLineParser parser;
	const CommandLineParser::Option *opt = parser.AddOption(
		{ QStringLiteral("project") }, QStringLiteral("Project file"), true,
		QStringLiteral("file"));

	parser.Process({ QStringLiteral("app"), QStringLiteral("-project"),
					 QStringLiteral("test.ove") });

	EXPECT_TRUE(opt->IsSet());
	EXPECT_EQ(opt->GetSetting(), QStringLiteral("test.ove"));
}

TEST(CommonCommandLineParser, PositionalArgument)
{
	CommandLineParser parser;
	const CommandLineParser::PositionalArgument *arg =
		parser.AddPositionalArgument(QStringLiteral("filename"),
									 QStringLiteral("Project file"), true);

	parser.Process({ QStringLiteral("app"), QStringLiteral("test.ove") });

	EXPECT_EQ(arg->GetSetting(), QStringLiteral("test.ove"));
}

TEST(CommonCommandLineParser, UnknownOptionWarning)
{
	CommandLineParser parser;
	parser.AddOption({ QStringLiteral("known") }, QStringLiteral("Known"));

	// Should not crash; unknown option is logged
	parser.Process({ QStringLiteral("app"), QStringLiteral("-unknown") });
}

TEST(CommonCommandLineParser, UnknownPositionalWarning)
{
	CommandLineParser parser;

	// Should not crash; unknown positional is logged
	parser.Process({ QStringLiteral("app"), QStringLiteral("extra") });
}

TEST(CommonCommandLineParser, HiddenOptionExcludedFromHelp)
{
	CommandLineParser parser;
	parser.AddOption({ QStringLiteral("visible") }, QStringLiteral("Visible"));
	parser.AddOption({ QStringLiteral("hidden") }, QStringLiteral("Hidden"),
					 false, QString(), true);
	parser.AddPositionalArgument(QStringLiteral("file"),
								 QStringLiteral("Input file"));

	// Should not crash; hidden option should be skipped during help output
	parser.PrintHelp("/usr/bin/app");
}
