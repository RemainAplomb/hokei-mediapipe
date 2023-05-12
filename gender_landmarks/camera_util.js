"use strict";
var __assign = (this && this.__assign) || function () {
    __assign = Object.assign || function(t) {
        for (var s, i = 1, n = arguments.length; i < n; i++) {
            s = arguments[i];
            for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p))
                t[p] = s[p];
        }
        return t;
    };
    return __assign.apply(this, arguments);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.Camera = void 0;
var defaultOptions = {
    facingMode: 'user',
    width: 640,
    height: 480
};
/**
 * Represents a mediadevice camera. It will start a camera and then run an
 * animation loop that calls the user for each frame. If the user spends too
 * much time in the callback, then animation frames will be dropped.
 */
var Camera = /** @class */ (function () {
    function Camera(videoElement, options) {
        // The video frame rate may be lower than the (browser-controlled) tick rate;
        // we use this to avoid processing the same frame twice.
        this.lastVideoTime = 0;
        this.video = videoElement;
        this.options = __assign(__assign({}, defaultOptions), options);
    }
    /**
     * Drives tick() be called on the next animation frame.
     */
    Camera.prototype.requestTick = function () {
        var _this = this;
        window.requestAnimationFrame(function () {
            _this.tick();
        });
    };
    /**
    * Will be called when the camera feed has been acquired. We then start
    * streaming this into our video object.
    * @param {!MediaStream} stream The video stream.
    */
    Camera.prototype.onAcquiredUserMedia = function (stream) {
        var _this = this;
        this.video.srcObject = stream;
        this.video.onloadedmetadata = function () {
            _this.video.play();
            _this.requestTick();
        };
    };
    Camera.prototype.tick = function () {
        var _this = this;
        var result = null;
        // TODO(b/168744981): Do not schedule ticks when we are paused.
        if (!this.video.paused &&
            this.video.currentTime !== this.lastVideoTime) {
            this.lastVideoTime = this.video.currentTime;
            result = this.options.onFrame();
        }
        if (result) {
            result.then(function () {
                _this.requestTick();
            });
        }
        else {
            this.requestTick();
        }
    };
    Camera.prototype.start = function () {
        var _this = this;
        if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
            // This can be worked around with legacy commands, but we choose not to
            // do so for this demo.
            alert('No navigator.mediaDevices.getUserMedia exists.');
        }
        var options = this.options;
        return navigator.mediaDevices
            .getUserMedia({
            video: {
                facingMode: options.facingMode,
                width: options.width,
                height: options.height,
            }
        })
            .then(function (stream) {
            _this.onAcquiredUserMedia(stream);
        })
            .catch(function (e) {
            console.error("Failed to acquire camera feed: ".concat(e));
            alert("Failed to acquire camera feed: ".concat(e));
            throw e;
        });
    };
    return Camera;
}());
exports.Camera = Camera;
//exports.Camera = Camera;
