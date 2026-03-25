#include "pch.h"
#include <gtest/gtest.h>
#include "Utilities/Video/AviWriter.h"
#include "Shared/Video/VideoRenderer.h"

/// Tests for video recording format constants, enums, and structs.
/// Tracks: #922 (Screenshot and recording integration tests)

// VideoCodec enum values
TEST(VideoRecordingFormatTests, VideoCodec_NoneIsZero) {
	EXPECT_EQ(static_cast<int>(VideoCodec::None), 0);
}

TEST(VideoRecordingFormatTests, VideoCodec_ZMBVIsOne) {
	EXPECT_EQ(static_cast<int>(VideoCodec::ZMBV), 1);
}

TEST(VideoRecordingFormatTests, VideoCodec_CSCDIsTwo) {
	EXPECT_EQ(static_cast<int>(VideoCodec::CSCD), 2);
}

TEST(VideoRecordingFormatTests, VideoCodec_GIFIsThree) {
	EXPECT_EQ(static_cast<int>(VideoCodec::GIF), 3);
}

// RecordAviOptions struct defaults
TEST(VideoRecordingFormatTests, RecordAviOptions_DefaultsAreZeroed) {
	RecordAviOptions options = {};
	EXPECT_EQ(options.Codec, VideoCodec::None);
	EXPECT_EQ(options.CompressionLevel, 0u);
	EXPECT_FALSE(options.RecordSystemHud);
	EXPECT_FALSE(options.RecordInputHud);
}

TEST(VideoRecordingFormatTests, RecordAviOptions_CanSetCodec) {
	RecordAviOptions options = {};
	options.Codec = VideoCodec::ZMBV;
	options.CompressionLevel = 5;
	EXPECT_EQ(options.Codec, VideoCodec::ZMBV);
	EXPECT_EQ(options.CompressionLevel, 5u);
}

TEST(VideoRecordingFormatTests, RecordAviOptions_CanSetHudFlags) {
	RecordAviOptions options = {};
	options.RecordSystemHud = true;
	options.RecordInputHud = true;
	EXPECT_TRUE(options.RecordSystemHud);
	EXPECT_TRUE(options.RecordInputHud);
}

TEST(VideoRecordingFormatTests, RecordAviOptions_GifCodec) {
	RecordAviOptions options = {};
	options.Codec = VideoCodec::GIF;
	EXPECT_EQ(options.Codec, VideoCodec::GIF);
}

TEST(VideoRecordingFormatTests, RecordAviOptions_CSCDCodec) {
	RecordAviOptions options = {};
	options.Codec = VideoCodec::CSCD;
	EXPECT_EQ(options.Codec, VideoCodec::CSCD);
}

// RecordAviOptions field independence
TEST(VideoRecordingFormatTests, RecordAviOptions_HudFlagsAreIndependent) {
	RecordAviOptions opts1 = {};
	opts1.RecordSystemHud = true;
	opts1.RecordInputHud = false;
	EXPECT_TRUE(opts1.RecordSystemHud);
	EXPECT_FALSE(opts1.RecordInputHud);

	RecordAviOptions opts2 = {};
	opts2.RecordSystemHud = false;
	opts2.RecordInputHud = true;
	EXPECT_FALSE(opts2.RecordSystemHud);
	EXPECT_TRUE(opts2.RecordInputHud);
}
