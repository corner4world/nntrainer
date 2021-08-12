// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright (C) 2021 Jihoon Lee <jhoon.it.lee@samsung.com>
 *
 * @file data_producer_common_tests.cpp
 * @date 12 July 2021
 * @brief Common test for nntrainer data producers (Param Tests)
 * @see	https://github.com/nnstreamer/nntrainer
 * @author Jihoon Lee <jhoon.it.lee@samsung.com>
 * @bug No known bugs except for NYI items
 */
#include <gtest/gtest.h>

#include <algorithm>
#include <data_producer_common_tests.h>
#include <memory>
#include <tuple>
#include <vector>

static std::tuple<std::vector<nntrainer::Tensor> /** inputs */,
                  std::vector<nntrainer::Tensor> /** labels */>
createSample(const std::vector<nntrainer::TensorDim> &input_dims,
             const std::vector<nntrainer::TensorDim> &label_dims) {
  using namespace nntrainer;
  auto populate_tensor = [](const TensorDim &dim) {
    Tensor t(dim);
    t.updateBatch(1);
    return t;
  };

  std::vector<Tensor> inputs;
  inputs.reserve(input_dims.size());

  std::vector<Tensor> labels;
  labels.reserve(label_dims.size());

  std::transform(input_dims.begin(), input_dims.end(),
                 std::back_inserter(inputs), populate_tensor);

  std::transform(label_dims.begin(), label_dims.end(),
                 std::back_inserter(labels), populate_tensor);

  return {inputs, labels};
}

/** batchwise producer tests */
/// @todo remove this
void DataProducerSemantics::SetUp() {
  auto [producerFactory, properties, input_dims_, label_dims_, validator_,
        result_] = GetParam();

  /** check if input_dims, label_dims not empty and have the same batch */
  ASSERT_FALSE(input_dims_.empty());
  ASSERT_FALSE(label_dims_.empty());
  auto b = input_dims_[0].batch();

  ASSERT_TRUE(std::all_of(input_dims_.begin(), input_dims_.end(),
                          [b](const auto &dim) { return b == dim.batch(); }));
  ASSERT_TRUE(std::all_of(label_dims_.begin(), label_dims_.end(),
                          [b](const auto &dim) { return b == dim.batch(); }));

  producer = producerFactory(properties);
  input_dims = std::move(input_dims_);
  label_dims = std::move(label_dims_);
  result = result_;
  validator = std::move(validator_);

  if (result != DataProducerSemanticsExpectedResult::SUCCESS) {
    ASSERT_EQ(validator, nullptr)
      << "Given expected result of not success, validator must be empty!";
  }
}

void DataProducerSemantics::TearDown() {}

TEST_P(DataProducerSemantics, finalize_pn) {
  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_FINALIZE) {
    EXPECT_ANY_THROW(producer->finalize(input_dims, label_dims));
  } else {
    EXPECT_NO_THROW(producer->finalize(input_dims, label_dims));
  }
}

TEST_P(DataProducerSemantics, error_once_or_not_pn) {
  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_FINALIZE) {
    return; // skip this test
  }

  auto generator = producer->finalize(input_dims, label_dims);
  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_GENERATOR_CALL) {
    EXPECT_ANY_THROW(generator());
  } else {
    EXPECT_NO_THROW(generator());
  }
}

TEST_P(DataProducerSemantics, fetch_one_epoch_or_10_iteration_pn) {
  if (result != DataProducerSemanticsExpectedResult::SUCCESS) {
    return; // skip this test
  }

  auto generator = producer->finalize(input_dims, label_dims);
  auto sz = producer->size(input_dims, label_dims);
  bool has_fixed_size = sz != nntrainer::DataProducer::SIZE_UNDEFINED;

  if (!has_fixed_size) {
    sz = 10;
  }

  auto num_iterations = sz / input_dims[0].batch();

  for (unsigned i = 0; i < num_iterations; ++i) {
    auto [last, ins, labels] = generator();

    ASSERT_FALSE(last) << " reached last at iteration: " << i << '\n';
    if (validator) {
      ASSERT_TRUE(validator(ins, labels))
        << " failed validation for iteration: " << i << '\n';
    }
  }

  if (has_fixed_size) {
    {
      auto result = generator();
      bool last = std::get<0>(result);
      EXPECT_TRUE(last);
    }

    {
      auto [last, ins, labels] = generator();
      EXPECT_TRUE(validator(ins, labels))
        << "failed last validation after one epoch\n";
      EXPECT_FALSE(last);
    }
  }
}

/********* sample tests **********/
void DataProducerSemantics_samples::TearDown() {}

void DataProducerSemantics_samples::SetUp() {
  auto [producerFactory, properties, input_dims_, label_dims_, validator_,
        result_] = GetParam();
  producer = producerFactory(properties);
  input_dims = std::move(input_dims_);
  label_dims = std::move(label_dims_);
  result = result_;
  validator = std::move(validator_);

  if (result != DataProducerSemanticsExpectedResult::SUCCESS) {
    ASSERT_EQ(validator, nullptr)
      << "Given expected result of not success, validator must be empty!";
  }
}

TEST_P(DataProducerSemantics_samples, finalize_pn) {
  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_FINALIZE) {
    EXPECT_ANY_THROW(producer->finalize_sample(input_dims, label_dims));
  } else {
    EXPECT_NO_THROW(producer->finalize_sample(input_dims, label_dims));
  }
}

TEST_P(DataProducerSemantics_samples, error_once_or_not_pn) {
  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_FINALIZE) {
    return; // skip this test
  }

  auto generator = producer->finalize_sample(input_dims, label_dims);
  auto sample_data = createSample(input_dims, label_dims);

  if (result == DataProducerSemanticsExpectedResult::FAIL_AT_GENERATOR_CALL) {
    EXPECT_ANY_THROW(
      generator(0, std::get<0>(sample_data), std::get<1>(sample_data)));
  } else {
    EXPECT_NO_THROW(
      generator(0, std::get<0>(sample_data), std::get<1>(sample_data)));
  }
}

TEST_P(DataProducerSemantics_samples, fetch_one_epoch_or_10_iteration_pn) {
  if (result != DataProducerSemanticsExpectedResult::SUCCESS) {
    return; // skip this test
  }

  auto generator = producer->finalize_sample(input_dims, label_dims);
  auto sz = producer->size_sample(input_dims, label_dims);
  bool has_fixed_size = sz != nntrainer::DataProducer::SIZE_UNDEFINED;

  if (!has_fixed_size) {
    sz = 10;
  }

  auto [inputs, labels] = createSample(input_dims, label_dims);
  for (unsigned i = 0; i < sz; ++i) {
    auto last = generator(i, inputs, labels);

    if (i == sz - 1 && has_fixed_size) {
      EXPECT_TRUE(last);
    } else {
      ASSERT_FALSE(last) << " reached last at iteration: " << i << '\n';
    }

    if (validator) {
      ASSERT_TRUE(validator(inputs, labels))
        << " failed validation for iteration: " << i << '\n';
    }
  }
}
