load( "//mediapipe/framework/tool:mediapipe_graph.bzl", "mediapipe_binary_graph")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")


package(default_visibility = ["//visibility:public"])

exports_files( 
    srcs = glob(["**"]),
)

cc_binary(
    name = "face_mesh_hokei_solution_wasm_bin",
    srcs = [
        "my_face_mesh.cpp",
    ],
    visibility = ["//visibility:public"],
    linkopts = [
        # "-O0 -g -fdebug-compilation-dir='..'", #debuging information
        "-O3",
        "--bind",
        "-s WASM=1",
        #"-s ASYNCIFY=1",
        "-s FULL_ES3",
        "-s MAX_WEBGL_VERSION=2",
        "-s MIN_WEBGL_VERSION=2",
        # "-s EXPORT_ALL=1",
        "-s USE_WEBGL2=1",

        # "-pthread",
        "-s USE_PTHREADS=1",
        # "-s ALLOW_MEMORY_GROWTH=1",
        "-s TOTAL_MEMORY=134217728",

        "-s ASSERTIONS=1",

        "-s MODULARIZE=1",  #Allows us to manually invoke the initializatio of wasm
        "-s EXPORT_ES6=0",  #Export using an ES6 Module export rather than a UMD export. MODULARIZE must be enabled for ES6 exports.
        "-s STANDALONE_WASM=0", 
        "-s FORCE_FILESYSTEM=1",

        "-s USE_ES6_IMPORT_META=0", #Disable or enable the use of import.meta in JavaScript ouput file
        "-s ERROR_ON_UNDEFINED_SYMBOLS=0",
        "-s RESERVED_FUNCTION_POINTERS=1",
        "-s EXPORT_NAME=createMediapipeSolutionsWasm", #Customise the export name of the JavaScript output file
    ],
    deps = [
        "//mediapipe/gpu:gpu_buffer",
        "//mediapipe/gpu:gpu_shared_data_internal",
        "//mediapipe/gpu:gl_calculator_helper",
        "//mediapipe/gpu:gl_simple_calculator",
        "//mediapipe/gpu:gl_scaler_calculator",

        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//mediapipe/calculators/core:flow_limiter_calculator",
        
        # CANVAS_CALCULATOR
        "//mediapipe/calculators/util:render_gpu_buffer_to_canvas_calculator",
        # FACE_LANDMARKS
        "//mediapipe/calculators/core:constant_side_packet_calculator",
        "//mediapipe/graphs/face_mesh/subgraphs:face_renderer_gpu",
        "//mediapipe/modules/face_landmark:face_landmark_front_gpu",
        # FACE_GEOMETRY
        "//mediapipe/modules/face_geometry:env_generator_calculator",
        "//mediapipe/modules/face_geometry:face_geometry_from_landmarks",
        # GOOGLE
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "face_mesh_hokei_solution_simd_wasm_bin",
    srcs = [
        "face_detection.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/gpu:gpu_buffer",
        "//mediapipe/gpu:gpu_shared_data_internal",
        "//mediapipe/gpu:gl_calculator_helper",
        "//mediapipe/gpu:gl_simple_calculator",
        "//mediapipe/gpu:gl_simple_shaders",
        "//mediapipe/gpu:gl_scaler_calculator",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//mediapipe/calculators/image:image_properties_calculator",
        "//mediapipe/culators/core:constant_side_packet_calculator",
        # FACE LANDMARKS
        "//mediapipe/modules/face_landmark:face_landmark_front_gpu",
        #FACE GEOMETRY
        "//mediapipe/modules/face_geometry/protos:face_geometry_cc_proto",
        "//mediapipe/modules/face_geometry:face_geometry_from_landmarks",
        "//mediapipe/modules/face_geometry:geometry_pipeline_calculator",
        "//mediapipe/modules/face_geometry:env_generator_calculator",
        #CANVAS_CALCULATOR
        "//mediapipe/calculators/util:render_gpu_buffer_to_canvas_calculator",
        # GOOGLE
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
     linkopts = [
         # "-O0 -g -fdebug-compilation-dir='..'", #debuging information
        "-O3",
        "--bind",
        "-s WASM=1",
        #"-s ASYNCIFY=1",
       
        # "-pthread",
        "-s USE_PTHREADS=0",
        "-s ALLOW_MEMORY_GROWTH=1",
        "-s ASSERTIONS=1",

        "-s USE_WEBGL2=1",
        "-s FULL_ES3",
        "-s MAX_WEBGL_VERSION=2",
        "-s MIN_WEBGL_VERSION=2",
        "-s GL_UNSAFE_OPTS=1",
        
        "-s OFFSCREEN_FRAMEBUFFER=2",
        "-s OFFSCREENCANVAS_SUPPORT=2",
        "-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0",
        
        "-s MODULARIZE=1",  #Allows us to manually invoke the initializatio of wasm
        "-s EXPORT_ES6=0",  #Export using an ES6 Module export rather than a UMD export. MODULARIZE must be enabled for ES6 exports.
        "-s STANDALONE_WASM=0", 
        "-s FORCE_FILESYSTEM=1",

        "-s USE_ES6_IMPORT_META=0", #Disable or enable the use of import.meta in JavaScript ouput file
        "-s ERROR_ON_UNDEFINED_SYMBOLS=0",
        "-s RESERVED_FUNCTION_POINTERS=1",
        "-s EXPORT_NAME=createMediapipeSolutionsWasm", #Customise the export name of the JavaScript output file
    ],
)



