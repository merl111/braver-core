/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"  // NOLINT

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/creative_ad_info.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_utils.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"
#include "bat/ads/internal/ads_unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

namespace {

const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

const uint64_t kSecondsPerDay = base::Time::kSecondsPerHour *
    base::Time::kHoursPerDay;

}  // namespace

class BatAdsAdsPerDayFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsAdsPerDayFrequencyCapTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<NiceMock<
            brave_l10n::LocaleHelperMock>>()),
        frequency_cap_(std::make_unique<AdsPerDayFrequencyCap>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
  }

  ~BatAdsAdsPerDayFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ON_CALL(*ads_client_mock_, IsEnabled())
        .WillByDefault(Return(true));

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockLoad(ads_client_mock_.get());
    MockLoadUserModelForLanguage(ads_client_mock_.get());
    MockLoadJsonSchema(ads_client_mock_.get());
    MockSave(ads_client_mock_.get());

    Initialize(ads_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<AdsPerDayFrequencyCap> frequency_cap_;
};

TEST_F(BatAdsAdsPerDayFrequencyCapTest,
    AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerDay())
      .WillByDefault(Return(2));

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerDayFrequencyCapTest,
    AllowAdIfDoesNotExceedCap) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerDay())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_->get_client(), kCreativeInstanceId,
      base::Time::kSecondsPerHour, 1);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerDayFrequencyCapTest,
    AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerDay())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_->get_client(), kCreativeInstanceId,
      kSecondsPerDay, 2);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerDayFrequencyCapTest,
    DoNotAllowAdIfExceedsCap) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerDay())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_->get_client(), kCreativeInstanceId,
      base::Time::kSecondsPerHour, 2);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
