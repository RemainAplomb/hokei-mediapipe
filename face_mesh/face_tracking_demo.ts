import { Camera } from "./camera_util";
import { FaceMeshSolution, Results, FACEMESH_TESSELATION, FACEMESH_RIGHT_IRIS, FACEMESH_LEFT_IRIS } from './face_mesh'
import { drawConnectors } from "@mediapipe/drawing_utils";

//Video element
const videoSource = document.createElement('video')
videoSource.setAttribute('id', 'video_in')
videoSource.autofocus = true
videoSource.playsInline = true
videoSource.style.position = 'absolute'
videoSource.style.visibility = 'hidden'

//Canvas element
const canvasOutput = document.createElement('canvas')
//Context
const canvasCtx = canvasOutput.getContext('2d')
canvasOutput.setAttribute('id', 'video_out')
canvasOutput.style.zIndex = '3'
canvasOutput.style.backgroundColor = 'transparent'
canvasOutput.style.position = 'absolute'
canvasOutput.style.transform = 'scaleX(-1)'


let isDesktop = window.innerWidth > 640 ? true : false;

if (isDesktop) {
  //If desktop
  videoSource.width = 1080
  videoSource.height = 780
  canvasOutput.width = 1080
  canvasOutput.height = 780
  canvasOutput.style.width = "1080px"
  canvasOutput.style.height = "780px"
} else {
  videoSource.width = 380
  videoSource.height = 480
  canvasOutput.width = 380
  canvasOutput.height = 480
  canvasOutput.style.width = "380px"
  canvasOutput.style.height = "480px"
}


//Container element
export const VideoPlayer = document.createElement('div')
VideoPlayer.appendChild(videoSource)
VideoPlayer.appendChild(canvasOutput)
VideoPlayer.style.position = 'relative'
VideoPlayer.style.margin = '0'
VideoPlayer.style.padding = '0'
document.getElementById('container')?.appendChild(VideoPlayer)


const faceMesh = new FaceMeshSolution({
  locateFile: (file) => {
    return `${__dirname}${file}`
  }
})

const onResults = (results: Results) => {
  canvasCtx?.save();
  canvasCtx?.clearRect(0, 0, canvasOutput.width, canvasOutput.height);
  canvasCtx?.drawImage(results.image, 0, 0, canvasOutput.width, canvasOutput.height);
  console.log(results)
  // canvasCtx?.fillRect(25, 25, 100, 100);
  if (results.multiFaceLandmarks) {
    // console.log(results.multiFaceGeometry[0])
    for (const landmarks of results.multiFaceLandmarks) {
      if (canvasCtx) {
        drawConnectors(canvasCtx, landmarks, FACEMESH_TESSELATION, { color: "#ff0000", lineWidth: window.innerWidth > 600 ? 2 : .25 });
        // drawConnectors(canvasCtx, landmarks, FACEMESH_RIGHT_IRIS, { color: '#0EF4F2', lineWidth: window.innerWidth > 600 ? 3 : .25 });
        // drawConnectors(canvasCtx, landmarks, FACEMESH_LEFT_IRIS, { color: '#30FF30', lineWidth: window.innerWidth > 600 ? 3 : .25 });
      }
    }
  }
  canvasCtx?.restore();
  // if (!results.multiFaceLandmarks) return
  // console.log(results.multiFaceLandmarks[0][4])
}

// faceMesh.setOptions(solutionOptions)
faceMesh.onResults(onResults)

// The solution is already connected to the video, we just need to use
// the camera to drive it.
const camera = new Camera(videoSource, {
  onFrame: async () => {
    await faceMesh.send({ image: videoSource })
  },
  width: isDesktop ? 1080 : 380,
  height: isDesktop ? 720 : 480,
}
)

camera.start();