#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/util/resource_util.h"
#include "tensorflow/lite/model.h"
#include "mediapipe/framework/packet.h"
#include "mediapipe/framework/packet_type.h"
#include "mediapipe/framework/input_stream.h"
#include "mediapipe/framework/output_stream.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {
constexpr char kTensorsTag[] = "TENSORS";

class TfliteGenderAndLandmarksCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);
  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

 private:
  std::unique_ptr<tflite::Interpreter> interpreter_;
};
REGISTER_CALCULATOR(TfliteGenderAndLandmarksCalculator);

// GetContract
absl::Status TfliteGenderAndLandmarksCalculator::GetContract(CalculatorContract* cc) {
  cc->Inputs().Tag(kTensorsTag).Set<std::vector<TfLiteTensor>>();
  // cc->Outputs().Tag(kTensorsTag).Set<std::vector<float>>();
  cc->Outputs().Tag("OUTPUT_TENSORS").Set<std::vector<float>>();
  return absl::OkStatus();
}

// Open
absl::Status TfliteGenderAndLandmarksCalculator::Open(CalculatorContext* cc) {
  return absl::OkStatus();
}

// Process
absl::Status TfliteGenderAndLandmarksCalculator::Process(CalculatorContext* cc) {
  RET_CHECK(!cc->Inputs().Tag(kTensorsTag).IsEmpty());
  const auto& input_tensors = cc->Inputs().Tag(kTensorsTag).Get<std::vector<TfLiteTensor>>();
  // Create a new output stream for the tensor values.
  auto output_stream = absl::make_unique<std::vector<float>>();
  // Loop through all the output tensors and append their values to the output stream.
  for (int i = 0; i < input_tensors.size(); ++i) {
    const auto& output_tensor = input_tensors[i];
    const float* output_tensor_flat = output_tensor.data.f;

    const int num_elements = output_tensor.bytes / sizeof(float);
    // Append the values in the output tensor to the output stream.
    for (int j = 0; j < num_elements; ++j) {
      output_stream->push_back(output_tensor_flat[j]);
      LOG(ERROR) << "Output tensor " << i << " value at index " << j << ": " << output_tensor_flat[j];
    }
  }
  // Send the output stream to the output packet.
  cc->Outputs().Tag("OUTPUT_TENSORS").Add(output_stream.release(), cc->InputTimestamp());
  return absl::OkStatus();
}
}  // namespace mediapipe