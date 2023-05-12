
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "mediapipe/gpu/gpu_shared_data_internal.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h" // GLES_VERSION_COMPAT
#include "mediapipe/gpu/shader_util.h"

constexpr char kInputVideoTag[] = "VIDEO_IN";
constexpr char kOutputVideoTag[] = "VIDEO_OUT";

enum
{
  ATTRIB_VERTEX,
  ATTRIB_TEXTURE_POSITION,
  NUM_ATTRIBUTES
};

namespace mediapipe
{

  class RenderGPUBufferToCanvasCalculator : public CalculatorBase
  {
  public:
    RenderGPUBufferToCanvasCalculator() : initialized_(false) {}
    RenderGPUBufferToCanvasCalculator(const RenderGPUBufferToCanvasCalculator &) = delete;
    RenderGPUBufferToCanvasCalculator &operator=(const RenderGPUBufferToCanvasCalculator &) = delete;
    ~RenderGPUBufferToCanvasCalculator() override = default;

    static absl::Status GetContract(CalculatorContract *cc);
    absl::Status Open(CalculatorContext *cc) override;
    absl::Status Process(CalculatorContext *cc) override;
    absl::Status Close(CalculatorContext *cc) override;

    absl::Status GlBind() { return absl::OkStatus(); }

    void GetOutputDimensions(int src_width, int src_height, int *dst_width, int *dst_height)
    {
      *dst_width = src_width;
      *dst_height = src_height;
    }

    virtual GpuBufferFormat GetOutputFormat() { return GpuBufferFormat::kBGRA32; }

    absl::Status GlSetup();
    absl::Status GlRender(); //(const GlTexture& src, const GlTexture& dst);
    absl::Status GlTeardown();

  protected:
    template <typename F>
    auto RunInGlContext(F &&f) -> decltype(std::declval<GlCalculatorHelper>().RunInGlContext(f))
    {
      return helper_.RunInGlContext(std::forward<F>(f));
    }

    GlCalculatorHelper helper_;
    bool initialized_;

  private:
    absl::Status LoadOptions(CalculatorContext *cc);
    GLuint program_ = 0;
    GLint video_frame;
  };

  REGISTER_CALCULATOR(RenderGPUBufferToCanvasCalculator);

  absl::Status RenderGPUBufferToCanvasCalculator::LoadOptions(CalculatorContext *cc)
  {
    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::GetContract(CalculatorContract *cc)
  {
    CHECK_GE(cc->Inputs().NumEntries(), 1);

    RET_CHECK(!cc->Inputs().GetTags().empty());
    RET_CHECK(!cc->Outputs().GetTags().empty());

    TagOrIndex(&cc->Inputs(), kInputVideoTag, 0).Set<GpuBuffer>();

    TagOrIndex(&cc->Outputs(), kOutputVideoTag, 0).Set<GpuBuffer>();

    MP_RETURN_IF_ERROR(GlCalculatorHelper::UpdateContract(cc));

    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::Open(CalculatorContext *cc)
  {
    // Inform the framework that we always output at the same timestamp
    // as we receive a packet at.
    cc->SetOffset(mediapipe::TimestampDiff(0));

    // Let the helper access the GL context information.
    MP_RETURN_IF_ERROR(helper_.Open(cc));

    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::Process(CalculatorContext *cc)
  {
    MP_RETURN_IF_ERROR(RunInGlContext([this, cc]() -> absl::Status
                                      {
      if (!initialized_) {
        MP_RETURN_IF_ERROR(GlSetup());
        initialized_ = true;
      }
      //Prepare a input that contains the display
      const auto& input = TagOrIndex(cc->Inputs(), kInputVideoTag, 0).Get<GpuBuffer>();
      auto src = helper_.CreateSourceTexture(input);
      int dst_width;
      int dst_height;
      GetOutputDimensions(src.width(), src.height(), &dst_width, &dst_height);
      auto dst = helper_.CreateDestinationTexture(dst_width, dst_height, GetOutputFormat());
      helper_.BindFramebuffer(dst);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(src.target(), src.name());
      // Run core program.
      MP_RETURN_IF_ERROR(GlRender()); // (src, dst));
      auto output = dst.GetFrame<GpuBuffer>();
      glFlush();
      TagOrIndex(&cc->Outputs(), kOutputVideoTag, 0).Add(output.release(), cc->InputTimestamp());
      src.Release();
      return absl::OkStatus(); }));

    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::Close(CalculatorContext *cc)
  {
    return RunInGlContext([this]() -> absl::Status
                          { return GlTeardown(); });
  }

  absl::Status RenderGPUBufferToCanvasCalculator::GlSetup()
  {

    const GLint attr_location[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX,
        ATTRIB_TEXTURE_POSITION,
    };
    const GLchar *attr_name[NUM_ATTRIBUTES] = {"position", "texture_coordinate"};

    const GLchar *frag_src = GLES_VERSION_COMPAT
        R"(
    #if __VERSION__ < 130
      #define in varying
    #endif  // __VERSION__ < 130

    #ifdef GL_ES
      #define fragColor gl_FragColor
      precision highp float;
    #else
      out vec4 fragColor;
    #endif
      uniform sampler2D video_frame;
      in vec2 sample_coordinate;
      void main() {
        vec4 color = texture2D(video_frame, sample_coordinate);
        fragColor = vec4(color.r, color.g, color.b, color.a);
      }
  )";

    // Creates a GLSL program by compiling and linking the provided shaders.
    // Also obtains the locations of the requested attributes.
    auto glStatus = GlhCreateProgram(kBasicVertexShader, frag_src, NUM_ATTRIBUTES, (const GLchar **)&attr_name[0], attr_location, &program_);

    RET_CHECK(program_) << "Problem initializing the program.";
    video_frame = glGetUniformLocation(program_, "video_frame");
    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::GlRender()
  {
    static const GLfloat square_vertices[] = {
        -1.0f, -1.0f, // bottom left
        1.0f, -1.0f,  // bottom right
        -1.0f, 1.0f,  // top left
        1.0f, 1.0f,   // top right
    };
    static const GLfloat texture_vertices[] = {
        0.0f, 1.0f, // top left
        1.0f, 1.0f, // top right
        0.0f, 0.0f, // bottom left
        1.0f, 0.0f, // bottom right
    };

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // binding to canvas

    // program
    glUseProgram(program_); // Q do they return anything, can I call them once?
    glUniform1i(video_frame, 1);

    // vertex storage
    GLuint vbo[2];
    glGenBuffers(2, vbo);
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // // vbo 0
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), square_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, nullptr);

    // vbo 1
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), texture_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_TEXTURE_POSITION);
    glVertexAttribPointer(ATTRIB_TEXTURE_POSITION, 2, GL_FLOAT, 0, 0, nullptr);

    // draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // cleanup
    glDisableVertexAttribArray(ATTRIB_VERTEX);
    glDisableVertexAttribArray(ATTRIB_TEXTURE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, vbo);

    return absl::OkStatus();
  }

  absl::Status RenderGPUBufferToCanvasCalculator::GlTeardown()
  {
    helper_.RunInGlContext([this]
                           {
    if (program_) {
      glDeleteProgram(program_);
      program_ = 0;
    } });

    return absl::OkStatus();
  }
}