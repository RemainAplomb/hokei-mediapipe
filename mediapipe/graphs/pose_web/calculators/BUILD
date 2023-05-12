


load("//mediapipe/framework/port:build_config.bzl", "mediapipe_proto_library")
load("@bazel_skylib//lib:selects.bzl", "selects")

licenses(["notice"])

package(default_visibility = ["//visibility:private"])




mediapipe_proto_library(
    name = "tflite_webgl_inference_calculator_proto",
    srcs = ["tflite_webgl_inference_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_options_proto",
        "//mediapipe/framework:calculator_proto",
    ],
)



cc_library(
    name = "tflite_webgl_inference_calculator",
    srcs = ["tflite_webgl_inference_calculator.cc"],
    copts = select({
        "//mediapipe:ios": [
            "-x objective-c++",
            "-fobjc-arc",  # enable reference-counting
        ],
        "//conditions:default": [],
    }),
    linkopts = select({
        "//mediapipe:ios": [
            "-framework CoreVideo",
            "-framework MetalKit",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":tflite_webgl_inference_calculator_cc_proto",
        "@com_google_absl//absl/memory",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/util/tflite:config",
        "//mediapipe/util/tflite:tflite_model_loader",
        "@org_tensorflow//tensorflow/lite:framework",
        "@org_tensorflow//tensorflow/lite/delegates/xnnpack:xnnpack_delegate",
        "@org_tensorflow//tensorflow/lite/kernels:builtin_ops",
        "//mediapipe/framework/stream_handler:fixed_size_input_stream_handler",
        "//mediapipe/framework/port:ret_check",
    ] + select({
         "//conditions:default": [],
        "//mediapipe:android": [
            "//mediapipe/util/tflite:tflite_gpu_runner",
            "//mediapipe/gpu:gl_calculator_helper",
            "//mediapipe/gpu:gpu_buffer",
            "@org_tensorflow//tensorflow/lite/delegates/gpu/common:shape",
            "@org_tensorflow//tensorflow/lite/delegates/gpu:gl_delegate",
            "@org_tensorflow//tensorflow/lite/delegates/gpu/gl:gl_buffer",
            "@org_tensorflow//tensorflow/lite/delegates/gpu/gl:gl_program",
            "@org_tensorflow//tensorflow/lite/delegates/gpu/gl:gl_shader",
        ],
    }) + select({
        "//conditions:default": [],
        "//mediapipe:android": [
            "//mediapipe/util/android/file/base",
            "@org_tensorflow//tensorflow/lite/delegates/nnapi:nnapi_delegate",
        ],
    }) + select({
        "//conditions:default": [
            "//mediapipe/util:cpu_util",
        ],
    }) ,
    alwayslink = 1,
)
