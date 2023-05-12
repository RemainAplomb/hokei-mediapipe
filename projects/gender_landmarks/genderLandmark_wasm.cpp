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

// #include "mediapipe/framework/formats/landmark.pb.h"

// #include "mediapipe/calculators/tflite/tflite_tensors_to_gender_and_landmarks_calculator.cc"

#include <typeinfo>

using namespace emscripten;
using namespace mediapipe;
thread_local const emscripten::val Module = emscripten::val::global("Module");


// Frame Meta Data Wasm
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
	double z;
	val visibility = val::undefined();
};
typedef std::vector<FaceLandmark> FaceLandmarkList;

// Gender Landmarks
struct FaceLandmarksContainer
{
	FaceLandmarkList landmarks_list;
};

// Contains the function that need
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

	bool isLandmarks()
	{
		if (!face_landmark_container.landmarks_list.empty())
		{
			return true;
		}
		else
		{
			return false;
		};
	}

	FaceLandmarksContainer getLandmarks()
	{
		return face_landmark_container;
	}
};
typedef std::vector<ResultWasm> ResultWasmList; // use to access the properties of the ResultWasm class

class PacketListener : public ResultWasmList
{
public:
	virtual void onResults(ResultWasmList res) = 0; // THIS SHOULD RETURN JAVASCRIPT CANVAS ELEMENT AND (478) LANDMARKS ARRAY
};

class PacketListenerWrapper : public wrapper<PacketListener>
{
public:
	EMSCRIPTEN_WRAPPER(PacketListenerWrapper);
	void onResults(ResultWasmList res)
	{
		return call<void>("onResults", res);
	}
};


// For gender landmark solution
class GenderLandmarksWasm {
private:
    int x;
    std::string y;

public:
	GLuint Texture;
    mediapipe::CalculatorGraph graph;
    mediapipe::CalculatorGraphConfig config;
    mediapipe::GlCalculatorHelper gpu_helper;

    std::pair<std::string, Packet> cb_packet;

	// mediapipe::TfliteGenderAndLandmarksCalculator gender_landmarks_calculator;

    GenderLandmarksWasm() {}

	// Create Context
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

	// Bind Texture
	void bindTextureToCanvas()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// Process Frame
	void processFrame(std::string inputStringName, FrameMetaDataWasm frameInput)
	{
		absl::Status status = _processFrame(inputStringName, frameInput);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed to run the PROCESS FRAME: " << status.message();
		}
		LOG(ERROR) << "processFrame okStatus good";
	}

	absl::Status _processFrame(std::string inputStreamName, FrameMetaDataWasm fmd)
	{
		LOG(ERROR) << "_processFrame: Start";
		// Prepare and add graph input packet.
		if (gpu_helper.Initialized())
		{
			LOG(ERROR) << "_processFrame: Start gpu_helper.Initialized()";
			// Run GPU operations within a GlContext.
			MP_RETURN_IF_ERROR(gpu_helper.RunInGlContext([&inputStreamName, &fmd, this]() -> absl::Status
				{
					LOG(ERROR) << "_processFrame: gpu_helper.RunInGlContext";
					// Create a destination texture to hold the processed frame data.
					auto dst = gpu_helper.CreateDestinationTexture(fmd.width, fmd.height, GpuBufferFormat::kBGRA32);  
					auto fbo = gpu_helper.framebuffer();
					
					LOG(ERROR) << "_processFrame: glViewport(0, 0,fmd.width, fmd.height)";
					// Set the viewport to the dimensions of the frame data.
					glViewport(0, 0,fmd.width, fmd.height);
					glBindFramebuffer(GL_FRAMEBUFFER, fbo);
					
					LOG(ERROR) << "_processFrame: glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);";
					// Bind the texture to the GL_COLOR_ATTACHMENT0 slot and copy the data from the texture.
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);
					glBindTexture(dst.target(), dst.name());
					glCopyTexSubImage2D(dst.target(), 0, 0, 0, 0, 0, dst.width(), dst.height());
					
					LOG(ERROR) << "_processFrame: auto gpu_frame = dst.GetFrame<mediapipe::GpuBuffer>();";
					// Get the processed frame as a GpuBuffer.
					auto gpu_frame = dst.GetFrame<mediapipe::GpuBuffer>();
					
					LOG(ERROR) << "_processFrame: MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(inputStreamName.c_str(), mediapipe::Adopt(gpu_frame.release()).At(mediapipe::Timestamp(fmd.timestampMs))));";
					// Add the processed frame to the graph input stream.
					MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(inputStreamName.c_str(), mediapipe::Adopt(gpu_frame.release()).At(mediapipe::Timestamp(fmd.timestampMs))));
					glFlush();
					
					LOG(ERROR) << "_processFrame: gpu_helper.RunInGlContext";
					// Release the texture to avoid memory leaks.
					dst.Release();
					return graph.WaitUntilIdle();
				}));
		}
		return absl::OkStatus();
	}


    // Change Options
    void changeOptions() {}

    // Delete Graph
    void deleteGraph() {}

    // Load Graph
    absl::Status _loadGrap(std::string calculator_graph_config_file)
    {
        config.ParseFromArray(calculator_graph_config_file.c_str(), calculator_graph_config_file.length());
		glGenTextures(1, &Texture);
        return absl::OkStatus();
    }

    void loadGraph(std::string calculator_graph_config_file)
	{
		absl::Status status = _loadGrap(calculator_graph_config_file);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed to run the graph: " << status.message();
		}
	}

	void attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		absl::Status status = _attachMultiListener(wantsVector, packetListener);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed at attachMultiListener: " << status.message();
		}
	}

    // Attach Multi Listener
    absl::Status _attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		LOG(ERROR) << "wehhhhhh: ";
		std::function<void(const std::vector<Packet>&)> callback = [&packetListener](const std::vector<Packet>& packets)
		{
			LOG(ERROR) << "wahhhhhh: ";
			ResultWasmList result_wasm_list;
			ResultWasm result_wasm;

			LOG(ERROR) << "wuhhhhhh: ";

			const Packet& output_tensor_values_p = packets[0];
			LOG(ERROR) << "output_tensor_values_p: " << output_tensor_values_p;
			if (!output_tensor_values_p.IsEmpty()) {
				const auto& output_tensor_values = output_tensor_values_p.Get<std::vector<Packet>>();

				// Extract gender and landmarks result from output_tensor_values
				const auto& gender_p = output_tensor_values[0];
				const auto& landmarks_p = output_tensor_values[1];

				FaceLandmarkList face_landmark_list;

				if (!gender_p.IsEmpty()) {
					const auto& gender_result = gender_p.Get<float>();
					// Process gender result here
				}

				if (!landmarks_p.IsEmpty()) {
					const auto& landmarks_data = landmarks_p.Get<std::vector<float>>();
					for (int i = 0; i < landmarks_data.size() / 2; ++i) {
						FaceLandmark landmark_data;
						landmark_data.x = landmarks_data[i * 2];
						landmark_data.y = landmarks_data[i * 2 + 1];
						landmark_data.z = 0;
						landmark_data.visibility = val::undefined();
						result_wasm.face_landmark_container.landmarks_list.push_back(landmark_data);
					}
					result_wasm.canvas = 0;
					result_wasm_list.push_back(result_wasm);
				}
				else
				{
					result_wasm.canvas = 0;
					result_wasm_list.push_back(result_wasm);
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
};

void createContext(val glCanvas, bool useWebGL, bool setInModule, emscripten::val obj)
{
	GenderLandmarksWasm wasm;
	absl::Status status = wasm.createContext(glCanvas, useWebGL, setInModule, obj);
	if (!status.ok())
	{
		LOG(ERROR) << "Failed to create context: " << status.message();
	}
}

EMSCRIPTEN_BINDINGS(TfliteGenderAndLandmarksCalculator) {
	function("createContext", &createContext);

	register_vector<std::string>("StringList");

	register_vector<ResultWasm>("ResultWasmList");
	
	class_<GenderLandmarksWasm>("GenderLandmarksWasm")
        .constructor<>()
        .function("changeOptions", &GenderLandmarksWasm::changeOptions)
        .function("deleteGraph", &GenderLandmarksWasm::deleteGraph)
        .function("loadGraph", &GenderLandmarksWasm::loadGraph)
		.function("bindTextureToCanvas", &GenderLandmarksWasm::bindTextureToCanvas)
		.function("processFrame", &GenderLandmarksWasm::processFrame)
        .function("attachMultiListener", &GenderLandmarksWasm::attachMultiListener, allow_raw_pointers());

    value_object<FaceLandmark>("FaceLandmark")
        .field("x", &FaceLandmark::x)
        .field("y", &FaceLandmark::y)
        .field("z", &FaceLandmark::z)
        .field("visibility", &FaceLandmark::visibility);
	
	// value_object<FaceLandmarksContainer>("FaceLandmarksContainer")
    // 	.field("landmarks_list", &FaceLandmarksContainer::landmarks_list);

	value_object<FrameMetaDataWasm>("FrameMetaDataWasm")
		.field("width", &FrameMetaDataWasm::width)
		.field("height", &FrameMetaDataWasm::height)
		.field("timestampMs", &FrameMetaDataWasm::timestampMs);
	
	class_<ResultWasm>("ResultWasm")
        .function("isImage", &ResultWasm::isImage)
        .function("getImage", &ResultWasm::getImage)
        .function("isLandmarks", &ResultWasm::isLandmarks)
        .function("getLandmarks", &ResultWasm::getLandmarks);
	
	class_<PacketListener, base<ResultWasmList>>("PacketListener")
        .function("onResults", &PacketListener::onResults, pure_virtual())
        .allow_subclass<PacketListenerWrapper>("PacketListenerWrapper");

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