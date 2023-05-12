// LANDMARKS INCLUDE
#include "mediapipe/calculators/tflite/tflite_tensors_to_landmarks_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "tensorflow/lite/interpreter.h"

// ADDED FOR WASM
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/html5_webgl.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "mediapipe/gpu/gl_context.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "mediapipe/gpu/gpu_shared_data_internal.h"
#include "mediapipe/gpu/gl_calculator_helper.h"

#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/calculator_graph.h"
#include "mediapipe/framework/calculator_framework.h"

#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include <typeinfo>

using namespace emscripten;
using namespace mediapipe;

thread_local const emscripten::val Module = emscripten::val::global("Module");


// DETECTIONS INCLUDE
// #include <unordered_map>
// #include <vector>

// #include "absl/strings/str_format.h"
// #include "absl/types/span.h"
// #include "mediapipe/calculators/tflite/tflite_tensors_to_detections_calculator.pb.h"
// #include "mediapipe/framework/calculator_framework.h"
// #include "mediapipe/framework/deps/file_path.h"
// #include "mediapipe/framework/formats/detection.pb.h"
// #include "mediapipe/framework/formats/location.h"
// #include "mediapipe/framework/formats/object_detection/anchor.pb.h"
// #include "mediapipe/framework/port/ret_check.h"
// #include "mediapipe/util/tflite/config.h"
// #include "tensorflow/lite/interpreter.h"

// #if MEDIAPIPE_TFLITE_GL_INFERENCE
// #include "mediapipe/gpu/gl_calculator_helper.h"
// #include "tensorflow/lite/delegates/gpu/gl/gl_buffer.h"
// #include "tensorflow/lite/delegates/gpu/gl/gl_program.h"
// #include "tensorflow/lite/delegates/gpu/gl/gl_shader.h"
// #include "tensorflow/lite/delegates/gpu/gl_delegate.h"
// #endif  // MEDIAPIPE_TFLITE_GL_INFERENCE

// #if MEDIAPIPE_TFLITE_METAL_INFERENCE
// #import <CoreVideo/CoreVideo.h>
// #import <Metal/Metal.h>
// #import <MetalKit/MetalKit.h>

// #import "mediapipe/gpu/MPPMetalHelper.h"
// #include "mediapipe/gpu/MPPMetalUtil.h"
// #include "mediapipe/gpu/gpu_buffer.h"
// #include "tensorflow/lite/delegates/gpu/metal_delegate.h"
// #endif  // MEDIAPIPE_TFLITE_METAL_INFERENCE

// namespace {
// constexpr int kNumInputTensorsWithAnchors = 3;
// constexpr int kNumCoordsPerBox = 4;

// constexpr char kTensorsTag[] = "TENSORS";
// constexpr char kTensorsGpuTag[] = "TENSORS_GPU";
// }  // namespace


//  Initializes logging for the program using the Google Logging Library
int main(int argc, char **argv)
{
	FLAGS_logtostderr = 1;
	google::LogToStderr();
	google::InitGoogleLogging(argv[0]);
}

// Dom't know yet what this is for
struct SetOptions{

};

// This struct is used to represent metadata about a video frame, such as its size and timestamp.
struct FrameMetaDataWasm
{
	int width;
	int height;
	float timestampMs;
};

// LANDMARK Structure
struct FaceLandmark
{
	double x;
	double y;
	double z; // I don't have this
	val visibility = val::undefined();
};

typedef std::vector<FaceLandmark> FaceLandmarkList;

struct FaceLandmarksContainer
{
	FaceLandmarkList landmarks_list;
};


// This contains properties that we will need
class ResultWasm
{
public:
	int canvas = 0;

	FaceLandmarksContainer face_landmark_container;

	bool isImage()
	{
		if (canvas > 0)
		{
			return true;
		}
		else
		{
			return false;
		};
	}

	val getImage()
	{
		return Module["canvas"];
	}

	FaceLandmarksContainer getLandmarks()
	{
		return face_landmark_container;
	}
};

// This is to access the properties that we have defined in ResultWasm
typedef std::vector<ResultWasm> ResultWasmList;

// The purpose of the PacketListener class is to provide an interface for handling results from
// ResultWasm. It serves as a way to define a common interface for handling results in a flexible 
// and extensible way. By inheriting from ResultWasmList, it ensures that any class that implements 
// PacketListener can access the properties of ResultWasm class, and by defining a pure virtual function, it forces any derived class to implement the method, ensuring that it can be used in a consistent manner regardless of its implementation.
class PacketListener : public ResultWasmList
{
public:
	virtual void onResults(ResultWasmList res) = 0; // THIS SHOULD RETURN JAVASCRIPT CANVAS ELEMENT AND (478) LANDMARKS ARRAY
};

// Basically, it binds the packerlistener class of mediapipe into Javascript through Emscripten
class PacketListenerWrapper : public wrapper<PacketListener>
{
public:
	EMSCRIPTEN_WRAPPER(PacketListenerWrapper);
	void onResults(ResultWasmList res)
	{
		return call<void>("onResults", res);
	}
};



// For Wasm
class SolutionWasm
{
private:


public:
  // I will probably not need these 3
	// GLuint Texture;
	// MyClass Retouch;
	// float smoothFactor = 0;

  // graph is an instance of CalculatorGraph class from the MediaPipe
  // library, which represents a graph of MediaPipe calculators.
	mediapipe::CalculatorGraph graph;

  // config is an instance of CalculatorGraphConfig class from the MediaPipe library,
  // which represents the configuration of a MediaPipe calculator graph.
	mediapipe::CalculatorGraphConfig config;

  // gpu_helper is an instance of GlCalculatorHelper class from the MediaPipe library, 
  // which provides a set of helper functions to integrate MediaPipe calculators with OpenGL.
	mediapipe::GlCalculatorHelper gpu_helper;

  // cb_packet is a std::pair that stores a string and a Packet object from the MediaPipe library.
	std::pair<std::string, Packet> cb_packet;

  // ctx is an EMSCRIPTEN_WEBGL_CONTEXT_HANDLE that stores a handle to an Emscripten WebGL context.
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

  // The SolutionWasm constructor is defined as empty, so it initializes these member variables
  // to their default values.
	SolutionWasm() {}


  // For WebGL properties
	absl::Status createContext(val glCanvas, bool useWebGL, bool setInModule, emscripten::val obj)
	{
		// const auto contxt = glCanvas.call<emscripten::val, std::string>("getContext", "webgl");
		EmscriptenWebGLContextAttributes attrs;
		emscripten_webgl_init_context_attributes(&attrs);
		attrs.majorVersion = 2;
		attrs.renderViaOffscreenBackBuffer = EM_TRUE;
		attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_LOW_POWER;
		attrs.preserveDrawingBuffer = 1;
		attrs.depth = 0;
		attrs.stencil = 0;
		attrs.antialias = 0;
		attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
		EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_external = emscripten_webgl_create_context("#canvas", &attrs);
		emscripten_webgl_make_context_current(context_external);
		emscripten_performance_now();
		return absl::OkStatus();
	}


  //  This is a function that loads a calculator graph configuration file and creates an OpenGL texture.
  // It returns an absl::Status object indicating whether the operation was successful or not.
	absl::Status _loadGrap(std::string calculator_graph_config_file)
	{
		config.ParseFromArray(calculator_graph_config_file.c_str(), calculator_graph_config_file.length());
		glGenTextures(1, &Texture);
		return absl::OkStatus();
	}


  // Basically, this function takes in packets. Ideally, we would want it to be video packets.
  // Then, the program will check whether those video packets contains data for face landmark or geometry 
	absl::Status _attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		std::function<void(const std::vector<Packet> &)> callback = [&packetListener](std::vector<Packet> packets)
		{
			ResultWasmList result_wasm_list;
			ResultWasm result_wasm;

			Packet video_p = packets[0];
			Packet faces_p = packets[1];
			Packet facegeo_p = packets[2];
			// Packet gender_p = packets[3];

			FaceLandmark landmark_data;
			FaceLandmarkList face_landmark_list;

			if (!video_p.IsEmpty())
			{
				result_wasm.canvas = 1;
				result_wasm_list.push_back(result_wasm);
			}


			if (!faces_p.IsEmpty())
			{
				// const auto genders = gender_p.Get<std::vector<mediapipe::Detection>>();
				const auto faces = faces_p.Get<std::vector<mediapipe::NormalizedLandmarkList>>();

			//  for(const auto & gender : genders){
            //     int i = 0;
            //     for(const auto & label: gender.label()){

            //         if(label.size() == 4){
                
            //         // result_wasm.gender = 1;
            //        LOG(INFO) << "Male";
            //     }else{
            //           LOG(INFO) << "Female";
            //     }
    
            //         }
            //     }


				for (int face = 0; face < faces.size(); ++face)
				{
					for (int face_index = 0; face_index < faces[face].landmark_size(); ++face_index)
					{
						const mediapipe::NormalizedLandmark &landmark = faces[face].landmark(face_index);

						landmark_data.x = landmark.x();
						landmark_data.y = landmark.y();
						landmark_data.z = landmark.z();
						landmark_data.visibility = val::undefined();
						result_wasm.face_landmark_container.landmarks_list.push_back(landmark_data);
					}
				}
				result_wasm.canvas = 0;
				result_wasm_list.push_back(result_wasm);
			}
			else
			{
				result_wasm.canvas = 0;
				result_wasm_list.push_back(result_wasm);
			}

			if (!facegeo_p.IsEmpty())
			{
				auto multi_face_geometry = facegeo_p.Get<std::vector<face_geometry::FaceGeometry>>();
				const int num_faces = multi_face_geometry.size();
				std::vector<std::array<float, 16>> face_pose_transform_matrices(num_faces);
				for (int face_index = 0; face_index < multi_face_geometry.size(); ++face_index)
				{
					const auto &face_geometry = multi_face_geometry[face_index];
					const auto matrix_data = face_geometry.pose_transform_matrix();
					std::array<float, 16> matrix_array;
					for (int i = 0; i < 16; i++)
					{
						matrix_array[i] = matrix_data.packed_data(i);
					}

					// Matrix array must be in the OpenGL-friendly column-major order. If
					// `matrix_data` is in the row-major order, then transpose.
					if (matrix_data.layout() == MatrixData::ROW_MAJOR)
					{
						std::swap(matrix_array[1], matrix_array[4]);
						std::swap(matrix_array[2], matrix_array[8]);
						std::swap(matrix_array[3], matrix_array[12]);
						std::swap(matrix_array[6], matrix_array[9]);
						std::swap(matrix_array[7], matrix_array[13]);
						std::swap(matrix_array[11], matrix_array[14]);
					}
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[0]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[1]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[2]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[3]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[4]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[5]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[6]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[7]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[8]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[9]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[10]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[11]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[12]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[13]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[14]);
					result_wasm.face_geometry_container.face_geo_metrix.push_back(matrix_array[15]);
					result_wasm_list.push_back(result_wasm);

					// face_geo_data.face_geometry_matrix.push_back(matrix_data.packed_data(i));
					// LOG(INFO) << "Approx. distance away from camera for face" << face_index << ": " << face_geometry.pose_transform_matrix().packed_data(14) << " cm.";
				}
			}

			packetListener.onResults(result_wasm_list);
		};

		mediapipe::tool::AddMultiStreamCallback(wantsVector, callback, &config, &cb_packet);
		MP_RETURN_IF_ERROR(graph.Initialize(config));
		MP_RETURN_IF_ERROR(graph.StartRun({cb_packet}));
		gpu_helper.InitializeForTest(graph.GetGpuResources().get());
		return absl::OkStatus();
	}


  // This will create a destination texture and framebuffer, set the viewport to the dimensions of the
  // input frame, bind the framebuffer, and copy the input frame to the destination texture. The copied
  // frame is then converted into a mediapipe::GpuBuffer object and added as a packet to the input
  // stream of the MediaPipe graph using the graph.AddPacketToInputStream() function.
	absl::Status _processFrame(std::string inputStreamName, FrameMetaDataWasm fmd)
	{
		// Prepare and add graph input packet.
		if (gpu_helper.Initialized())
		{
			MP_RETURN_IF_ERROR(gpu_helper.RunInGlContext([&inputStreamName, &fmd, this]() -> absl::Status
				{
				auto dst = gpu_helper.CreateDestinationTexture( fmd.width, fmd.height, GpuBufferFormat::kBGRA32 );  
				auto fbo = gpu_helper.framebuffer();
								
				glViewport(0, 0,fmd.width, fmd.height);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);
				glBindTexture(dst.target(), dst.name());
				glCopyTexSubImage2D(dst.target(), 0, 0, 0, 0, 0, dst.width(), dst.height());
				auto gpu_frame = dst.GetFrame<mediapipe::GpuBuffer>();
				MP_RETURN_IF_ERROR( graph.AddPacketToInputStream(inputStreamName.c_str(), mediapipe::Adopt(gpu_frame.release()).At(mediapipe::Timestamp(fmd.timestampMs))));
				glFlush();
				dst.Release();
				return graph.WaitUntilIdle(); }));
		}
		return absl::OkStatus();
	}

  // This function will check whether the graph has been properly loaded.
  // In case the graph is not loaded, it will print out an error
	void loadGraph(std::string calculator_graph_config_file)
	{
		absl::Status status = _loadGrap(calculator_graph_config_file);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed to run the graph: " << status.message();
		}
	}


  // By binding the texture object to the current context,
  // subsequent OpenGL operations that reference textures will operate on the bound texture.
	void bindTextureToCanvas()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}


  // This function will check if the process frame has been properly executed.
  // Furthermore, it will also apply retouch. The smooth factor value will determine
  // the internsity of the retouch
	void processFrame(std::string inputStringName, FrameMetaDataWasm frameInput)
	{
		Retouch.setX(smoothFactor);
		absl::Status status = _processFrame(inputStringName, frameInput);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed to run the PROCESS FRAME: " << status.message();
		}
	}


  // This function will listen to the packets of specific output streams that is defined in the wantsVector
	void attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		absl::Status status = _attachMultiListener(wantsVector, packetListener);
	}

  // Maybe for deleting the graph? But it doesn't have anything in it
	void deleteGraph() {}

	//Set the smooth factor of the Retouch
	void applyRetouch(float val){
		smoothFactor = val;
	}

  // Maybe for changing options? But it doesn't have anything in it
	void changeOptions() {}
};


// If the call to createContext returns an error, it logs the error message using the LOG(ERROR)
// function. The purpose of this function is to create a context for running the solution on a
// canvas element in a web browser environment.
void createContext(val glCanvas, bool useWebGL, bool setInModule, emscripten::val obj)
{
	SolutionWasm wasm;
	absl::Status status = wasm.createContext(glCanvas, useWebGL, setInModule, obj);
	if (!status.ok())
	{
		LOG(ERROR) << "Failed to create context: " << status.message();
	}
}

EMSCRIPTEN_BINDINGS(FaceMesh)
{
	function("createContext", &createContext);

	register_vector<std::string>("StringList");

	register_vector<ResultWasm>("ResultWasmList");

	class_<MyClass>("MyClass")
    .constructor()
    .function("incrementX", &MyClass::incrementX)
    .function("setX", &MyClass::setX)
    .property("x",  &MyClass::x)
    ;

	class_<SolutionWasm>("SolutionWasm")
		.constructor()
		.function("changeOptions", &SolutionWasm::changeOptions)
		.function("deleteGraph", &SolutionWasm::deleteGraph)
		.function("loadGraph", &SolutionWasm::loadGraph)
		.function("bindTextureToCanvas", &SolutionWasm::bindTextureToCanvas)
		.function("processFrame", &SolutionWasm::processFrame)
		.function("attachMultiListener", &SolutionWasm::attachMultiListener, allow_raw_pointers())
		.function("delete", &SolutionWasm::deleteGraph)
		.function("applyRetouch", &SolutionWasm::applyRetouch);

	value_object<FaceLandmark>("FaceLandmark")
		.field("x", &FaceLandmark::x)
		.field("y", &FaceLandmark::y)
		.field("z", &FaceLandmark::z)
		.field("visibility", &FaceLandmark::visibility);

	value_object<FrameMetaDataWasm>("FrameMetaDataWasm")
		.field("width", &FrameMetaDataWasm::width)
		.field("height", &FrameMetaDataWasm::height)
		.field("timestampMs", &FrameMetaDataWasm::timestampMs);

	class_<ResultWasm>("ResultWasm")
		.function("isImage", &ResultWasm::isImage)
		.function("getImage", &ResultWasm::getImage)
		.function("isLandmarks", &ResultWasm::isLandmarks)
		.function("getLandmarks", &ResultWasm::getLandmarks)
		.function("isFaceGeometry", &ResultWasm::isFaceGeometry)
		.function("getFaceGeometry", &ResultWasm::getFaceGeometry);

	class_<PacketListener, base<ResultWasmList>>("PacketListener")
		.function("onResults", &PacketListener::onResults, pure_virtual())
		.allow_subclass<PacketListenerWrapper>("PacketListenerWrapper");

	value_array<FaceGeometryContainer>("FaceGeometryContainer")
		.element(&FaceGeometryContainer::face_geo_metrix);

	value_array<FaceGeometryMatrix>("FaceGeometryMatrix")
		.element(emscripten::index<0>())
		.element(emscripten::index<1>())
		.element(emscripten::index<2>())
		.element(emscripten::index<3>())
		.element(emscripten::index<4>())
		.element(emscripten::index<5>())
		.element(emscripten::index<6>())
		.element(emscripten::index<7>())
		.element(emscripten::index<8>())
		.element(emscripten::index<9>())
		.element(emscripten::index<10>())
		.element(emscripten::index<11>())
		.element(emscripten::index<12>())
		.element(emscripten::index<13>())
		.element(emscripten::index<14>())
		.element(emscripten::index<15>());

	value_array<FaceLandmarksContainer>("FaceLandmarksContainer")
		.element(&FaceLandmarksContainer::landmarks_list);

	value_array<FaceLandmarkList>("NormalizedLandmarkList")
		.element(emscripten::index<0>())
		.element(emscripten::index<1>())
		.element(emscripten::index<2>())
		.element(emscripten::index<3>())
		.element(emscripten::index<4>())
		.element(emscripten::index<5>())
		.element(emscripten::index<6>())
		.element(emscripten::index<7>())
		.element(emscripten::index<8>())
		.element(emscripten::index<9>())
		.element(emscripten::index<10>())
		.element(emscripten::index<11>())
		.element(emscripten::index<12>())
		.element(emscripten::index<13>())
		.element(emscripten::index<14>())
		.element(emscripten::index<15>())
		.element(emscripten::index<16>())
		.element(emscripten::index<17>())
		.element(emscripten::index<18>())
		.element(emscripten::index<19>())
		.element(emscripten::index<20>())
		.element(emscripten::index<21>())
		.element(emscripten::index<22>())
		.element(emscripten::index<23>())
		.element(emscripten::index<24>())
		.element(emscripten::index<25>())
		.element(emscripten::index<26>())
		.element(emscripten::index<27>())
		.element(emscripten::index<28>())
		.element(emscripten::index<29>())
		.element(emscripten::index<30>())
		.element(emscripten::index<31>())
		.element(emscripten::index<32>())
		.element(emscripten::index<33>())
		.element(emscripten::index<34>())
		.element(emscripten::index<35>())
		.element(emscripten::index<36>())
		.element(emscripten::index<37>())
		.element(emscripten::index<38>())
		.element(emscripten::index<39>())
		.element(emscripten::index<40>())
		.element(emscripten::index<41>())
		.element(emscripten::index<42>())
		.element(emscripten::index<43>())
		.element(emscripten::index<44>())
		.element(emscripten::index<45>())
		.element(emscripten::index<46>())
		.element(emscripten::index<47>())
		.element(emscripten::index<48>())
		.element(emscripten::index<49>())
		.element(emscripten::index<50>())
		.element(emscripten::index<51>())
		.element(emscripten::index<52>())
		.element(emscripten::index<53>())
		.element(emscripten::index<54>())
		.element(emscripten::index<55>())
		.element(emscripten::index<56>())
		.element(emscripten::index<57>())
		.element(emscripten::index<58>())
		.element(emscripten::index<59>())
		.element(emscripten::index<60>())
		.element(emscripten::index<61>())
		.element(emscripten::index<62>())
		.element(emscripten::index<63>())
		.element(emscripten::index<64>())
		.element(emscripten::index<65>())
		.element(emscripten::index<66>())
		.element(emscripten::index<67>())
		.element(emscripten::index<68>())
		.element(emscripten::index<69>())
		.element(emscripten::index<70>())
		.element(emscripten::index<71>())
		.element(emscripten::index<72>())
		.element(emscripten::index<73>())
		.element(emscripten::index<74>())
		.element(emscripten::index<75>())
		.element(emscripten::index<76>())
		.element(emscripten::index<77>())
		.element(emscripten::index<78>())
		.element(emscripten::index<79>())
		.element(emscripten::index<80>())
		.element(emscripten::index<81>())
		.element(emscripten::index<82>())
		.element(emscripten::index<83>())
		.element(emscripten::index<84>())
		.element(emscripten::index<85>())
		.element(emscripten::index<86>())
		.element(emscripten::index<87>())
		.element(emscripten::index<88>())
		.element(emscripten::index<89>())
		.element(emscripten::index<90>())
		.element(emscripten::index<91>())
		.element(emscripten::index<92>())
		.element(emscripten::index<93>())
		.element(emscripten::index<94>())
		.element(emscripten::index<95>())
		.element(emscripten::index<96>())
		.element(emscripten::index<97>())
		.element(emscripten::index<98>())
		.element(emscripten::index<99>())
		.element(emscripten::index<100>())
		.element(emscripten::index<101>())
		.element(emscripten::index<102>())
		.element(emscripten::index<103>())
		.element(emscripten::index<104>())
		.element(emscripten::index<105>())
		;
}




namespace mediapipe {

namespace {
// LANDMARKS
inline float Sigmoid(float value) { return 1.0f / (1.0f + std::exp(-value)); }

float ApplyActivation(
    ::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions::Activation
        activation,
    float value) {
  switch (activation) {
    case ::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions::SIGMOID:
      return Sigmoid(value);
      break;
    default:
      return value;
  }
}

}  // namespace


class TfLiteTensorsToLandmarksCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

 private:
  absl::Status LoadOptions(CalculatorContext* cc);
  int num_landmarks_ = 106;
  bool flip_vertically_ = false;
  bool flip_horizontally_ = false;

  // Add the following member variables to store the input tensor dimensions
  int input_tensor_width_ = 0;
  int input_tensor_height_ = 0;
  
  ::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions options_;
};
REGISTER_CALCULATOR(TfLiteTensorsToLandmarksCalculator);


absl::Status TfLiteTensorsToLandmarksCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(!cc->Inputs().GetTags().empty());
  RET_CHECK(!cc->Outputs().GetTags().empty());

  if (cc->Inputs().HasTag("TENSORS")) {
    // Modify the following line to specify the expected input tensor dimensions
    cc->Inputs().Tag("TENSORS").Set<std::vector<TfLiteTensor>>(
        {TensorShape({1, 1, 212})});
  }

  if (cc->Inputs().HasTag("FLIP_HORIZONTALLY")) {
    cc->Inputs().Tag("FLIP_HORIZONTALLY").Set<bool>();
  }

  if (cc->Inputs().HasTag("FLIP_VERTICALLY")) {
    cc->Inputs().Tag("FLIP_VERTICALLY").Set<bool>();
  }

  if (cc->InputSidePackets().HasTag("FLIP_HORIZONTALLY")) {
    cc->InputSidePackets().Tag("FLIP_HORIZONTALLY").Set<bool>();
  }

  if (cc->InputSidePackets().HasTag("FLIP_VERTICALLY")) {
    cc->InputSidePackets().Tag("FLIP_VERTICALLY").Set<bool>();
  }

  if (cc->Outputs().HasTag("LANDMARKS")) {
    cc->Outputs().Tag("LANDMARKS").Set<LandmarkList>();
  }

  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    cc->Outputs().Tag("NORM_LANDMARKS").Set<NormalizedLandmarkList>();
  }

  return absl::OkStatus();
}

absl::Status TfLiteTensorsToLandmarksCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  MP_RETURN_IF_ERROR(LoadOptions(cc));

  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    RET_CHECK(options_.has_input_image_height() &&
              options_.has_input_image_width())
        << "Must provide input width/height for getting normalized landmarks.";
  }
  if (cc->Outputs().HasTag("LANDMARKS") &&
      (options_.flip_vertically() || options_.flip_horizontally() ||
       cc->InputSidePackets().HasTag("FLIP_HORIZONTALLY") ||
       cc->InputSidePackets().HasTag("FLIP_VERTICALLY"))) {
    RET_CHECK(options_.has_input_image_height() &&
              options_.has_input_image_width())
        << "Must provide input width/height for using flip_vertically option "
           "when outputing landmarks in absolute coordinates.";
  }

  flip_horizontally_ =
      cc->InputSidePackets().HasTag("FLIP_HORIZONTALLY")
          ? cc->InputSidePackets().Tag("FLIP_HORIZONTALLY").Get<bool>()
          : options_.flip_horizontally();

  flip_vertically_ =
      cc->InputSidePackets().HasTag("FLIP_VERTICALLY")
          ? cc->InputSidePackets().Tag("FLIP_VERTICALLY").Get<bool>()
          : options_.flip_vertically();

  num_landmarks_ = 106; // set the number of landmarks to 106

  return absl::OkStatus();
}

absl::Status TfLiteTensorsToLandmarksCalculator::Process(
    CalculatorContext* cc) {
  // Override values if specified so.
  if (cc->Inputs().HasTag("FLIP_HORIZONTALLY") &&
      !cc->Inputs().Tag("FLIP_HORIZONTALLY").IsEmpty()) {
    flip_horizontally_ = cc->Inputs().Tag("FLIP_HORIZONTALLY").Get<bool>();
  }
  if (cc->Inputs().HasTag("FLIP_VERTICALLY") &&
      !cc->Inputs().Tag("FLIP_VERTICALLY").IsEmpty()) {
    flip_vertically_ = cc->Inputs().Tag("FLIP_VERTICALLY").Get<bool>();
  }

  if (cc->Inputs().Tag("TENSORS").IsEmpty()) {
    return absl::OkStatus();
  }

  const auto& input_tensors =
      cc->Inputs().Tag("TENSORS").Get<std::vector<TfLiteTensor>>();

  const TfLiteTensor* raw_tensor = &input_tensors[0];

  int num_values = 1;
  for (int i = 0; i < raw_tensor->dims->size; ++i) {
    num_values *= raw_tensor->dims->data[i];
  }
  const int num_dimensions = num_values / num_landmarks_;
  CHECK_GT(num_dimensions, 0);

  const float* raw_landmarks = raw_tensor->data.f;

  LandmarkList output_landmarks;

  for (int ld = 0; ld < num_landmarks_; ++ld) {
    const int offset = ld * num_dimensions;
    Landmark* landmark = output_landmarks.add_landmark();

    if (flip_horizontally_) {
      landmark->set_x(options_.input_image_width() - raw_landmarks[offset]);
    } else {
      landmark->set_x(raw_landmarks[offset]);
    }
    if (num_dimensions > 1) {
      if (flip_vertically_) {
        landmark->set_y(options_.input_image_height() -
                        raw_landmarks[offset + 1]);
      } else {
        landmark->set_y(raw_landmarks[offset + 1]);
      }
    }
    if (num_dimensions > 2) {
      landmark->set_z(raw_landmarks[offset + 2]);
    }
    if (num_dimensions > 3) {
      landmark->set_visibility(ApplyActivation(options_.visibility_activation(),
                                               raw_landmarks[offset + 3]));
    }
    if (num_dimensions > 4) {
      landmark->set_presence(ApplyActivation(options_.presence_activation(),
                                             raw_landmarks[offset + 4]));
    }
  }

  // Output normalized landmarks if required.
  if (cc->Outputs().HasTag("NORM_LANDMARKS")) {
    NormalizedLandmarkList output_norm_landmarks;
    for (int i = 0; i < output_landmarks.landmark_size(); ++i) {
      const Landmark& landmark = output_landmarks.landmark(i);
      NormalizedLandmark* norm_landmark = output_norm_landmarks.add_landmark();
      norm_landmark->set_x(landmark.x() / options_.input_image_width());
      norm_landmark->set_y(landmark.y() / options_.input_image_height());
      // Scale Z coordinate as X + allow additional uniform normalization.
      norm_landmark->set_z(landmark.z() / options_.input_image_width() /
                           options_.normalize_z());
      if (landmark.has_visibility()) {  // Set only if supported in the model.
        norm_landmark->set_visibility(landmark.visibility());
      }
      if (landmark.has_presence()) {  // Set only if supported in the model.
        norm_landmark->set_presence(landmark.presence());
      }
    }
    cc->Outputs()
        .Tag("NORM_LANDMARKS")
        .AddPacket(MakePacket<NormalizedLandmarkList>(output_norm_landmarks)
                       .At(cc->InputTimestamp()));
  }

  // Output absolute landmarks.
  if (cc->Outputs().HasTag("LANDMARKS")) {
    cc->Outputs()
        .Tag("LANDMARKS")
        .AddPacket(MakePacket<LandmarkList>(output_landmarks)
                       .At(cc->InputTimestamp()));
  }

  return absl::OkStatus();
}

absl::Status TfLiteTensorsToLandmarksCalculator::LoadOptions(
    CalculatorContext* cc) {
  // Get calculator options specified in the graph.
  options_ =
      cc->Options<::mediapipe::TfLiteTensorsToLandmarksCalculatorOptions>();
  num_landmarks_ = options_.num_landmarks();

  return absl::OkStatus();
}
}  // namespace mediapipe
