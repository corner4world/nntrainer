/**
 * Copyright (C) 2020 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * @file	loss_layer.cpp
 * @date	12 June 2020
 * @brief	This is Loss Layer Class for Neural Network
 * @see		https://github.com/nnstreamer/nntrainer
 * @author	Parichay Kapoor <pk.kapoor@samsung.com>
 * @bug		No known bugs except for NYI items
 *
 */

#include <loss_layer.h>
#include <layer.h>
#include <lazy_tensor.h>
#include <nntrainer_error.h>
#include <nntrainer_log.h>
#include <parse_util.h>
#include <util_func.h>

namespace nntrainer {

int LossLayer::initialize(bool last) {
  int status = ML_ERROR_NONE;
  if (input_dim.batch() <= 0 || input_dim.height() <= 0 ||
      input_dim.width() <= 0 || input_dim.channel() <= 0) {
    ml_loge("Error: Dimension must be greater than 0.");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!last) {
    ml_loge("Error: Loss layer, if exists, must be the layer.");
    return ML_ERROR_INVALID_PARAMETER;
  }

  this->last_layer = last;
  output_dim = input_dim;
  return status;
}

Tensor LossLayer::forwarding(Tensor output, Tensor label, int &status) {
  input = output;
  Tensor y2 = label;
  Tensor y = output;
  Tensor l;

  switch (cost) {
  case COST_MSR: {
    // y2 <- y2 - y;
    y2.subtract_i(y);

    l = y2.chain().multiply_i(y2).sum_by_batch().multiply_i(0.5).run();
  } break;
  case COST_ENTROPY_SIGMOID: {
    // @todo: change this to apply_i
    // @note: the output should be logit before applying sigmoid
    // log(1 + exp(-abs(y))) + max(y, 0)
    Tensor mid_term = y.apply(static_cast<float (*)(float)>(&std::fabs))
      .multiply(-1.0).apply(static_cast<float (*)(float)>(&std::exp))
      .add(1.0).apply(logFloat);
    mid_term = mid_term.add(mid_term.apply(relu));

    // loss = y * y2 - (log(1 + exp(-abs(y))) + max(y, 0))
    l = y2.chain().multiply_i(y)
          .add_i(mid_term)
          .multiply_i(-1.0 / y2.getWidth())
          .run()
          .sum_by_batch();
  } break;
  case COST_ENTROPY: {
    if (activation_type == ACT_SOFTMAX) {
      l = y2.chain()
            .multiply_i(y.apply(logFloat))
            .multiply_i(-1.0 / y2.getWidth())
            .run()
            .sum_by_batch();
    } else {
      status = ML_ERROR_NOT_SUPPORTED;
      ml_loge("Only support softmax for cross entropy loss");
      return y;
    }

  } break;
  case COST_UNKNOWN:
    /** intended */
  default:
    break;
  }

  updateLoss(l);
  status = ML_ERROR_NONE;
  return y;
}

void LossLayer::updateLoss(const Tensor& l) {
  float loss_sum = 0.0;
  const float *data = l.getData();

  for (int i = 0; i < l.getBatch(); i++) {
    loss_sum += data[i];
  }
  loss = loss_sum / (float)l.getBatch();
}

void LossLayer::copy(std::shared_ptr<Layer> l) {
  std::shared_ptr<LossLayer> from =
    std::static_pointer_cast<LossLayer>(l);
  this->last_layer = from->last_layer;
  this->input.copy(from->input);
  this->cost = from->cost;
  this->loss = from->loss;
}

Tensor LossLayer::backwarding(Tensor derivative, int iteration) {
  Tensor ret_derivative;
  Tensor y2 = derivative;
  Tensor y = input;

  switch (cost) {
  case COST_MSR: {
    ret_derivative = y.subtract(y2);
  } break;
  case COST_ENTROPY_SIGMOID: {
    y = y.apply(sigmoid);
    ret_derivative = y.subtract(y2).multiply(1.0 / y.getWidth());
  } break;
  case COST_ENTROPY: {
    ret_derivative = y.subtract(y2).multiply(1.0 / y.getWidth());
  } break;
  case COST_UNKNOWN:
  default:
    break;
  }

  return ret_derivative;
}

Tensor LossLayer::forwarding(Tensor in, int &status) {
  status = ML_ERROR_NOT_SUPPORTED;
  return in;
}

int LossLayer::setProperty(std::vector<std::string> values) {
  return ML_ERROR_NOT_SUPPORTED;
}
} /* namespace nntrainer */