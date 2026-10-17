#include "pch.h"
#include "Utilities/FastString.h"

// Test fixture for FastString
class FastStringTest : public ::testing::Test {};

// ===== Construction Tests =====

TEST_F(FastStringTest, DefaultConstruction_IsEmpty) {
	FastString fs;
	EXPECT_EQ(fs.GetSize(), 0);
	EXPECT_STREQ(fs.ToString(), "");
}

TEST_F(FastStringTest, Construct_FromCString) {
	FastString fs("Hello", 5);
	EXPECT_EQ(fs.GetSize(), 5);
	EXPECT_STREQ(fs.ToString(), "Hello");
}

TEST_F(FastStringTest, Construct_FromStdString) {
	std::string s = "World";
	FastString fs(s);
	EXPECT_EQ(fs.GetSize(), 5);
	EXPECT_STREQ(fs.ToString(), "World");
}

// ===== Write Single Character =====

TEST_F(FastStringTest, Write_SingleChar) {
	FastString fs;
	fs.Write('A');
	EXPECT_EQ(fs.GetSize(), 1);
	EXPECT_STREQ(fs.ToString(), "A");
}

TEST_F(FastStringTest, Write_MultipleChars) {
	FastString fs;
	fs.Write('H');
	fs.Write('i');
	EXPECT_EQ(fs.GetSize(), 2);
	EXPECT_STREQ(fs.ToString(), "Hi");
}

// ===== Write C-String =====

TEST_F(FastStringTest, Write_CString) {
	FastString fs;
	fs.Write("Test");
	EXPECT_EQ(fs.GetSize(), 4);
	EXPECT_STREQ(fs.ToString(), "Test");
}

TEST_F(FastStringTest, Write_CStringWithSize) {
	FastString fs;
	fs.Write("Hello World", 5);
	EXPECT_EQ(fs.GetSize(), 5);
	EXPECT_STREQ(fs.ToString(), "Hello");
}

// ===== Write std::string =====

TEST_F(FastStringTest, Write_StdString) {
	FastString fs;
	std::string s = "StdStr";
	fs.Write(s);
	EXPECT_EQ(fs.GetSize(), 6);
	EXPECT_STREQ(fs.ToString(), "StdStr");
}

// ===== Write FastString =====

TEST_F(FastStringTest, Write_FastString) {
	FastString source;
	source.Write("Source");

	FastString dest;
	dest.Write("Dest:");
	dest.Write(source);

	EXPECT_STREQ(dest.ToString(), "Dest:Source");
	EXPECT_EQ(dest.GetSize(), 11);
}

// ===== Delimiter =====

TEST_F(FastStringTest, Delimiter_SkippedWhenEmpty) {
	FastString fs;
	fs.Delimiter(", ");
	EXPECT_EQ(fs.GetSize(), 0);
	EXPECT_STREQ(fs.ToString(), "");
}

TEST_F(FastStringTest, Delimiter_WrittenWhenNonEmpty) {
	FastString fs;
	fs.Write("A");
	fs.Delimiter(", ");
	fs.Write("B");
	EXPECT_STREQ(fs.ToString(), "A, B");
}

TEST_F(FastStringTest, Delimiter_MultipleItems) {
	FastString fs;
	const char* items[] = {"X", "Y", "Z"};
	for (const char* item : items) {
		fs.Delimiter(", ");
		fs.Write(item);
	}
	EXPECT_STREQ(fs.ToString(), "X, Y, Z");
}

// ===== Lowercase Mode =====

TEST_F(FastStringTest, Lowercase_SingleChar) {
	FastString fs(true);
	fs.Write('A');
	EXPECT_STREQ(fs.ToString(), "a");
}

TEST_F(FastStringTest, Lowercase_CString) {
	FastString fs(true);
	fs.Write("HELLO");
	EXPECT_STREQ(fs.ToString(), "hello");
}

TEST_F(FastStringTest, Lowercase_MixedCase) {
	FastString fs(true);
	fs.Write("HeLLo WoRLd");
	EXPECT_STREQ(fs.ToString(), "hello world");
}

TEST_F(FastStringTest, Lowercase_StdString) {
	FastString fs(true);
	std::string s = "ABC";
	fs.Write(s);
	EXPECT_STREQ(fs.ToString(), "abc");
}

TEST_F(FastStringTest, Lowercase_PreserveCase) {
	FastString fs(true);
	std::string s = "ABC";
	fs.Write(s, true); // preserveCase=true
	EXPECT_STREQ(fs.ToString(), "ABC");
}

TEST_F(FastStringTest, Lowercase_NonAlpha) {
	FastString fs(true);
	fs.Write("123!@#");
	EXPECT_STREQ(fs.ToString(), "123!@#");
}

// ===== WriteSafe =====

TEST_F(FastStringTest, WriteSafe_NormalWrite) {
	FastString fs;
	fs.WriteSafe('X');
	EXPECT_EQ(fs.GetSize(), 1);
	EXPECT_STREQ(fs.ToString(), "X");
}

TEST_F(FastStringTest, WriteSafe_AtBoundary_DoesNotOverflow) {
	FastString fs;
	// Fill to 999 chars
	for (int i = 0; i < 999; i++) {
		fs.WriteSafe('A');
	}
	EXPECT_EQ(fs.GetSize(), 999);

	// 1000th should be silently ignored
	fs.WriteSafe('B');
	EXPECT_EQ(fs.GetSize(), 999);
}

// ===== Reset =====

TEST_F(FastStringTest, Reset_ClearsPosition) {
	FastString fs;
	fs.Write("Hello");
	EXPECT_EQ(fs.GetSize(), 5);

	fs.Reset();
	EXPECT_EQ(fs.GetSize(), 0);
	EXPECT_STREQ(fs.ToString(), "");
}

TEST_F(FastStringTest, Reset_AllowsReuse) {
	FastString fs;
	fs.Write("First");
	fs.Reset();
	fs.Write("Second");
	EXPECT_STREQ(fs.ToString(), "Second");
}

// ===== Subscript Operator =====

TEST_F(FastStringTest, Subscript_ReadsCorrectChar) {
	FastString fs;
	fs.Write("ABCDE");
	EXPECT_EQ(fs[0], 'A');
	EXPECT_EQ(fs[2], 'C');
	EXPECT_EQ(fs[4], 'E');
}

// ===== Chaining Writes =====

TEST_F(FastStringTest, Chaining_MultipleTypes) {
	FastString fs;
	fs.Write("Name:");
	fs.Write(' ');
	std::string name = "Test";
	fs.Write(name);
	EXPECT_STREQ(fs.ToString(), "Name: Test");
}

// ===== Stress: Moderate-sized string =====

TEST_F(FastStringTest, Write_ModerateLength_Succeeds) {
	FastString fs;
	// Write 500 'A' characters — well within 1000 byte buffer
	for (int i = 0; i < 500; i++) {
		fs.Write('A');
	}
	EXPECT_EQ(fs.GetSize(), 500);
	std::string result(fs.ToString());
	EXPECT_EQ(result.size(), 500u);
	EXPECT_TRUE(std::all_of(result.begin(), result.end(), [](char c) { return c == 'A'; }));
}
