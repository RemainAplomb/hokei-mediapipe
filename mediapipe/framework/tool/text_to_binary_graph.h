// #include <stdlib.h>

// #include <fstream>
// #include <string>

// #include "absl/flags/flag.h"
// #include "absl/flags/parse.h"
// #include "mediapipe/framework/calculator.pb.h"
// #include "mediapipe/framework/port/advanced_proto_inc.h"
// #include "mediapipe/framework/port/canonical_errors.h"
// #include "mediapipe/framework/port/ret_check.h"
// #include "mediapipe/framework/port/status.h"

// #ifndef MEDIAPIPE_FRAMEWORK_TEXT_TO_BINARY_GRAPH_H_
// #define MEDIAPIPE_FRAMEWORK_TEXT_TO_BINARY_GRAPH_H_

// namespace mediapipe {
//     using mediapipe::CalculatorGraphConfig;

//     absl::Status ReadProto(proto_ns::io::ZeroCopyInputStream* in, bool read_text, const std::string& source, proto_ns::Message* result);

//     absl::Status WriteProto(const proto_ns::Message& message, bool write_text, const std::string& dest, proto_ns::io::ZeroCopyOutputStream* out);

//     absl::Status ReadFile(const std::string& proto_source, bool read_text, proto_ns::Message* result);

//     absl::Status WriteFile(const std::string& proto_output, bool write_text, const proto_ns::Message& message);


// }  // namespace mediapipe

// #endif  // MEDIAPIPE_FRAMEWORK_TEXT_TO_BINARY_GRAPH_H_