import * as genderLandmarksApi from './gender_landmarks_solution_api'


// Global

/**
 * GpuBuffers should all be compatible with Canvas' `drawImage`
 */
type GpuBuffer = HTMLCanvasElement | HTMLImageElement | ImageBitmap

/**
 * Legal inputs for FaceMeshSolution.
 */
export interface Inputs {
    image: HTMLVideoElement | HTMLImageElement | HTMLCanvasElement
}

/**
 * Listener for any results from PoseSolution.
 */
export type ResultsListener = (results: Results) => Promise<void> | void

/**
 * Contains all of the setup options to drive the face solution.
 */
export interface GenderLandmarksConfig {
    locateFile?: (path: string, prefix?: string) => string
}

/**
 * Configurable options for FaceMesh.
 */
export interface Options {
    cameraNear?: number
    cameraFar?: number
    cameraVerticalFovDegrees?: number
    enableFaceGeometry?: boolean
    selfieMode?: boolean
    maxNumFaces?: number
    refineLandmarks?: boolean
    minDetectionConfidence?: number
    minTrackingConfidence?: number
}

/**
 * Possible results from PoseSolution.
 */
export interface Results {
    image: GpuBuffer
    outputTensorValues: any;
    // multiFaceGeometry: any;
    // multiFaceLandmarks: genderLandmarksApi.NormalizedLandmarkList
}
  



export class GenderLandmarksSolution {
    private readonly solution: genderLandmarksApi.Solution
    private listener?: ResultsListener
  
    constructor(config?: GenderLandmarksConfig) {
      config = config || {}
      this.solution = new genderLandmarksApi.Solution({
        locateFile: config.locateFile,
        files: [
          'gender_landmark_solution_packed_assets_loader.js',
          'gender_landmark_solution_wasm_bin.js'
        ],
        graph: { url: 'gender_landmarks.binarypb' },
        onRegisterListeners: (attachFn: genderLandmarksApi.AttachListenerFn) => {
          console.log("Test: onRegisterListeners: (attachFn: genderLandmarksApi.AttachListenerFn) =>")
          const thiz = this
          // The listeners can be attached before or after the graph is loaded.
          // We will eventually hide these inside the face_mesh api so that a
          // developer doesn't have to know the stream names.
          attachFn(
            ['output_video', 'output_tensor_values'],
            (results: genderLandmarksApi.ResultMap) => {
              if (thiz.listener) {
                thiz.listener({
                  image: results['output_video'] as genderLandmarksApi.GpuBuffer,
                  outputTensorValues: results['output_tensor_values'] as any,
                  // multiFaceLandmarks: results['multi_face_landmarks'] as genderLandmarksApi.NormalizedLandmarkList,
                })
              }
            },
          )
          console.log("after attachFn")
        },
      })
    }
  
    /**
     * Shuts down the object. Call before creating a new instance.aph",
          "//mediapipe/framework/port:logging",
          "//mediapipe/framework/port:parse_text_proto",
          "//mediapipe/framework/port:status",
     */
    close(): Promise<void> {
      this.solution.close()
      return Promise.resolve()
    }
  
    /**
     * Registers a single callback that will carry any results that occur
     * after calling Send().
     */
    onResults(listener: ResultsListener): void {
      this.listener = listener
    }
  
    /**
     * Initializes the solution. This includes loading ML models and mediapipe
     * configurations, as well as setting up potential listeners for metadata. If
     * `initialize` is not called manually, then it will be called the first time
     * the developer calls `send`.
     */
    async initialize(): Promise<void> {
      await this.solution.initialize()
    }
  
  
    /**
     * Sends inputs to the solution. The developer can await the results, which
     * resolves when the graph and any listeners have completed.
     */
    async send(inputs: Inputs): Promise<void> {
      if (inputs.image) {
        const image = inputs.image
        await this.solution.processFrame('input_video', {
          height: image.clientHeight,
          width: image.clientWidth,
          timestampMs: performance.now(),
        },
          { image },
        )
      }
    }
  
    /**
     * Adjusts options in the solution. This may trigger a graph reload the next
     * time the graph tries to run.
     */
    setOptions(options: Options): void {
      this.solution.setOptions(
        (options as unknown) as genderLandmarksApi.OptionList,
      )
    }
  }