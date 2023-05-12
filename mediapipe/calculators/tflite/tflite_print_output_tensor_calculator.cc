#include "mediapipe/framework/calculator_framework.h"

#include "tensorflow/lite/model.h"
#include "mediapipe/framework/packet.h"
#include "mediapipe/framework/packet_type.h"
#include "mediapipe/framework/input_stream.h"

#include "mediapipe/framework/formats/tensor.h"
#include "mediapipe/framework/port/logging.h"

namespace mediapipe {

constexpr char kTensorsTag[] = "TENSORS";

// A custom calculator that prints the output tensor values to the console.
class TflitePrintOutputTensorCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;

  absl::Status Process(CalculatorContext* cc) override;
};
REGISTER_CALCULATOR(TflitePrintOutputTensorCalculator);

// GetContract
absl::Status TflitePrintOutputTensorCalculator::GetContract(CalculatorContract* cc) {
  cc->Inputs().Tag(kTensorsTag).Set<std::vector<TfLiteTensor>>();
  return absl::OkStatus();
}

// Open
absl::Status TflitePrintOutputTensorCalculator::Open(CalculatorContext* cc) {
  return absl::OkStatus();
}

// Process
absl::Status TflitePrintOutputTensorCalculator::Process(CalculatorContext* cc) {
  RET_CHECK(!cc->Inputs().Tag(kTensorsTag).IsEmpty());
  const auto& input_tensors = cc->Inputs().Tag(kTensorsTag).Get<std::vector<TfLiteTensor>>();
  
  // Loop through all the output tensors and print their values.
  for (int i = 0; i < input_tensors.size(); ++i) {
    const auto& output_tensor = input_tensors[i];
    const float* output_tensor_flat = output_tensor.data.f;

    const int num_elements = output_tensor.bytes / sizeof(float);
    // Print the values in the output tensor to the console.
    for (int j = 0; j < num_elements; ++j) {
      LOG(ERROR) << "Output tensor " << i << " value at index " << j << ": " << output_tensor_flat[j];
    }
  }

  return absl::OkStatus();
}

}  // namespace mediapipe
