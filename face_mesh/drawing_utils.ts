/*@fileoverview Common drawing routines for our demos.
 */

import * as solutionsApi from './solution_api';

const CIRCLE = 2 * Math.PI;

/**
 * Draws circles onto the provided landmarks.
 */
export function drawLandmarks(
  ctx: CanvasRenderingContext2D,
  landmarks: any, color: string) {
  ctx.save();
  const canvas = ctx.canvas;
  ctx.scale(canvas.width, canvas.height);
  ctx.fillStyle = color;
  for (let loop = 0; loop < landmarks.size(); ++loop) {
    const landmark = landmarks.get(loop);
    const circle = new Path2D();
    circle.arc(landmark.x, landmark.y, 2 / canvas.width, 0, CIRCLE);
    ctx.fill(circle);
  }
  ctx.restore();
}

/**
 * Draws lines between landmarks (given a connection graph).
 */
export function drawConnectors(
  ctx: CanvasRenderingContext2D, landmarks: solutionsApi.NormalizedLandmarkList, connections: Array<[number, number]>, lineStyle: { color: string, lineWidth: number }) {
  ctx.save();
  const canvas = ctx.canvas;
  ctx.scale(canvas.width, canvas.height);
  ctx.strokeStyle = lineStyle.color;
  ctx.lineWidth = lineStyle.lineWidth / canvas.width;
  for (const connection of connections) {
    ctx.beginPath();
    const from = landmarks[connection[0]];
    const to = landmarks[connection[1]];
    ctx.moveTo(from.x, from.y);
    ctx.lineTo(to.x, to.y);
    ctx.stroke();
  }
  ctx.restore();
}

/**
 * Draws a filled rectangle that can be rotated.
 */
export function drawRectangle(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, rot: number, color: string) {
  ctx.save();
  const canvas = ctx.canvas;
  ctx.scale(canvas.width, canvas.height);
  ctx.fillStyle = color;
  ctx.translate((x + w / 2), (y + w / 2));
  ctx.rotate(rot * Math.PI / 180);
  ctx.fillRect(-w / 2, -h / 2, w, h);
  ctx.restore();
}
