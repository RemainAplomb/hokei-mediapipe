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

#include "mediapipe/modules/face_geometry/protos/face_geometry.pb.h"
#include <typeinfo>

using namespace emscripten;
using namespace mediapipe;
using namespace face_geometry;

thread_local const emscripten::val Module = emscripten::val::global("Module");

int main(int argc, char **argv)
{
	FLAGS_logtostderr = 1;
	google::LogToStderr();
	google::InitGoogleLogging(argv[0]);
}

struct FrameMetaDataWasm
{
	int width;
	int height;
	float timestampMs;
};

// LANDMARK Structure
struct FaceLandmark
{
	float x;
	float y;
	float z;
	val visibility = val::undefined();
};

struct FaceRotation
{
	float x;
	float y;
	float z;
};

struct FaceGeometryData
{
	FaceRotation faceRot;
};

typedef std::vector<FaceLandmark> FaceLandmarkList;

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

	FaceGeometryData face_geometry_data;

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

	bool isFaceGeometry()
	{
		return false;
	}

	FaceGeometryData getFaceGeometry()
	{
		return face_geometry_data;
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

class SolutionWasm
{
private:
	static absl::StatusOr<std::array<float, 16>>

	Convert4x4MatrixDataToArrayFormat(const MatrixData &matrix_data)
	{
		RET_CHECK(matrix_data.rows() == 4 && //
				  matrix_data.cols() == 4 && //
				  matrix_data.packed_data_size() == 16)
			<< "The matrix data must define a 4x4 matrix!";

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
		return matrix_array;
	}

public:
	GLuint Texture;

	mediapipe::CalculatorGraph graph;
	mediapipe::CalculatorGraphConfig config;
	mediapipe::GlCalculatorHelper gpu_helper;
	std::pair<std::string, Packet> cb_packet;

	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

	int errCounter = 0;

	std::shared_ptr<::mediapipe::GpuResources> gpu_resources;

	SolutionWasm() {}

	absl::Status createContext(val glCanvas, bool useWebGL, bool setInModule, emscripten::val obj)
	{
		// const auto contxt = glCanvas.call<emscripten::val, std::string>("getContext", "webgl");
		EmscriptenWebGLContextAttributes attrs;
		emscripten_webgl_init_context_attributes(&attrs);
		attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
		attrs.preserveDrawingBuffer = 0;
		EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_external = emscripten_webgl_create_context("#canvas", &attrs);
		emscripten_webgl_make_context_current(context_external);
		emscripten_performance_now();
		return absl::OkStatus();
	}

	absl::Status _loadGrap(std::string calculator_graph_config_file)
	{
		config.ParseFromArray(calculator_graph_config_file.c_str(), calculator_graph_config_file.length());
		// Initialize an OpenGL texture
		glGenTextures(1, &Texture);
		return absl::OkStatus();
	}

	absl::Status _attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		std::function<void(const std::vector<Packet> &)> callback = [&packetListener](std::vector<Packet> packets)
		{
			ResultWasmList result_wasm_list;
			ResultWasm result_wasm;

			Packet video_p = packets[0];
			Packet faces_p = packets[1];
			Packet facegeo_p = packets[2];
			Packet facedetect_p = packets[3];

			FaceLandmark landmark_data;
			FaceLandmarkList face_landmark_list;

			if (!facedetect_p.IsEmpty())
			{
				auto faces = facedetect_p.Get<mediapipe::Detection>();
				printf("test\n");
			}

			if (!video_p.IsEmpty() || !facedetect_p.IsEmpty())
			{
				result_wasm.canvas = 1;
				result_wasm_list.push_back(result_wasm);
			}

			if (!faces_p.IsEmpty())
			{
				auto faces = faces_p.Get<std::vector<mediapipe::NormalizedLandmarkList>>();
				for (auto landmark : faces.data()->landmark())
				{
					landmark_data.x = landmark.x();
					landmark_data.y = landmark.y();
					landmark_data.z = landmark.z();
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

			if (!facegeo_p.IsEmpty())
			{
				auto multi_face_geometry = facegeo_p.Get<std::vector<face_geometry::FaceGeometry>>();
				const int num_faces = multi_face_geometry.size();
				std::vector<std::array<float, 16>> face_pose_transform_matrices(num_faces);
				// LOG(INFO) << "Number of face instances with landmarks: " << multi_face_geometry.size();
				for (int face_index = 0; face_index < multi_face_geometry.size(); ++face_index)
				{
					const auto &face_geometry = multi_face_geometry[face_index];

					// Extract the face pose transformation matrix.
					// ASSIGN_OR_RETURN(face_pose_transform_matrices[face_index], Convert4x4MatrixDataToArrayFormat(multi_face_geometry.data()->pose_transform_matrix()), _ << "Failed to extract the face pose transformation matrix!");
					// LOG(INFO) << "Approx. distance away from camera for face" << face_index << ": " << face_geometry.pose_transform_matrix().packed_data(14) << " cm.";
				}
			}

			// From here the result_wasm_list vector has a size of 2;
			packetListener.onResults(result_wasm_list);
		};

		mediapipe::tool::AddMultiStreamCallback(wantsVector, callback, &config, &cb_packet);
		MP_RETURN_IF_ERROR(graph.Initialize(config));
		MP_RETURN_IF_ERROR(graph.StartRun({cb_packet}));
		gpu_helper.InitializeForTest(graph.GetGpuResources().get());
		return absl::OkStatus();
	}

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
				return absl::OkStatus(); }));
		}
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

	void bindTextureToCanvas()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
	}

	void processFrame(std::string inputStringName, FrameMetaDataWasm frameInput)
	{
		absl::Status status = _processFrame(inputStringName, frameInput);
		if (!status.ok())
		{
			LOG(ERROR) << "Failed to run the PROCESS FRAME: " << status.message();
		}
	}

	void attachMultiListener(std::vector<std::string> &wantsVector, PacketListenerWrapper packetListener)
	{
		absl::Status status = _attachMultiListener(wantsVector, packetListener);
	}

	void deleteGraph() {}

	void changeOptions() {}
};

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

	class_<SolutionWasm>("SolutionWasm")
		.constructor()
		.function("loadGraph", &SolutionWasm::loadGraph)
		.function("bindTextureToCanvas", &SolutionWasm::bindTextureToCanvas)
		.function("processFrame", &SolutionWasm::processFrame)
		.function("attachMultiListener", &SolutionWasm::attachMultiListener, allow_raw_pointers())
		.function("delete", &SolutionWasm::deleteGraph);

	value_object<FaceLandmark>("FaceLandmark")
		.field("x", &FaceLandmark::x)
		.field("y", &FaceLandmark::y)
		.field("z", &FaceLandmark::x)
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
		.element(emscripten::index<106>())
		.element(emscripten::index<107>())
		.element(emscripten::index<108>())
		.element(emscripten::index<109>())
		.element(emscripten::index<110>())
		.element(emscripten::index<111>())
		.element(emscripten::index<112>())
		.element(emscripten::index<113>())
		.element(emscripten::index<114>())
		.element(emscripten::index<115>())
		.element(emscripten::index<116>())
		.element(emscripten::index<117>())
		.element(emscripten::index<118>())
		.element(emscripten::index<119>())
		.element(emscripten::index<120>())
		.element(emscripten::index<121>())
		.element(emscripten::index<122>())
		.element(emscripten::index<123>())
		.element(emscripten::index<124>())
		.element(emscripten::index<125>())
		.element(emscripten::index<126>())
		.element(emscripten::index<127>())
		.element(emscripten::index<128>())
		.element(emscripten::index<129>())
		.element(emscripten::index<130>())
		.element(emscripten::index<131>())
		.element(emscripten::index<132>())
		.element(emscripten::index<133>())
		.element(emscripten::index<134>())
		.element(emscripten::index<135>())
		.element(emscripten::index<136>())
		.element(emscripten::index<137>())
		.element(emscripten::index<138>())
		.element(emscripten::index<139>())
		.element(emscripten::index<140>())
		.element(emscripten::index<141>())
		.element(emscripten::index<142>())
		.element(emscripten::index<143>())
		.element(emscripten::index<144>())
		.element(emscripten::index<145>())
		.element(emscripten::index<146>())
		.element(emscripten::index<147>())
		.element(emscripten::index<148>())
		.element(emscripten::index<149>())
		.element(emscripten::index<150>())
		.element(emscripten::index<151>())
		.element(emscripten::index<152>())
		.element(emscripten::index<153>())
		.element(emscripten::index<154>())
		.element(emscripten::index<155>())
		.element(emscripten::index<156>())
		.element(emscripten::index<157>())
		.element(emscripten::index<158>())
		.element(emscripten::index<159>())
		.element(emscripten::index<160>())
		.element(emscripten::index<161>())
		.element(emscripten::index<162>())
		.element(emscripten::index<163>())
		.element(emscripten::index<164>())
		.element(emscripten::index<165>())
		.element(emscripten::index<166>())
		.element(emscripten::index<167>())
		.element(emscripten::index<168>())
		.element(emscripten::index<169>())
		.element(emscripten::index<170>())
		.element(emscripten::index<171>())
		.element(emscripten::index<172>())
		.element(emscripten::index<173>())
		.element(emscripten::index<174>())
		.element(emscripten::index<175>())
		.element(emscripten::index<176>())
		.element(emscripten::index<177>())
		.element(emscripten::index<178>())
		.element(emscripten::index<179>())
		.element(emscripten::index<180>())
		.element(emscripten::index<181>())
		.element(emscripten::index<182>())
		.element(emscripten::index<183>())
		.element(emscripten::index<184>())
		.element(emscripten::index<185>())
		.element(emscripten::index<186>())
		.element(emscripten::index<187>())
		.element(emscripten::index<188>())
		.element(emscripten::index<189>())
		.element(emscripten::index<190>())
		.element(emscripten::index<191>())
		.element(emscripten::index<192>())
		.element(emscripten::index<193>())
		.element(emscripten::index<194>())
		.element(emscripten::index<195>())
		.element(emscripten::index<196>())
		.element(emscripten::index<197>())
		.element(emscripten::index<198>())
		.element(emscripten::index<199>())
		.element(emscripten::index<200>())
		.element(emscripten::index<201>())
		.element(emscripten::index<202>())
		.element(emscripten::index<203>())
		.element(emscripten::index<204>())
		.element(emscripten::index<205>())
		.element(emscripten::index<206>())
		.element(emscripten::index<207>())
		.element(emscripten::index<208>())
		.element(emscripten::index<209>())
		.element(emscripten::index<210>())
		.element(emscripten::index<211>())
		.element(emscripten::index<212>())
		.element(emscripten::index<213>())
		.element(emscripten::index<214>())
		.element(emscripten::index<215>())
		.element(emscripten::index<216>())
		.element(emscripten::index<217>())
		.element(emscripten::index<218>())
		.element(emscripten::index<219>())
		.element(emscripten::index<220>())
		.element(emscripten::index<221>())
		.element(emscripten::index<222>())
		.element(emscripten::index<223>())
		.element(emscripten::index<224>())
		.element(emscripten::index<225>())
		.element(emscripten::index<226>())
		.element(emscripten::index<227>())
		.element(emscripten::index<228>())
		.element(emscripten::index<229>())
		.element(emscripten::index<230>())
		.element(emscripten::index<231>())
		.element(emscripten::index<232>())
		.element(emscripten::index<233>())
		.element(emscripten::index<234>())
		.element(emscripten::index<235>())
		.element(emscripten::index<236>())
		.element(emscripten::index<237>())
		.element(emscripten::index<238>())
		.element(emscripten::index<239>())
		.element(emscripten::index<240>())
		.element(emscripten::index<241>())
		.element(emscripten::index<242>())
		.element(emscripten::index<243>())
		.element(emscripten::index<244>())
		.element(emscripten::index<245>())
		.element(emscripten::index<246>())
		.element(emscripten::index<247>())
		.element(emscripten::index<248>())
		.element(emscripten::index<249>())
		.element(emscripten::index<250>())
		.element(emscripten::index<251>())
		.element(emscripten::index<252>())
		.element(emscripten::index<253>())
		.element(emscripten::index<254>())
		.element(emscripten::index<255>())
		.element(emscripten::index<256>())
		.element(emscripten::index<257>())
		.element(emscripten::index<258>())
		.element(emscripten::index<259>())
		.element(emscripten::index<260>())
		.element(emscripten::index<261>())
		.element(emscripten::index<262>())
		.element(emscripten::index<263>())
		.element(emscripten::index<264>())
		.element(emscripten::index<265>())
		.element(emscripten::index<266>())
		.element(emscripten::index<267>())
		.element(emscripten::index<268>())
		.element(emscripten::index<269>())
		.element(emscripten::index<270>())
		.element(emscripten::index<271>())
		.element(emscripten::index<272>())
		.element(emscripten::index<273>())
		.element(emscripten::index<274>())
		.element(emscripten::index<275>())
		.element(emscripten::index<276>())
		.element(emscripten::index<277>())
		.element(emscripten::index<278>())
		.element(emscripten::index<279>())
		.element(emscripten::index<280>())
		.element(emscripten::index<281>())
		.element(emscripten::index<282>())
		.element(emscripten::index<283>())
		.element(emscripten::index<284>())
		.element(emscripten::index<285>())
		.element(emscripten::index<286>())
		.element(emscripten::index<287>())
		.element(emscripten::index<288>())
		.element(emscripten::index<289>())
		.element(emscripten::index<290>())
		.element(emscripten::index<291>())
		.element(emscripten::index<292>())
		.element(emscripten::index<293>())
		.element(emscripten::index<294>())
		.element(emscripten::index<295>())
		.element(emscripten::index<296>())
		.element(emscripten::index<297>())
		.element(emscripten::index<298>())
		.element(emscripten::index<299>())
		.element(emscripten::index<300>())
		.element(emscripten::index<301>())
		.element(emscripten::index<302>())
		.element(emscripten::index<303>())
		.element(emscripten::index<304>())
		.element(emscripten::index<305>())
		.element(emscripten::index<306>())
		.element(emscripten::index<307>())
		.element(emscripten::index<308>())
		.element(emscripten::index<309>())
		.element(emscripten::index<310>())
		.element(emscripten::index<311>())
		.element(emscripten::index<312>())
		.element(emscripten::index<313>())
		.element(emscripten::index<314>())
		.element(emscripten::index<315>())
		.element(emscripten::index<316>())
		.element(emscripten::index<317>())
		.element(emscripten::index<318>())
		.element(emscripten::index<319>())
		.element(emscripten::index<320>())
		.element(emscripten::index<321>())
		.element(emscripten::index<322>())
		.element(emscripten::index<323>())
		.element(emscripten::index<324>())
		.element(emscripten::index<325>())
		.element(emscripten::index<326>())
		.element(emscripten::index<327>())
		.element(emscripten::index<328>())
		.element(emscripten::index<329>())
		.element(emscripten::index<330>())
		.element(emscripten::index<331>())
		.element(emscripten::index<332>())
		.element(emscripten::index<333>())
		.element(emscripten::index<334>())
		.element(emscripten::index<335>())
		.element(emscripten::index<336>())
		.element(emscripten::index<337>())
		.element(emscripten::index<338>())
		.element(emscripten::index<339>())
		.element(emscripten::index<340>())
		.element(emscripten::index<341>())
		.element(emscripten::index<342>())
		.element(emscripten::index<343>())
		.element(emscripten::index<344>())
		.element(emscripten::index<345>())
		.element(emscripten::index<346>())
		.element(emscripten::index<347>())
		.element(emscripten::index<348>())
		.element(emscripten::index<349>())
		.element(emscripten::index<350>())
		.element(emscripten::index<351>())
		.element(emscripten::index<352>())
		.element(emscripten::index<353>())
		.element(emscripten::index<354>())
		.element(emscripten::index<355>())
		.element(emscripten::index<356>())
		.element(emscripten::index<357>())
		.element(emscripten::index<358>())
		.element(emscripten::index<359>())
		.element(emscripten::index<360>())
		.element(emscripten::index<361>())
		.element(emscripten::index<362>())
		.element(emscripten::index<363>())
		.element(emscripten::index<364>())
		.element(emscripten::index<365>())
		.element(emscripten::index<366>())
		.element(emscripten::index<367>())
		.element(emscripten::index<368>())
		.element(emscripten::index<369>())
		.element(emscripten::index<370>())
		.element(emscripten::index<371>())
		.element(emscripten::index<372>())
		.element(emscripten::index<373>())
		.element(emscripten::index<374>())
		.element(emscripten::index<375>())
		.element(emscripten::index<376>())
		.element(emscripten::index<377>())
		.element(emscripten::index<378>())
		.element(emscripten::index<379>())
		.element(emscripten::index<380>())
		.element(emscripten::index<381>())
		.element(emscripten::index<382>())
		.element(emscripten::index<383>())
		.element(emscripten::index<384>())
		.element(emscripten::index<385>())
		.element(emscripten::index<386>())
		.element(emscripten::index<387>())
		.element(emscripten::index<388>())
		.element(emscripten::index<389>())
		.element(emscripten::index<390>())
		.element(emscripten::index<391>())
		.element(emscripten::index<392>())
		.element(emscripten::index<393>())
		.element(emscripten::index<394>())
		.element(emscripten::index<395>())
		.element(emscripten::index<396>())
		.element(emscripten::index<397>())
		.element(emscripten::index<398>())
		.element(emscripten::index<399>())
		.element(emscripten::index<400>())
		.element(emscripten::index<401>())
		.element(emscripten::index<402>())
		.element(emscripten::index<403>())
		.element(emscripten::index<404>())
		.element(emscripten::index<405>())
		.element(emscripten::index<406>())
		.element(emscripten::index<407>())
		.element(emscripten::index<408>())
		.element(emscripten::index<409>())
		.element(emscripten::index<410>())
		.element(emscripten::index<411>())
		.element(emscripten::index<412>())
		.element(emscripten::index<413>())
		.element(emscripten::index<414>())
		.element(emscripten::index<415>())
		.element(emscripten::index<416>())
		.element(emscripten::index<417>())
		.element(emscripten::index<418>())
		.element(emscripten::index<419>())
		.element(emscripten::index<420>())
		.element(emscripten::index<421>())
		.element(emscripten::index<422>())
		.element(emscripten::index<423>())
		.element(emscripten::index<424>())
		.element(emscripten::index<425>())
		.element(emscripten::index<426>())
		.element(emscripten::index<427>())
		.element(emscripten::index<428>())
		.element(emscripten::index<429>())
		.element(emscripten::index<430>())
		.element(emscripten::index<431>())
		.element(emscripten::index<432>())
		.element(emscripten::index<433>())
		.element(emscripten::index<434>())
		.element(emscripten::index<435>())
		.element(emscripten::index<436>())
		.element(emscripten::index<437>())
		.element(emscripten::index<438>())
		.element(emscripten::index<439>())
		.element(emscripten::index<440>())
		.element(emscripten::index<441>())
		.element(emscripten::index<442>())
		.element(emscripten::index<443>())
		.element(emscripten::index<444>())
		.element(emscripten::index<445>())
		.element(emscripten::index<446>())
		.element(emscripten::index<447>())
		.element(emscripten::index<448>())
		.element(emscripten::index<449>())
		.element(emscripten::index<450>())
		.element(emscripten::index<451>())
		.element(emscripten::index<452>())
		.element(emscripten::index<453>())
		.element(emscripten::index<454>())
		.element(emscripten::index<455>())
		.element(emscripten::index<456>())
		.element(emscripten::index<457>())
		.element(emscripten::index<458>())
		.element(emscripten::index<459>())
		.element(emscripten::index<460>())
		.element(emscripten::index<461>())
		.element(emscripten::index<462>())
		.element(emscripten::index<463>())
		.element(emscripten::index<464>())
		.element(emscripten::index<465>())
		.element(emscripten::index<466>())
		.element(emscripten::index<467>())
		// .element(emscripten::index<468>())
		// .element(emscripten::index<469>())
		// .element(emscripten::index<470>())
		// .element(emscripten::index<471>())
		// .element(emscripten::index<472>())
		// .element(emscripten::index<473>())
		// .element(emscripten::index<474>())
		// .element(emscripten::index<475>())
		// .element(emscripten::index<476>())
		// .element(emscripten::index<477>())
		;
}
