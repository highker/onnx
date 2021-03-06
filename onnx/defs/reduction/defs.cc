// Copyright (c) Facebook Inc. and Microsoft Corporation.
// Licensed under the MIT license.

#include <algorithm>
#include <functional>
#include "onnx/defs/schema.h"

namespace ONNX_NAMESPACE {

std::function<void(OpSchema&)> ReduceDocGenerator(const char* name) {
  return [=](OpSchema& schema) {
    std::string doc = R"DOC(
Computes the {name} of the input tensor's element along the provided axes. The resulted
tensor has the same rank as the input if keepdims equal 1. If keepdims equal 0, then
the resulted tensor have the reduced dimension pruned.

The above behavior is similar to numpy, with the exception that numpy default keepdims to
False instead of True.)DOC";
    ReplaceAll(doc, "{name}", name);
    schema.SetDoc(doc);
    schema.Attr(
        "axes",
        "A list of integers, along which to reduce. The default is to reduce over "
        "all the dimensions of the input tensor.",
        AttributeProto::INTS,
        OPTIONAL);
    schema.Attr(
        "keepdims",
        "Keep the reduced dimension or not, default 1 mean keep reduced dimension.",
        AttributeProto::INT,
        static_cast<int64_t>(1));
    schema.Input(0, "data", "An input tensor.", "T");
    schema.Output(0, "reduced", "Reduced output tensor.", "T");
    schema.TypeConstraint(
        "T",
        OpSchema::high_precision_numeric_types(),
        "Constrain input and output types to high-precision numeric tensors.");
    schema.TypeAndShapeInferenceFunction([](InferenceContext& ctx) {
      propagateElemTypeFromInputToOutput(ctx, 0, 0);
      if (!hasNInputShapes(ctx, 1)) {
        return;
      }

      int64_t keep_dims = 1;
      auto attr_proto = ctx.getAttribute("keepdims");
      if (attr_proto) {
        keep_dims = attr_proto->i();
      }
      auto& input_shape = ctx.getInputType(0)->tensor_type().shape();
      int64_t input_ndim = input_shape.dim_size();
      auto output_shape =
          ctx.getOutputType(0)->mutable_tensor_type()->mutable_shape();
      std::vector<int64_t> axes;
      auto axes_proto = ctx.getAttribute("axes");
      if (axes_proto)
        axes.assign(axes_proto->ints().begin(), axes_proto->ints().end());

      for (size_t i = 0; i < axes.size(); ++i) {
        if (axes[i] < 0)
          axes[i] += input_ndim;
      }
      // do we need handle negative axis?
      for (int i = 0; i < input_ndim; ++i) {
        // axes empty means reduce all dim
        if (!axes.empty() &&
            std::find(axes.begin(), axes.end(), i) == axes.end()) {
          auto dim = output_shape->add_dim();
          dim->CopyFrom(input_shape.dim(i));
        } else {
          if (keep_dims == 1) {
            auto dim = output_shape->add_dim();
            dim->set_dim_value(1);
          }
        }
      }
    });
  };
}

ONNX_OPERATOR_SCHEMA(ReduceMax).FillUsing(ReduceDocGenerator("max"));

ONNX_OPERATOR_SCHEMA(ReduceMin).FillUsing(ReduceDocGenerator("min"));

ONNX_OPERATOR_SCHEMA(ReduceSum).FillUsing(ReduceDocGenerator("sum"));

ONNX_OPERATOR_SCHEMA(ReduceSumSquare)
    .FillUsing(ReduceDocGenerator("sum square"));

ONNX_OPERATOR_SCHEMA(ReduceMean).FillUsing(ReduceDocGenerator("mean"));

ONNX_OPERATOR_SCHEMA(ReduceProd).FillUsing(ReduceDocGenerator("product"));

ONNX_OPERATOR_SCHEMA(ReduceLogSum).FillUsing(ReduceDocGenerator("log sum"));

ONNX_OPERATOR_SCHEMA(ReduceLogSumExp)
    .FillUsing(ReduceDocGenerator("log sum exponent"));

ONNX_OPERATOR_SCHEMA(ReduceL1).FillUsing(ReduceDocGenerator("L1 norm"));

ONNX_OPERATOR_SCHEMA(ReduceL2).FillUsing(ReduceDocGenerator("L2 norm"));

} // namespace ONNX_NAMESPACE

namespace ONNX_NAMESPACE {

std::function<void(OpSchema&)> ArgReduceDocGenerator(const char* name) {
  return [=](OpSchema& schema) {
    std::string doc = R"DOC(
Computes the indices of the {name} elements of the input tensor's element along the 
provided axis. The resulted tensor has the same rank as the input if keepdims equal 1.
If keepdims equal 0, then the resulted tensor have the reduced dimension pruned. 
The type of the output tensor is integer.)DOC";
    ReplaceAll(doc, "{name}", name);
    schema.SetDoc(doc);
    schema.Attr(
        "axis",
        "The axis in which to compute the arg indices. Default is 0.",
        AttributeProto::INT,
        static_cast<int64_t>(0));
    schema.Attr(
        "keepdims",
        "Keep the reduced dimension or not, default 1 mean keep reduced dimension.",
        AttributeProto::INT,
        static_cast<int64_t>(1));
    schema.Input(0, "data", "An input tensor.", "T");
    schema.Output(
        0,
        "reduced",
        "Reduced output tensor with integer data type.",
        "tensor(int64)");
    schema.TypeConstraint(
        "T",
        OpSchema::all_numeric_types(),
        "Constrain input and output types to all numeric tensors.");
    schema.TypeAndShapeInferenceFunction([](InferenceContext& ctx) {
      // set output element type to int64
      auto output_type = ctx.getOutputType(0);
      if (output_type->value_case() == TypeProto::kTensorType ||
          output_type->value_case() == TypeProto::VALUE_NOT_SET) {
        output_type->mutable_tensor_type()->set_elem_type(
            TensorProto_DataType_INT64);
      }
      if (!hasNInputShapes(ctx, 1)) {
        return;
      }

      auto& input_shape = ctx.getInputType(0)->tensor_type().shape();
      auto output_shape =
          ctx.getOutputType(0)->mutable_tensor_type()->mutable_shape();
      int64_t input_ndim = input_shape.dim_size();
      int64_t axis = 0; // default to 0
      auto axis_proto = ctx.getAttribute("axis");
      if (axis_proto) {
        axis = axis_proto->i();
        if (axis < 0)
          axis += input_ndim;
      }

      int64_t keep_dims = 1;
      auto attr_proto = ctx.getAttribute("keepdims");
      if (attr_proto) {
        keep_dims = attr_proto->i();
      }
      // do we need handle negative axis?
      for (int i = 0; i < input_ndim; ++i) {
        if (i != axis) {
          auto dim = output_shape->add_dim();
          dim->CopyFrom(input_shape.dim(i));
        } else {
          if (keep_dims == 1) {
            auto dim = output_shape->add_dim();
            dim->set_dim_value(1);
          }
        }
      }
    });
  };
} // namespace ONNX_NAMESPACE

ONNX_OPERATOR_SCHEMA(ArgMax).FillUsing(ArgReduceDocGenerator("max"));

ONNX_OPERATOR_SCHEMA(ArgMin).FillUsing(ArgReduceDocGenerator("min"));

} // namespace ONNX_NAMESPACE
