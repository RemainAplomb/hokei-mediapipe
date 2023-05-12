binary:
	bazel build //mediapipe/graphs/face_mesh:face_mesh_hokei_binary
build:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt -O3 --copt -flto //projects/face_mesh_web:face_mesh_hokei_solution_wasm_bin --config=wasm
#--copt='-experimental-wasm-simd' 
build-simd:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt -experimental-wasm-simd --copt -msimd128  --copt -O3 --copt -flto //projects/face_mesh_web:face_mesh_hokei_solution_simd_wasm_bin --config=wasm

preload:	
	python3 ~/emsdk/upstream/emscripten/tools/file_packager.py  face_mesh/face_mesh_hokei_solution_packed_assets.data --preload ~/ruent/mediapipe-main/mediapipe/modules/face_detection/face_detection_short_range.tflite@mediapipe/modules/face_detection/face_detection_short_range.tflite --preload ~/ruent/mediapipe-main/mediapipe/modules/face_landmark/face_landmark_with_attention.tflite@mediapipe/modules/face_landmark/face_landmark_with_attention.tflite --preload ~/ruent/mediapipe-main/mediapipe/modules/face_landmark/face_landmark.tflite@mediapipe/modules/face_landmark/face_landmark.tflite --preload ~/ruent/mediapipe-main/mediapipe/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb@mediapipe/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb --js-output="face_mesh/face_mesh_hokei_solution_packed_assets_loader.js"  --export-name=createMediapipeSolutionsPackedAssets

preload-face-mesh:	
	python3 ~/Documents/emsdk/upstream/emscripten/tools/file_packager.py face_mesh/face_mesh_hokei_solution_packed_assets.data --preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_detection/face_detection_short_range.tflite@mediapipe/modules/face_detection/face_detection_short_range.tflite --preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_landmark/face_landmark_with_attention.tflite@mediapipe/modules/face_landmark/face_landmark_with_attention.tflite --preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_landmark/face_landmark.tflite@mediapipe/modules/face_landmark/face_landmark.tflite --preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb@mediapipe/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb --js-output="face_mesh/face_mesh_hokei_solution_packed_assets_loader.js" --export-name=createMediapipeSolutionsPackedAssets

preload-face-mesh-2:
	python3 ~/Documents/emsdk/upstream/emscripten/tools/file_packager.py face_mesh/dist/face_mesh_hokei_solution_packed_assets.data \
	--preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_detection/face_detection_short_range.tflite@./face_detection_short_range.tflite \
	--preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_landmark/face_landmark_with_attention.tflite@./face_landmark_with_attention.tflite \
	--preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_landmark/face_landmark.tflite@./face_landmark.tflite \
	--preload /mnt/e/Anaconda/sdk/mediapipe-main/mediapipe/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb@./geometry_pipeline_metadata_landmarks.binarypb \
	--js-output="face_mesh/dist/face_mesh_hokei_solution_packed_assets_loader.js" --export-name=createMediapipeSolutionsPackedAssets

# python3 ~/Documents/CPP/emsdk/upstream/emscripten/tools/file_packager.py face_mesh/face_mesh_hokei_solution_packed_assets.data --preload ~/Documents/CPP/mediapipe-for-vto/third_party/modules/face_detection/face_detection_short_range.tflite@mediapipe/third_party/modules/face_detection/face_detection_short_range.tflite --preload ~/Documents/CPP/mediapipe-for-vto/third_party/modules/face_landmark/face_landmark_with_attention.tflite@mediapipe/third_party/modules/face_landmark/face_landmark_with_attention.tflite --preload ~/Documents/CPP/mediapipe-for-vto/third_party/modules/face_landmark/face_landmark_with.tflite@mediapipe/third_party/modules/face_landmark/face_landmark.tflite --preload ~/Documents/CPP/mediapipe-for-vto/third_party/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb@mediapipe/third_party/modules/face_geometry/data/geometry_pipeline_metadata_landmarks.binarypb --js-output="face_mesh/face_mesh_hokei_solution_packed_assets_loader.js"  --export-name=createMediapipeSolutionsPackedAssets
# Lips Segmentation
build-lips-segmentation:
	bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 mediapipe/examples/desktop/lip_segmentation:lips_segmentation_cpu
build-hand-tracking:
	bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 mediapipe/examples/desktop/hand_tracking:hand_tracking_desktop

# Gender and Landmarks Wasm
build-gender-landmarks-standalone:
	bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 mediapipe/examples/desktop/gender_and_landmarks:gender_and_landmarks_cpu
gender-landmarks-binary:
	bazel build //mediapipe/graphs/gender_and_landmarks:gender_landmarks_binary
build-gender-landmarks-wasm:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt -O3 --copt -flto //projects/gender_landmarks:gender_landmark_solution_wasm_bin --config=wasm --verbose_failures
build-gender-landmarks-wasm-2:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 //projects/gender_landmarks:gender_landmark_solution_wasm_bin --config=wasm
preload-gender-landmarks-wasm:	
	python3 ~/Documents/emsdk/upstream/emscripten/tools/file_packager.py gender_landmarks/gender_landmark_solution_packed_assets.data --preload /mnt/e/anaconda/sdk/mediapipe-main/mediapipe/models/gender_landmarks.tflite@mediapipe/models/gender_landmarks.tflite --js-output="gender_landmarks/dist/gender_landmark_solution_packed_assets_loader.js"  --export-name=createMediapipeSolutionsPackedAssets
#python3 ~/Documents/emsdk/upstream/emscripten/tools/file_packager.py gender_landmarks/gender_landmarks_solution_packed_assets.data --preload /mnt/e/anaconda/sdk/mediapipe-main/mediapipe/models/gender_landmarks.tflite@//mediapipe/models/gender_landmarks.tflite --export-name=createMediapipeSolutionsPackedAssets

forcefilesystem-gender-landmarks-wasm:
	bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 --define='MEDIAPIPE_DISABLE_GPU=1' --copt=-mfpmath=both --copt=-msse4.2 --copt=-mpopcnt //projects/gender_landmarks:gender_landmark_solution_wasm_bin -s FORCE_FILESYSTEM=1
run-server:
	python3 -m http.server 8000 --bind localhost


#python3 -m http.server 8000 --bind localhost --directory /mnt/e/Anaconda/sdk/mediapipe-main/face_mesh/dist --certfile /mnt/e/Anaconda/sdk/mediapipe-main/server.crt --keyfile /mnt/e/Anaconda/sdk/mediapipe-main/server.key
#python3 -m http.server 8000 --bind localhost --directory /mnt/e/Anaconda/sdk/mediapipe-main/face_mesh/dist --cert /mnt/e/Anaconda/sdk/mediapipe-main/server.crt --key /mnt/e/Anaconda/sdk/mediapipe-main/server.key
#python3 -m http.server --bind localhost 8000 --directory /mnt/e/Anaconda/sdk/mediapipe-main/face_mesh/dist --certfile server.crt --keyfile server.key
# python3 -m http.server 8000 --bind localhost --directory /mnt/e/Anaconda/sdk/mediapipe-main/face_mesh/dist/index.html --ssl-cert server.crt --ssl-key server.key



# FACE DETECTION
face-detection-binary:
	bazel build //mediapipe/graphs/face_detection:face_detection_hokei_binary
face-detection-build:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt='-DNDEBUG' --copt -O3 --copt -flto //projects/face_detection:face_mesh_hokei_solution_wasm_bin --config=wasm
face-detection-build-simd:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11  --copt -experimental-wasm-simd --copt -msimd128  --copt -O3 --copt -flto --copt='-DNDEBUG' --define EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK=1 //projects/face_detection:face_mesh_hokei_solution_simd_wasm_bin --config=wasm





# FACE MESH V2#####################################################################################
build-face-binary:
	bazel build //mediapipe/graphs/face_mesh:face_mesh_web_binary
build-face:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 //projects/face_mesh_v2:face_mesh_solution_wasm_bin --config=wasm
build-face-simd:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt='-msimd128' //projects/face_mesh_v2:face_mesh_solution_simd_wasm_bin --config=wasm

build-face-preload:	
	python3 ~/Documents/emsdk/upstream/emscripten/tools/file_packager.py face_mesh/dist/face_mesh_solution_packed_assets.data --preload ~/Documents/mediapipe-for-vto/mediapipe/modules/face_detection/face_detection_short_range.tflite@mediapipe/modules/face_detection/face_detection_short_range.tflite --preload ~/Documents/mediapipe-for-vto/mediapipe/modules/face_landmark/face_landmark_with_attention.tflite@mediapipe/modules/face_landmark/face_landmark_with_attention.tflite --js-output="face_mesh/dist/face_mesh_solution_packed_assets_loader.js"  --export-name=createMediapipeSolutionsPackedAssets
# FACE MESH V2#####################################################################################




#Mr. James Projects
build-pose:
	bazel build -c opt --copt -DMESA_EGL_NO_X11_HEADERS --copt -DEGL_NO_X11 --copt='-DNDEBUG' //mediapipe/examples/wasm_stuff/rebuild_pose_tracking:pose_tracking_solution_wasm_bin --config=wasm



# INVALID_ARGUMENT: CalculatorGraph::Run() failed in Run:
# Calculator::Open() for node"facelandmarkfrontgpu__facedetectionshortrangegpu__inferencecalculator__facelandmarkfrontgpu__InferenceCalculator" failed ; Can't find file: mediapipe/modules/face_detection/face_detection_short_range.tflite