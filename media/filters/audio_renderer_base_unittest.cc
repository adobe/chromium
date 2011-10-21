// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/stl_util.h"
#include "media/base/data_buffer.h"
#include "media/base/mock_callback.h"
#include "media/base/mock_filter_host.h"
#include "media/base/mock_filters.h"
#include "media/filters/audio_renderer_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest/include/gtest/gtest_prod.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrictMock;

namespace media {

// Mocked subclass of AudioRendererBase for testing purposes.
class MockAudioRendererBase : public AudioRendererBase {
 public:
  MockAudioRendererBase()
      : AudioRendererBase() {}
  virtual ~MockAudioRendererBase() {}

  // AudioRenderer implementation.
  MOCK_METHOD1(SetVolume, void(float volume));

  // AudioRendererBase implementation.
  MOCK_METHOD3(OnInitialize, bool(int, ChannelLayout, int));
  MOCK_METHOD0(OnStop, void());

  // Used for verifying check points during tests.
  MOCK_METHOD1(CheckPoint, void(int id));

 private:
  FRIEND_TEST(AudioRendererBaseTest, OneCompleteReadCycle);
  FRIEND_TEST(AudioRendererBaseTest, Underflow);

  DISALLOW_COPY_AND_ASSIGN(MockAudioRendererBase);
};

class AudioRendererBaseTest : public ::testing::Test {
 public:
  // Give the decoder some non-garbage media properties.
  AudioRendererBaseTest()
      : renderer_(new MockAudioRendererBase()),
        decoder_(new MockAudioDecoder()),
        pending_reads_(0) {
    renderer_->set_host(&host_);

    // Queue all reads from the decoder.
    EXPECT_CALL(*decoder_, ProduceAudioSamples(_))
        .WillRepeatedly(Invoke(this, &AudioRendererBaseTest::EnqueueCallback));

    // Set up audio properties.
    ON_CALL(*decoder_, bits_per_channel())
        .WillByDefault(Return(16));
    ON_CALL(*decoder_, channel_layout())
        .WillByDefault(Return(CHANNEL_LAYOUT_MONO));
    ON_CALL(*decoder_, samples_per_second())
        .WillByDefault(Return(44100));

    EXPECT_CALL(*decoder_, bits_per_channel())
        .Times(AnyNumber());
    EXPECT_CALL(*decoder_, channel_layout())
        .Times(AnyNumber());
    EXPECT_CALL(*decoder_, samples_per_second())
        .Times(AnyNumber());
  }

  virtual ~AudioRendererBaseTest() {
    // Expect a call into the subclass.
    EXPECT_CALL(*renderer_, OnStop());
    renderer_->Stop(NewExpectedClosure());
  }

  MOCK_METHOD0(OnUnderflow, void());

  base::Closure NewUnderflowClosure() {
    return base::Bind(&AudioRendererBaseTest::OnUnderflow,
                      base::Unretained(this));
  }

  void WriteUntilNoPendingReads(int data_size, uint8 value,
                                uint32* bytes_buffered) {
    while (pending_reads_ > 0) {
      scoped_refptr<DataBuffer> buffer(new DataBuffer(data_size));
      buffer->SetDataSize(data_size);
      memset(buffer->GetWritableData(), value, buffer->GetDataSize());
      --pending_reads_;
      *bytes_buffered += data_size;
      decoder_->ConsumeAudioSamplesForTest(buffer);
    }
  }

 protected:
  static const size_t kMaxQueueSize;

  // Fixture members.
  scoped_refptr<MockAudioRendererBase> renderer_;
  scoped_refptr<MockAudioDecoder> decoder_;
  StrictMock<MockFilterHost> host_;

  // Number of asynchronous read requests sent to |decoder_|.
  size_t pending_reads_;

 private:
  void EnqueueCallback(scoped_refptr<Buffer> buffer) {
    ++pending_reads_;
  }

  DISALLOW_COPY_AND_ASSIGN(AudioRendererBaseTest);
};

const size_t AudioRendererBaseTest::kMaxQueueSize = 1u;

TEST_F(AudioRendererBaseTest, Initialize_Failed) {
  InSequence s;

  // Our subclass will fail when asked to initialize.
  EXPECT_CALL(*renderer_, OnInitialize(_, _, _))
      .WillOnce(Return(false));

  // We expect to receive an error.
  EXPECT_CALL(host_, SetError(PIPELINE_ERROR_INITIALIZATION_FAILED));

  // Initialize, we expect to have no reads.
  renderer_->Initialize(decoder_, NewExpectedClosure(), NewUnderflowClosure());
  EXPECT_EQ(0u, pending_reads_);
}

TEST_F(AudioRendererBaseTest, Initialize_Successful) {
  InSequence s;

  // Then our subclass will be asked to initialize.
  EXPECT_CALL(*renderer_, OnInitialize(_, _, _))
      .WillOnce(Return(true));

  // Initialize, we shouldn't have any reads.
  renderer_->Initialize(decoder_, NewExpectedClosure(), NewUnderflowClosure());

  EXPECT_EQ(0u, pending_reads_);

  // Now seek to trigger prerolling, verifying the callback hasn't been
  // executed yet.
  EXPECT_CALL(*renderer_, CheckPoint(0));
  renderer_->Seek(base::TimeDelta(), NewExpectedStatusCB(PIPELINE_OK));
  EXPECT_EQ(kMaxQueueSize, pending_reads_);
  renderer_->CheckPoint(0);

  // Now satisfy the read requests.  Our callback should be executed after
  // exiting this loop.
  while (pending_reads_) {
    scoped_refptr<DataBuffer> buffer(new DataBuffer(1024));
    buffer->SetDataSize(1024);
    --pending_reads_;
    decoder_->ConsumeAudioSamplesForTest(buffer);
  }
}

TEST_F(AudioRendererBaseTest, OneCompleteReadCycle) {
  InSequence s;

  // Then our subclass will be asked to initialize.
  EXPECT_CALL(*renderer_, OnInitialize(_, _, _))
      .WillOnce(Return(true));

  // Initialize, we shouldn't have any reads.
  renderer_->Initialize(decoder_, NewExpectedClosure(), NewUnderflowClosure());
  EXPECT_EQ(0u, pending_reads_);

  // Now seek to trigger prerolling, verifying the callback hasn't been
  // executed yet.
  EXPECT_CALL(*renderer_, CheckPoint(0));
  renderer_->Seek(base::TimeDelta(), NewExpectedStatusCB(PIPELINE_OK));
  EXPECT_EQ(kMaxQueueSize, pending_reads_);
  renderer_->CheckPoint(0);

  // Now satisfy the read requests.  Our callback should be executed after
  // exiting this loop.
  const uint32 kDataSize = 1024;
  uint32 bytes_buffered = 0;

  WriteUntilNoPendingReads(kDataSize, 1, &bytes_buffered);

  // Then set the renderer to play state.
  renderer_->Play(NewExpectedClosure());
  renderer_->SetPlaybackRate(1.0f);
  EXPECT_EQ(1.0f, renderer_->GetPlaybackRate());

  // Then flush the data in the renderer by reading from it.
  uint8 buffer[kDataSize];
  for (size_t i = 0; i < kMaxQueueSize; ++i) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize,
                                    base::TimeDelta(), true));
    bytes_buffered -= kDataSize;
  }

  // Make sure the read request queue is full.
  EXPECT_EQ(kMaxQueueSize, pending_reads_);

  // Fulfill the read with an end-of-stream packet.
  scoped_refptr<DataBuffer> last_buffer(new DataBuffer(0));
  decoder_->ConsumeAudioSamplesForTest(last_buffer);
  --pending_reads_;

  // We shouldn't report ended until all data has been flushed out.
  EXPECT_FALSE(renderer_->HasEnded());

  // We should have one less read request in the queue.
  EXPECT_EQ(kMaxQueueSize - 1, pending_reads_);

  // Flush the entire internal buffer and verify NotifyEnded() isn't called
  // right away.
  EXPECT_CALL(*renderer_, CheckPoint(1));
  EXPECT_EQ(0u, bytes_buffered % kDataSize);
  while (bytes_buffered > 0) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize,
                                    base::TimeDelta(), true));
    bytes_buffered -= kDataSize;
  }

  // Although we've emptied the buffer, we don't consider ourselves ended until
  // we request another buffer.  This way we know the last of the audio has
  // played.
  EXPECT_FALSE(renderer_->HasEnded());
  renderer_->CheckPoint(1);

  // Do an additional read to trigger NotifyEnded().
  EXPECT_CALL(host_, NotifyEnded());
  EXPECT_EQ(0u, renderer_->FillBuffer(buffer, kDataSize,
                                      base::TimeDelta(), true));

  // We should now report ended.
  EXPECT_TRUE(renderer_->HasEnded());

  // Further reads should return muted audio and not notify any more.
  EXPECT_EQ(0u, renderer_->FillBuffer(buffer, kDataSize,
                                      base::TimeDelta(), true));
}

TEST_F(AudioRendererBaseTest, Underflow) {
  InSequence s;

  base::TimeDelta playback_delay(base::TimeDelta::FromSeconds(1));

  // Then our subclass will be asked to initialize.
  EXPECT_CALL(*renderer_, OnInitialize(_, _, _))
      .WillOnce(Return(true));

  // Initialize, we shouldn't have any reads.
  renderer_->Initialize(decoder_, NewExpectedClosure(), NewUnderflowClosure());
  EXPECT_EQ(0u, pending_reads_);

  // Now seek to trigger prerolling, verifying the callback hasn't been
  // executed yet.
  EXPECT_CALL(*renderer_, CheckPoint(0));
  renderer_->Seek(base::TimeDelta(), NewExpectedStatusCB(PIPELINE_OK));
  EXPECT_EQ(kMaxQueueSize, pending_reads_);
  renderer_->CheckPoint(0);

  // Now satisfy the read requests.  Our callback should be executed after
  // exiting this loop.
  const uint32 kDataSize = 1024;
  uint32 bytes_buffered = 0;

  WriteUntilNoPendingReads(kDataSize, 1, &bytes_buffered);

  uint32 bytes_for_preroll = bytes_buffered;

  // Then set the renderer to play state.
  renderer_->Play(NewExpectedClosure());
  renderer_->SetPlaybackRate(1.0f);
  EXPECT_EQ(1.0f, renderer_->GetPlaybackRate());

  // Consume all of the data passed into the renderer.
  uint8 buffer[kDataSize];
  while (bytes_buffered > 0) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize, playback_delay, false));
    EXPECT_EQ(1, buffer[0]);
    bytes_buffered -= kDataSize;
  }

  // Make sure there are read requests pending.
  EXPECT_GT(pending_reads_, 0u);

  // Verify the next FillBuffer() call triggers calls the underflow callback
  // since the queue is empty.
  EXPECT_CALL(*this, OnUnderflow());
  EXPECT_CALL(*renderer_, CheckPoint(1));
  EXPECT_EQ(0u, renderer_->FillBuffer(buffer, kDataSize, playback_delay,
                                      false));
  renderer_->CheckPoint(1);

  // Verify that zeroed out buffers are being returned during the underflow.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize, playback_delay, false));
    EXPECT_EQ(0, buffer[0]);
  }

  renderer_->ResumeAfterUnderflow(false);

  // Verify we are still getting zeroed out buffers since no new data has been
  // pushed to the renderer.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize, playback_delay, false));
    EXPECT_EQ(0, buffer[0]);
  }

  // Satisfy all pending read requests.
  WriteUntilNoPendingReads(kDataSize, 2, &bytes_buffered);

  EXPECT_GE(bytes_buffered, bytes_for_preroll);

  // Verify that we are now getting the new data.
  while (bytes_buffered > 0) {
    EXPECT_EQ(kDataSize,
              renderer_->FillBuffer(buffer, kDataSize, playback_delay, false));
    EXPECT_EQ(2, buffer[0]);
    bytes_buffered -= kDataSize;
  }
}

}  // namespace media
