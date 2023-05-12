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
    name="pose_tracking_dep",
    deps=[
        "//mediapipe/calculators/core:previous_loopback_calculator",
        "//mediapipe/calculators/core:gate_calculator",
        "//mediapipe/calculators/core:merge_calculator",
    ]
)



cc_library(
    name="pose_detection_dep",
    deps=[
        "//mediapipe/calculators/image:image_transformation_calculator",
        "//mediapipe/graphs/pose_web/calculators:tflite_webgl_inference_calculator",
        "//mediapipe/calculators/tflite:ssd_anchors_calculator",
        "//mediapipe/calculators/tflite:tflite_tensors_to_detections_calculator",
        "//mediapipe/calculators/util:non_max_suppression_calculator",
        "//mediapipe/calculators/util:detection_letterbox_removal_calculator",
        "//mediapipe/calculators/image:image_properties_calculator",
        "//mediapipe/calculators/util:alignment_points_to_rects_calculator",
        "//mediapipe/calculators/util:rect_transformation_calculator",
     
    
      ]
)




cc_library(
    name="pose_landmarks_dep",
    deps=[
        "//mediapipe/calculators/image:image_cropping_calculator",
        "//mediapipe/calculators/image:image_transformation_calculator",
        "//mediapipe/graphs/pose_web/calculators:tflite_webgl_inference_calculator",      
        "//mediapipe/calculators/core:split_vector_calculator",
        "//mediapipe/calculators/tflite:tflite_tensors_to_floats_calculator",
        "//mediapipe/calculators/util:thresholding_calculator",
        "//mediapipe/calculators/tflite:tflite_tensors_to_landmarks_calculator",
        "//mediapipe/calculators/util:landmark_letterbox_removal_calculator",
        "//mediapipe/calculators/util:landmark_projection_calculator",
        "//mediapipe/calculators/image:image_properties_calculator",
        "//mediapipe/calculators/util:landmarks_smoothing_calculator",
        # "//mediapipe/calculators/core:split_proto_list_calculator",
        "//mediapipe/calculators/util:landmarks_to_detection_calculator",
        "//mediapipe/calculators/util:alignment_points_to_rects_calculator",
        "//mediapipe/calculators/util:rect_transformation_calculator",
    ]
)



mediapipe_simple_subgraph(
    name = "pose_tracking",
    graph = "pose_tracking.pbtxt",
    register_as = "PoseTrackingSubgraph",
    deps = [
        ":pose_tracking_dep"
    ],
)

mediapipe_simple_subgraph(
    name = "pose_detection",
    graph = "pose_detection.pbtxt",
    register_as = "PoseDetectionSubgraph",
    deps = [
        ":pose_detection_dep"
    ],
)


mediapipe_simple_subgraph(
    name = "pose_landmarks",
    graph = "pose_landmarks.pbtxt",
    register_as = "PoseLandmarkSubgraph",
    deps = [
      ":pose_landmarks_dep"
    ],
)


mediapipe_simple_subgraph(
    name = "pose_renderer",
    graph = "pose_renderer.pbtxt",
    register_as = "PoseRendererSubgraph",
    deps = [
        "//mediapipe/calculators/core:split_proto_list_calculator",
        "//mediapipe/calculators/image:recolor_calculator",
        # "//mediapipe/calculators/util:annotation_overlay_calculator",
        "//mediapipe/calculators/util:detections_to_render_data_calculator",
        "//mediapipe/calculators/util:landmarks_to_render_data_calculator",
        # "//mediapipe/calculators/util:rect_to_render_data_calculator",
        # "//mediapipe/calculators/util:rect_to_render_scale_calculator",
    ],
)
