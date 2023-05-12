import { Camera } from "./camera_util";
import { GenderLandmarksSolution, Results} from './gender_landmarks'

// CAMERA RELATED/ VIDEO RELATED
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


// GLOBAL
const genderLandmarks = new GenderLandmarksSolution({
    locateFile: (file) => {
      return `${__dirname}${file}`
    }
})






// The solution is already connected to the video, we just need to use
// the camera to drive it.
const camera = new Camera(videoSource, {
    onFrame: async () => {
      console.log("before send")
      await genderLandmarks.send({ image: videoSource })
      console.log("send")
    },
    width: isDesktop ? 1080 : 380,
    height: isDesktop ? 720 : 480,
  }
  )
  
  camera.start();