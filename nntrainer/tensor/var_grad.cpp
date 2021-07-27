// SPDX-License-Identifier: Apache-2.0
/**
 * Copyright (C) 2020 Parichay Kapoor <pk.kapoor@samsung.com>
 *
 * @file   var_grad.cpp
 * @date   13 November 2020
 * @see    https://github.com/nnstreamer/nntrainer
 * @author Parichay Kapoor <pk.kapoor@samsung.com>
 * @bug    No known bugs except for NYI items
 * @brief  This is Var_Grad Class for Neural Network
 *
 */

#include <util_func.h>
#include <var_grad.h>

#include <nntrainer_error.h>

namespace nntrainer {

Var_Grad::Var_Grad(const TensorDim &dim, const Tensor::Initializer init,
                   bool ng, bool alloc_now_, const std::string &name) :
  dim(dim),
  need_gradient(ng),
  alloc_now(alloc_now_),
  name(name) {
  var = std::make_shared<Tensor>(dim, alloc_now, init);
  if (need_gradient)
    grad = std::make_shared<Tensor>(dim, alloc_now, Tensor::Initializer::ZEROS);
  else
    grad = std::make_shared<Tensor>();
}

void Var_Grad::initializeVariable(const Tensor &preallocated) {
  if (!preallocated.empty()) {
    var->makeSharedDataTensor(preallocated);
  }
}

void Var_Grad::initializeGradient(const Tensor &preallocated) {
  if (!preallocated.empty()) {
    /**
     * Making a new tensor is intentional here as this tensor is not shared
     * with other layers but the internal memory is.
     */
    grad->makeSharedDataTensor(preallocated);
  }
  /**
   * No need to reset gradient here. With shared memory, each gradient setting
   * their own memory to zero is inefficient. Rather, the preallocated memory
   * must be created with zero initializer.
   */
}

void Var_Grad::initializeShared() { grad->makeSharedDataTensor(*var.get()); }

void Var_Grad::needsGradient(bool ng) {
  need_gradient = ng;
  if (need_gradient && grad->empty()) {
    bool alloc_now_ = var->isAllocated();
    grad = std::make_shared<Tensor>(var->getDim(), alloc_now_);
  }
}

} // namespace nntrainer
