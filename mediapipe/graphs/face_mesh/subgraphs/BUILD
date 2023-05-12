# Copyright 2019 The MediaPipe Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load(
    "//mediapipe/framework/tool:mediapipe_graph.bzl",
    "mediapipe_simple_subgraph",
)

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "renderer_calculators",
    deps = [
        "//mediapipe/calculators/core:split_landmarks_calculator",
        "//mediapipe/calculators/core:packet_cloner_calculator",
        # "//mediapipe/calculators/util:annotation_overlay_calculator",
        "//mediapipe/calculators/util:render_gpu_buffer_to_canvas_calculator",
        "//mediapipe/calculators/util:detections_to_render_data_calculator",
        "//mediapipe/calculators/util:landmarks_to_render_data_calculator",
        "//mediapipe/calculators/util:rect_to_render_data_calculator",
        "//mediapipe/graphs/face_mesh/calculators:face_landmarks_to_render_data_calculator",
    ] + select({
        "//mediapipe:emscripten_wasm": [],
        "//conditions:default": [
            "//mediapipe/calculators/util:annotation_overlay_calculator",
        ]
    }),
)

mediapipe_simple_subgraph(
    name = "face_renderer_web",
    graph = "face_renderer_web.pbtxt",
    register_as = "FaceRendererWeb",
    deps = [
        ":renderer_calculators",
    ],
)


mediapipe_simple_subgraph(
    name = "face_renderer_gpu",
    graph = "face_renderer_gpu.pbtxt",
    register_as = "FaceRendererGpu",
    deps = [
        ":renderer_calculators",
    ],
)

mediapipe_simple_subgraph(
    name = "face_renderer_cpu",
    graph = "face_renderer_cpu.pbtxt",
    register_as = "FaceRendererCpu",
    deps = [
        ":renderer_calculators",
    ],
)


cc_library(
    name="face_tracking_dep",
    deps=[
        ":face_detection_dep",
        ":face_landmarks_dep",
        "//mediapipe/calculators/core:previous_loopback_calculator",
        "//mediapipe/calculators/core:gate_calculator",
        "//mediapipe/calculators/core:merge_calculator",
    ]
)

cc_library(
    name="face_detection_dep",
    deps=[
        "//mediapipe/modules/face_detection:face_detection_short_range_common",
        "//mediapipe/calculators/tensor:image_to_tensor_calculator",
        # "//mediapipe/calculators/tflite:tflite_webgl_inference_calculator",   
        "//mediapipe/calculators/tensor:inference_calculator",
        "//mediapipe/calculators/util:to_image_calculator",
      ]
)

cc_library(
    name="face_landmarks_dep",
    deps=[
        
    ]
)

mediapipe_simple_subgraph(
    name = "face_tracking",
    graph = "face_tracking.pbtxt",
    register_as = "FaceTrackingSubgraph",
    deps = [
        ":face_tracking_dep"
    ],
)

mediapipe_simple_subgraph(
    name = "face_detection",
    graph = "face_detection.pbtxt",
    register_as = "FaceDetectionSubgraph",
    deps = [
        ":face_detection_dep"
    ],
)


mediapipe_simple_subgraph(
    name = "face_landmarks",
    graph = "face_landmarks.pbtxt",
    register_as = "FaceLandmarkSubgraph",
    deps = [
      ":face_landmarks_dep"
    ],
)

