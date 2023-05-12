

/**
 * Required data to process turn the input frame into a packet.
 */
export type FrameMetadata = {
  width: number;
  height: number;
  timestampMs: number;
}

/**
 * Represents a detected landmark.
 */
export type NormalizedLandmark = {
  x: number;
  y: number;
  z: number;
  visibility?: number;
}

/**
 * Represents a list of landmarks.
 */
export type NormalizedLandmarkList = NormalizedLandmark[]

/**
 * GpuBuffers should all be compatible with Canvas' `drawImage`
 */
export type GpuBuffer = HTMLCanvasElement | HTMLImageElement | ImageBitmap

/**
 * Represents one of the possible results that can be passed to the listener.
 * TODO(mhays): Surface a typename so that someone who is coming at solutions
 * API directly can figure our which type they are looking at.
 */
export type RecognizedResult = GpuBuffer | NormalizedLandmarkList | FaceGeometry | undefined
/**
 * Represents a single result of a multi result listener.
 */
export interface ResultMap {
  [key: string]: RecognizedResult
}

/**
 * Shows the vertex type of a mesh in order to decode the vertex buffer list.
 */
export interface VertexType {
  VERTEX_PT: 0 // Position (XYZ) + Texture (UV)
}

/**
 * Shows the type of primitive shape in a mesh in order to give shape.
 */
export interface PrimitiveType {
  TRIANGLE: 0
}

/**
 * A representation of a mesh given by the Mesh3d proto
 */
export interface Mesh {
  getVertexBufferList(): Float32Array
  getVertexType(): VertexType
  getIndexBufferList(): Uint32Array
  getPrimitiveType(): PrimitiveType
}

/**
 * Represents the Layout of a Matrix for the MatrixData proto
 */
export interface Layout {
  COLUMN_MAJOR: 0
  ROW_MAJOR: 1
}

/**
 * A representation of a matrix given by the MatrixData proto.
 */
export interface MatrixData {
  getPackedDataList(): number[]
  getRows(): number
  getCols(): number
  getLayout(): Layout
}

/**
 * A representation of a face geometry from the face geometry proto.
 */
export interface FaceGeometry {
  getMesh(): Mesh
  getPoseTransformMatrix(): MatrixData
}


/**
 * This represents the callbacks from the graph's stream listeners.
 */
export type ResultMapListener = (map: ResultMap) => void

/**
 * Option descriptions that describe where to change an option in a graph.
 */
export type GraphOptionXRef = {
  cameraNear?: number;
  cameraFar?: number;
  cameraVerticalFovDegrees?: number;
  enableFaceGeometry?: boolean;
  selfieMode?: boolean;
  maxNumFaces?: number;
  refineLandmarks?: boolean;
  minDetectionConfidence?: number;
  minTrackingConfidence?: number;
}

/**
 * If your files are organized in some weird structure, you can customize how
 * the path is constructed by sending this to the config options for Solution.
 */
export type FileLocatorFn = (file: string, prefix: string) => string 

/**
 * During configuration, the user can specify streams and callbacks to be
 * attached to the graph. These will get reregistered on the user's behalf if
 * the graph gets reset.
 */
export type AttachListenerFn = (wants: string[], listener: (results: ResultMap) => void,) => void

/**
 * Users of the solution set options using a key-value map. However, specific
 * solutions will constrain the options with an exported interface.
 *
 * e.g.
 *
 * generalSolution.setOptions({\"myKey\": myValue}) becomes
 * specificSolution.setOptions({ myKey: myValue }}
 *
 * declare interface SpecificSolutionOptionList {
 *   myKey: number
 * };
 *
 * This works because `Object`s in JavaScript are just hashmaps, and we fit
 * meaning on top of those hashmaps with our type declarations.
 */
export interface OptionList {
  [key: string]: unknown
}

/**
 * Used in the constructor of Graph.
 */
export interface Graph {
  url: string
}

/**
 * Describes how to interpret OptionConfig. @see GraphOptionChangeRequest.
 * e.g., A NUMBER tells us to fill out valueNumber (which gets interpreted as
 * a double on the C++ side). We will add other types here as the need arises.
 */
export enum OptionType {
  NUMBER,
}

/**
 * Used to register keys with the solution. Right now it is limited to
 * graphOptionXref, which is specifically for options that will be fed to the
 * MediaPipe graph, but we'll also need options that are handled by the specific
 * solution to do work (e.g., selfie-flipping).
 */
export interface OptionConfig {
  type: OptionType
  graphOptionXref?: GraphOptionXRef
}

/**
 * The collection of option configuration entries, arranged by option name.
 */
export interface OptionConfigMap {
  [optionName: string]: OptionConfig
}


const customFileLocator: FileLocatorFn = (file, prefix) => `.${prefix}/${file}`;


/**
 * Options to configure the solution.
 */
export interface SolutionConfig {
  /**
   * The pack loader and the wasm loader need to be here. A file loader will be
   * provided to them, and They will get loaded asynchronously. We won't
   * continue initialization until they've completely loaded and run.
   */
  files: string[]

  /**
   * The binary graph. Can support multiple ways of getting that graph.
   */
  graph: Graph

  /**
   * See FileLocatorFn. Any file loading done on the user's behalf will use
   * locateFile if its profived. This includes WASM blobs and graph urls.
   */
  locateFile?: FileLocatorFn

  /**
   * Specifies how to interpret options fed to setOptions.
   */
  options?: OptionConfigMap

  /**
   * Whenever the graph is reset, this callback will be invoked. It is here that
   * you can attach listeners to the graph.
   */
  onRegisterListeners?: (attachFn: AttachListenerFn) => void
}

/**
 * Represents a graph that can be fed to mediapipe Solution.
 */
export interface GraphData {
  toArrayBuffer(): Promise<ArrayBuffer>
}

// These export names come from wasm_cc_binary BUILD rules. They belong to two
// different scripts that are loaded in parallel (see Promise.all, below).
// Because they mutate their respective variables, there is a race condition
// where they will stomp on each other if they choose the same name. Users
// won't normally see this race condition because they put script tags in the
// HTML, and HTML guarantees that the scripts will be run in order.
const API_EXPORT_NAME = 'createMediapipeSolutionsWasm'
const PACK_EXPORT_NAME = 'createMediapipeSolutionsPackedAssets'

export type ResultWasm = {
  isImage: () => boolean;
  getImage: () => GpuBuffer;
  isLandmarks: () => boolean;
  getLandmarks: () => NormalizedLandmarkList;
  isFaceGeometry: () => boolean;
  getFaceGeometry: () => any;
}

/**
 * Extracts the result from a collection of results.
 */
function extractWasmResult(result: ResultWasm): RecognizedResult {
  if (result?.isImage()) {
    return result?.getImage()
  }

  if (result?.isFaceGeometry()) {
    return result?.getFaceGeometry()
  }

  if (result?.isLandmarks()) {
    return result?.getLandmarks()
  }


  return undefined
}

/**
 * Returns the default path to a resource if the user did not overload the
 * locateFile parameter in SolutionConfig.
 */
function defaultLocateFile(file: string, prefix: string) {
  return prefix + file
}

/**
 * Sets an arbitrary value on `window`. This is a typing wrapper to prevent
 * errors for using `window` improperly.
 */
function setWindow<T>(key: string, value: T): void {
  ; ((window as unknown) as { [key: string]: T })[key] = value
}

/**
 * Gets an arbitrary value from `window`. This is a typing wrapper to prevent
 * errors for using `window` improperly.
 */
function getWindow(key: string): unknown {
  return ((window as unknown) as { [key: string]: unknown })[key]
}

/**
 * Dynamically loads a ascript into the current page and returns a `Promise`
 * that resolves when its loading is complete.
 */
function loadScript(url: string): Promise<void> {
  // console.log('loadScript:', url);
  const script = document.createElement('script')
  // console.log("loadscript debug")
  script.setAttribute('src', url)
  // Changed
  // Problem: 
  // This code "script.setAttribute('src', url)" outputs something like this "/gender_landmark_solution_packed_assets_loader.js"
  // But it can't be loaded, since the file location should be "./gender_landmark_solution_packed_assets_loader.js"
  // script.setAttribute('src', `.${url}`);

  // Fix 2 (Better):
  // loadScript(`.${this.locateFile(x, this.packagePath)}`)
  script.setAttribute('crossorigin', 'anonymous')
  // console.log("loadscript debug 2")
  // console.log("loadscript debug 3", script)
  document.body.appendChild(script)
  return new Promise((resolve) => {
    // console.log("return new Promise((resolve) =>")
    script.addEventListener(
      'load',
      () => {
        // console.log("resolve")
        resolve()
      },
      false,
    )
  })
}

function getPackagePath(): string {
  if (typeof window === 'object') {
    return (
      window.location.pathname
        .toString()
        .substring(0, window.location.pathname.toString().lastIndexOf('/')) +
      '/'
    )
  } else if (typeof location !== 'undefined') {
    // worker
    return (
      location.pathname.toString().substring(0, location.pathname.toString().lastIndexOf('/')) + '/'
    )
  } else {
    throw new Error(
      'solutions can only be loaded on a web page or in a web worker',
    )
  }
}

class GraphDataImpl {
  constructor(
    private readonly graph: Graph,
    private readonly locateFile: FileLocatorFn,
    private readonly packagePath: string,
  ) { }

  async toArrayBuffer(): Promise<ArrayBuffer> {
    if (this.graph.url) {
      const fetched = await fetch(
        // this.locateFile(this.graph.url, this.packagePath),
        `.${this.locateFile(this.graph.url, this.packagePath)}`,
      )
      if (fetched.body) {
        return fetched.arrayBuffer()
      }
    }
    return new ArrayBuffer(0)
  }
}

/**
 * Inputs to the graph. Currently only one input, a video frame, is
 * permitted, but this should encompass any input data to a solution.
 */
export interface FrameInputs {
  image: HTMLVideoElement | HTMLImageElement | HTMLCanvasElement
}

/**
 * MediaPipe Solution upon which all specific solutions will be built.
 */
export class Solution {
  private readonly packagePath: string
  private readonly locateFile: FileLocatorFn

  // BEGIN: Assigned during initializeWasm...
  private glCanvas!: HTMLCanvasElement
  private gl!: WebGL2RenderingContext
  private wasm!: any;
  private solutionWasm!: any
  packLoader: any;
  // END: Assigned during initializeWasm...

  private pendingChanges?: any

  private needToInitializeWasm = true
  private needToInitializeGraph = true

  constructor(private readonly config: SolutionConfig) {
    this.locateFile = (config && config.locateFile) || defaultLocateFile
    this.packagePath = getPackagePath()
  }

  close(): Promise<void> {
    if (this.solutionWasm) {
      this.solutionWasm.delete()
    }
    return Promise.resolve()
  }


  /**
   * Loads all of the dependent WASM files. This is heavy, so we make sure to
   * only do this once.
   */
  private async tryToInitializeWasm(): Promise<void> {
    if (!this.needToInitializeWasm) {
      return
    }
    // Set up the file loader for both external loaders.
    setWindow(API_EXPORT_NAME, { locateFile: this.locateFile })
    setWindow(PACK_EXPORT_NAME, { locateFile: this.locateFile })

    const files = this.config.files || []

    // Load all the files and wait for them to finish loading
    await Promise.all(files.map(async (x) => {
      console.log('loading file:', x)
      console.log("Package Path:", this.packagePath)
      console.log("Final Path:", this.locateFile(x, this.packagePath));
      console.log("Final Path with dot:", `.${this.locateFile(x, this.packagePath)}`);
      const scriptUrl = `.${this.locateFile(x, this.packagePath)}`
      console.log('loading file:', scriptUrl)
      await loadScript(scriptUrl)
      console.log('file loaded:', scriptUrl)
    }))

    type ApiPromiseFn = (packLoader: any) => Promise<any>;

    console.log("Debug apiFn:", getWindow(API_EXPORT_NAME))
    // Get the API function
    const apiFn = getWindow(API_EXPORT_NAME) as ApiPromiseFn
    console.log('apiFn:', apiFn);

    // Call the API function
    this.wasm = await apiFn(this.packLoader)
    console.log("wahhhh")

    // TODO(mhays): Developer should be able to explicitly load/unload a
    // solution to prevent stealing all of the WebGL resources (e.g., Chrome
    // may limit the number of WebGL contexts by domain).
    this.glCanvas = document.createElement("canvas")
    this.wasm.canvas = this.glCanvas
    this.wasm.createContext(this.glCanvas, /* useWebGl= */ true,  /* setInModule= */ true, {},)

    // The graph only needs to be loaded once into the solution, but the WASM
    // might re-initialize the solution, and that will specifically happen
    // during wasm.ProcessFrame.
    this.solutionWasm = new this.wasm.GenderLandmarksWasm()

    const graphData = new GraphDataImpl(
      this.config.graph,
      this.locateFile,
      this.packagePath,
    )

    await this.loadGraph(graphData)

    this.needToInitializeWasm = false
  }

  /**
   * Sets the options for the graph, potentially triggering a reinitialize.
   * The triggering is contingent upon the options matching those set up in
   * the solution configuration. If a match is found, initialize is set to run
   * on the next processFrame.
   *
   * We do not create a WASM object here, because it's possible (likely) that
   * WASM has not loaded yet (i.e., the user calls setOptions before calling
   * processFrame / initialize).  We'll do that during initialize when we know
   * it's safe.
   */
  setOptions(options: OptionList): void {
    if (!this.config.options) {
      return
    }

    const pendingChanges: any[] = []

    for (const option of Object.keys(options)) {
      // Look each option in the option config.
      const optionConfig = this.config.options[option]
      if (optionConfig) {
        if (optionConfig.graphOptionXref) {
          const optionValue = {
            valueNumber: optionConfig.type === OptionType.NUMBER ? (options[option] as number) : undefined,
          }
          // Combine the xref with the value. This is what the WASM will be expecting.
          const request = { ...optionConfig.graphOptionXref, ...optionValue }
          pendingChanges.push(request)
        }
      }
    }

    if (pendingChanges.length !== 0) {
      this.needToInitializeGraph = true
      this.pendingChanges = pendingChanges
    }
  }

  /**
   * Initializes the graph is it has not been loaded, or has been triggered to
   * reload (setOptions was called while the graph was running).
   */
  private async tryToInitializeGraph() {
    if (!this.needToInitializeGraph) {
      return
    }
    console.log("Before bind to Canvas")
    this.solutionWasm.bindTextureToCanvas()
    console.log("After bind to Canvas")

    // Move the video frame into the texture.
    const gl = this.glCanvas.getContext('webgl2')
    if (!gl) {
      alert('Failed to create WebGL canvas context when passing video frame.')
      return
    }
    this.gl = gl
    console.log("After gl")
    // Changing options on the graph will mutate the graph config.
    if (this.pendingChanges) {
      const changeList = new this.wasm.GraphOptionChangeRequestList()
      for (const change of this.pendingChanges) {
        changeList.push_back(change)
      }
      this.solutionWasm.changeOptions(changeList)
      changeList.delete()
      this.pendingChanges = undefined
    }

    if (this.config.onRegisterListeners) {
      console.log("onRegisterListeners")
      this.config.onRegisterListeners(this.attachListener.bind(this))
    }

    this.needToInitializeGraph = false
  }

  async initialize(): Promise<void> {
    console.log("Initializing Wasm...")
    await this.tryToInitializeWasm()
    console.log("Wasm Initialized!")
    console.log("Initializing Graph...")
    await this.tryToInitializeGraph()
    console.log("Graph initialized!")
  }

  async loadGraph(graph: GraphData): Promise<void> {
    const graphData = await graph.toArrayBuffer()
    this.solutionWasm.loadGraph(graphData)
  }

  /**
   * TODO(mhays): frame inputs will be an array of {name, packet}.
   */
  async processFrame(inputStreamName: string, metadata: FrameMetadata, inputs: FrameInputs): Promise<void> {
    await this.initialize()
    const gl = this.gl
    this.solutionWasm.bindTextureToCanvas()
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, inputs.image)
    gl.canvas.width = metadata.width
    gl.canvas.height = metadata.height
    console.log("")

    console.log("Before this.solutionWasm.processFrame")
    console.log("Image: ", inputs)
    console.log("this.solutionWasm.processFrame width: ", inputs.image.clientWidth)
    console.log("this.solutionWasm.processFrame height: ", inputs.image.clientHeight)
    console.log("this.solutionWasm.processFrame timestampMs: ", metadata.timestampMs)
    this.solutionWasm.processFrame(
      inputStreamName,
      {
        width: inputs.image.clientWidth,
        height: inputs.image.clientHeight,
        timestampMs: metadata.timestampMs
      }
    )
    console.log("After this.solutionWasm.processFrame")
  }



  /**
   * Attaches a listener that will be called when the graph produces
   * compatible packets on the named stream.
   */
  private attachListener(wants: string[], listener: ResultMapListener): void {
    const wantsCopy = [...wants] //image, multi_face_landmarks
    const wantsVector = new this.wasm.StringList() //new instance of StringList class that located in c++.
    //push the wants string element in wantsVector array variable in cpp
    for (const want of wants) {
      wantsVector.push_back(want)
    }
    const wasmListener = {
      onResults: (values: any) => {
        const results: ResultMap = {}
        for (let index = 0; index < wants.length; ++index) {
          results[wantsCopy[index]] = extractWasmResult(values.get(index)) //image, multiFaceGeometry, multifacelandmarks
        }
        listener(results)
      },
    }

    console.log("wantsVector", wantsVector)
    const packetListener = this.wasm.PacketListener.implement(wasmListener)
    this.solutionWasm.attachMultiListener(wantsVector, packetListener)
    wantsVector.delete()
  }
}
